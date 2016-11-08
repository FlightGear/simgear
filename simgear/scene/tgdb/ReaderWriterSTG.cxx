// tileentry.cxx -- routines to handle a scenery tile
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2001  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "ReaderWriterSTG.hxx"

#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/IntersectionVisitor>

#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>

#include <simgear/math/sg_random.h>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/scene/util/OptionsReadFileCallback.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/tgdb/apt_signs.hxx>
#include <simgear/scene/tgdb/obj.hxx>

#include "SGOceanTile.hxx"

namespace simgear {

/// Ok, this is a hack - we do not exactly know if it's an airport or not.
/// This feature might also vanish again later. This is currently to
/// support testing an external ai component that just loads the the airports
/// and supports ground queries on only these areas.
static bool isAirportBtg(const std::string& name)
{
    if (name.size() < 8)
        return false;
    if (name.substr(4, 4) != ".btg")
        return false;
    for (unsigned i = 0; i < 4; ++i) {
        if ('A' <= name[i] && name[i] <= 'Z')
            continue;
        return false;
    }
    return true;
}

static SGBucket bucketIndexFromFileName(const std::string& fileName)
{
  // Extract the bucket from the filename
  std::istringstream ss(osgDB::getNameLessExtension(fileName));
  long index;
  ss >> index;
  if (ss.fail())
    return SGBucket();

  return SGBucket(index);
}

struct ReaderWriterSTG::_ModelBin {
    struct _Object {
        std::string _errorLocation;
        std::string _token;
        std::string _name;
        osg::ref_ptr<SGReaderWriterOptions> _options;
    };
    struct _ObjectStatic {
        _ObjectStatic() : _agl(false), _proxy(false), _lon(0), _lat(0), _elev(0), _hdg(0), _pitch(0), _roll(0), _range(SG_OBJECT_RANGE_ROUGH) { }
        std::string _errorLocation;
        std::string _token;
        std::string _name;
        bool _agl;
        bool _proxy;
        double _lon, _lat, _elev;
        double _hdg, _pitch, _roll;
        double _range;
        osg::ref_ptr<SGReaderWriterOptions> _options;
    };
    struct _Sign {
        _Sign() : _agl(false), _lon(0), _lat(0), _elev(0), _hdg(0), _size(-1) { }
        std::string _errorLocation;
        std::string _token;
        std::string _name;
        bool _agl;
        double _lon, _lat, _elev;
        double _hdg;
        int _size;
    };

    class DelayLoadReadFileCallback : public OptionsReadFileCallback {

    private:
      // QuadTreeBuilder for structuring static objects
      struct MakeQuadLeaf {
          osg::LOD* operator() () const { return new osg::LOD; }
      };
      struct AddModelLOD {
          void operator() (osg::LOD* leaf, _ObjectStatic o) const
          {
            osg::ref_ptr<osg::Node> node;
            if (o._proxy)  {
                osg::ref_ptr<osg::ProxyNode> proxy = new osg::ProxyNode;
                proxy->setName("proxyNode");
                proxy->setLoadingExternalReferenceMode(osg::ProxyNode::DEFER_LOADING_TO_DATABASE_PAGER);
                proxy->setFileName(0, o._name);
                proxy->setDatabaseOptions(o._options.get());

                // Give the node some values so the Quadtree builder has
                // a BoundingBox to work with prior to the model being loaded.
                proxy->setCenter(osg::Vec3f(0.0f,0.0f,0.0f));
                proxy->setRadius(10);
                proxy->setCenterMode(osg::ProxyNode::UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED);
                node = proxy;
            } else {
                node = osgDB::readRefNodeFile(o._name, o._options.get());
                if (!node.valid()) {
                    SG_LOG(SG_TERRAIN, SG_ALERT, o._errorLocation << ": Failed to load "
                           << o._token << " '" << o._name << "'");
                    return;
                }
            }
            if (SGPath(o._name).lower_extension() == "ac")
                node->setNodeMask(~simgear::MODELLIGHT_BIT);

            osg::Matrix matrix;
            matrix = makeZUpFrame(SGGeod::fromDegM(o._lon, o._lat, o._elev));
            matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(o._hdg), osg::Vec3(0, 0, 1)));
            matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(o._pitch), osg::Vec3(0, 1, 0)));
            matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(o._roll), osg::Vec3(1, 0, 0)));

            osg::MatrixTransform* matrixTransform;
            matrixTransform = new osg::MatrixTransform(matrix);
            matrixTransform->setName("rotateStaticObject");
            matrixTransform->setDataVariance(osg::Object::STATIC);
            matrixTransform->addChild(node.get());

            leaf->addChild(matrixTransform, 0, o._range);
          }
      };
      struct GetModelLODCoord {
          GetModelLODCoord() {}
          GetModelLODCoord(const GetModelLODCoord& rhs)
          {}
          osg::Vec3 operator() (const _ObjectStatic& o) const
          {
              SGVec3d coord;
              SGGeodesy::SGGeodToCart(SGGeod::fromDegM(o._lon, o._lat, o._elev), coord);
              return toOsg(coord);
          }
      };
      typedef QuadTreeBuilder<osg::LOD*, _ObjectStatic, MakeQuadLeaf, AddModelLOD,
                              GetModelLODCoord>  STGObjectsQuadtree;


    public:
        virtual osgDB::ReaderWriter::ReadResult
        readNode(const std::string&, const osgDB::Options*)
        {
            STGObjectsQuadtree quadtree((GetModelLODCoord()), (AddModelLOD()));
            quadtree.buildQuadTree(_objectStaticList.begin(), _objectStaticList.end());
            osg::ref_ptr<osg::Group> group = quadtree.getRoot();
            group->setName("STG-group-A");
            group->setDataVariance(osg::Object::STATIC);

            simgear::AirportSignBuilder signBuilder(_options->getMaterialLib(), _bucket.get_center());
            for (std::list<_Sign>::iterator i = _signList.begin(); i != _signList.end(); ++i)
                signBuilder.addSign(SGGeod::fromDegM(i->_lon, i->_lat, i->_elev), i->_hdg, i->_name, i->_size);
            if (signBuilder.getSignsGroup())
                group->addChild(signBuilder.getSignsGroup());

            return group.release();
        }

        mt _seed;
        std::list<_ObjectStatic> _objectStaticList;
        std::list<_Sign> _signList;

        /// The original options to use for this bunch of models
        osg::ref_ptr<SGReaderWriterOptions> _options;
        SGBucket _bucket;
    };

    _ModelBin() :
        _object_range_bare(SG_OBJECT_RANGE_BARE),
        _object_range_rough(SG_OBJECT_RANGE_ROUGH),
        _object_range_detailed(SG_OBJECT_RANGE_DETAILED),
        _building_mesh_enabled(false),
        _foundBase(false)
    { }

    SGReaderWriterOptions* sharedOptions(const std::string& filePath, const osgDB::Options* options)
    {
        osg::ref_ptr<SGReaderWriterOptions> sharedOptions;
        sharedOptions = SGReaderWriterOptions::copyOrCreate(options);
        sharedOptions->getDatabasePathList().clear();

        SGPath path = filePath;
        path.append(".."); path.append(".."); path.append("..");
        sharedOptions->getDatabasePathList().push_back(path.local8BitStr());

        // ensure Models directory synced via TerraSync is searched before the copy in
        // FG_ROOT, so that updated models can be used.
        std::string terrasync_root = options->getPluginStringData("SimGear::TERRASYNC_ROOT");
        if (!terrasync_root.empty()) {
            sharedOptions->getDatabasePathList().push_back(terrasync_root);
        }

        std::string fg_root = options->getPluginStringData("SimGear::FG_ROOT");
        sharedOptions->getDatabasePathList().push_back(fg_root);

        // TODO how should we handle this for OBJECT_SHARED?
        sharedOptions->setModelData
        (
            sharedOptions->getModelData()
          ? sharedOptions->getModelData()->clone()
          : 0
        );

        return sharedOptions.release();
    }
    SGReaderWriterOptions* staticOptions(const std::string& filePath, const osgDB::Options* options)
    {
        osg::ref_ptr<SGReaderWriterOptions> staticOptions;
        staticOptions = SGReaderWriterOptions::copyOrCreate(options);
        staticOptions->getDatabasePathList().clear();

        staticOptions->getDatabasePathList().push_back(filePath);
        staticOptions->setObjectCacheHint(osgDB::Options::CACHE_NONE);

        // Every model needs its own SGModelData to ensure load/unload is
        // working properly
        staticOptions->setModelData
        (
            staticOptions->getModelData()
          ? staticOptions->getModelData()->clone()
          : 0
        );

        return staticOptions.release();
    }

    double elevation(osg::Group& group, const SGGeod& geod)
    {
        SGVec3d start = SGVec3d::fromGeod(SGGeod::fromGeodM(geod, 10000));
        SGVec3d end = SGVec3d::fromGeod(SGGeod::fromGeodM(geod, -1000));

        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector;
        intersector = new osgUtil::LineSegmentIntersector(toOsg(start), toOsg(end));
        osgUtil::IntersectionVisitor visitor(intersector.get());
        group.accept(visitor);

        if (!intersector->containsIntersections())
            return 0;

        SGVec3d cart = toSG(intersector->getFirstIntersection().getWorldIntersectPoint());
        return SGGeod::fromCart(cart).getElevationM();
    }

    bool read(const std::string& absoluteFileName, const osgDB::Options* options)
    {

				// Determine object ranges.  Mesh size needs to be accounted for.
				_object_range_bare = 2000.0f + atof(options->getPluginStringData("SimGear::LOD_RANGE_BARE").c_str());
				_object_range_rough = 2000.0f + atof(options->getPluginStringData("SimGear::LOD_RANGE_ROUGH").c_str());
				_object_range_detailed = 2000.0f + atof(options->getPluginStringData("SimGear::LOD_RANGE_DETAILED").c_str());
				_building_mesh_enabled = (options->getPluginStringData("SimGear::RENDER_BUILDING_MESH") == "true");

        if (absoluteFileName.empty())
            return false;

        sg_gzifstream stream(absoluteFileName);
        if (!stream.is_open())
            return false;

        SG_LOG(SG_TERRAIN, SG_INFO, "Loading stg file " << absoluteFileName);

        std::string filePath = osgDB::getFilePath(absoluteFileName);

        // Bucket provides a consistent seed
        // so we have consistent set of pseudo-random numbers for each STG file
        mt seed;
        int bucket = atoi(SGPath(absoluteFileName).file_base().c_str());
        mt_init(&seed, bucket);

        // do only load airport btg files.
        bool onlyAirports = options->getPluginStringData("SimGear::FG_ONLY_AIRPORTS") == "ON";
        // do only load terrain btg files
        bool onlyTerrain = options->getPluginStringData("SimGear::FG_ONLY_TERRAIN") == "ON";

        while (!stream.eof()) {
            // read a line
            std::string line;
            std::getline(stream, line);

            // strip comments
            std::string::size_type hash_pos = line.find('#');
            if (hash_pos != std::string::npos)
                line.resize(hash_pos);

            // and process further
            std::stringstream in(line);

            std::string token;
            in >> token;

            // No comment
            if (token.empty())
                continue;

            // Then there is always a name
            std::string name;
            in >> name;

            SGPath path = filePath;
            path.append(name);

            if (token == "OBJECT_BASE") {
                // Load only once (first found)
                SG_LOG( SG_TERRAIN, SG_BULK, "    " << token << " " << name );
                _foundBase = true;
                if (!onlyAirports || isAirportBtg(name)) {
                    _Object obj;
                    obj._errorLocation = absoluteFileName;
                    obj._token = token;
                    obj._name = path.local8BitStr();
                    obj._options = staticOptions(filePath, options);
                    _objectList.push_back(obj);
                }

            } else if (token == "OBJECT") {
                if (!onlyAirports || isAirportBtg(name)) {
                    _Object obj;
                    obj._errorLocation = absoluteFileName;
                    obj._token = token;
                    obj._name = path.local8BitStr();
                    obj._options = staticOptions(filePath, options);
                    _objectList.push_back(obj);
                }

            } else if (!onlyTerrain) {
                // Load non-terrain objects

								// Determine an appropriate range for the object, which has some randomness
								double range = _object_range_rough;
								double lrand = mt_rand(&seed);
								if      (lrand < 0.1) range = range * 2.0;
								else if (lrand < 0.4) range = range * 1.5;

                if (token == "OBJECT_STATIC" || token == "OBJECT_STATIC_AGL") {
										osg::ref_ptr<SGReaderWriterOptions> opt;
										opt = staticOptions(filePath, options);
										if (SGPath(name).lower_extension() == "ac")
												opt->setInstantiateEffects(true);
										else
												opt->setInstantiateEffects(false);
										_ObjectStatic obj;
										obj._errorLocation = absoluteFileName;
										obj._token = token;
										obj._name = name;
										obj._agl = (token == "OBJECT_STATIC_AGL");
										obj._proxy = true;
										in >> obj._lon >> obj._lat >> obj._elev >> obj._hdg >> obj._pitch >> obj._roll;
										obj._range = range;
										obj._options = opt;
										_objectStaticList.push_back(obj);
                } else if (token == "OBJECT_SHARED" || token == "OBJECT_SHARED_AGL") {
										osg::ref_ptr<SGReaderWriterOptions> opt;
										opt = sharedOptions(filePath, options);
										if (SGPath(name).lower_extension() == "ac")
												opt->setInstantiateEffects(true);
										else
												opt->setInstantiateEffects(false);
										_ObjectStatic obj;
										obj._errorLocation = absoluteFileName;
										obj._token = token;
										obj._name = name;
										obj._agl = (token == "OBJECT_SHARED_AGL");
										obj._proxy = false;
										in >> obj._lon >> obj._lat >> obj._elev >> obj._hdg >> obj._pitch >> obj._roll;
										obj._range = range;
										obj._options = opt;
										_objectStaticList.push_back(obj);
                } else if (token == "OBJECT_SIGN" || token == "OBJECT_SIGN_AGL") {
										_Sign sign;
										sign._token = token;
										sign._name = name;
										sign._agl = (token == "OBJECT_SIGN_AGL");
										in >> sign._lon >> sign._lat >> sign._elev >> sign._hdg >> sign._size;
										_signList.push_back(sign);
                } else if (token == "OBJECT_BUILDING_MESH_ROUGH" || token == "OBJECT_BUILDING_MESH_DETAILED") {

										// Only load if building mesh enabled to avoid impacting low-powered systems
										if (_building_mesh_enabled) {
											osg::ref_ptr<SGReaderWriterOptions> opt;
											opt = staticOptions(filePath, options);
											if (SGPath(name).lower_extension() == "ac")
													opt->setInstantiateEffects(true);
											else
													opt->setInstantiateEffects(false);
											_ObjectStatic obj;
											obj._errorLocation = absoluteFileName;
											obj._token = token;
											obj._name = name;
											obj._agl = false;
											obj._proxy = true;
											in >> obj._lon >> obj._lat >> obj._elev >> obj._hdg >> obj._pitch >> obj._roll;

											if (token == "OBJECT_BUILDING_MESH_DETAILED") {
												// Apply a lower LOD range if this is a detailed building
												range = _object_range_detailed;
												double lrand = mt_rand(&seed);
												if      (lrand < 0.1) range = range * 2.0;
												else if (lrand < 0.4) range = range * 1.5;
											}

											obj._range = range;
											obj._options = opt;
											_objectStaticList.push_back(obj);
                    }
                } else {
                    SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                            << ": Unknown token '" << token << "'" );
                }
            } else {
                SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName
                        << ": Unknown token '" << token << "'" );
            }
        }

        return true;
    }

    osg::Node* load(const SGBucket& bucket, const osgDB::Options* opt)
    {
        osg::ref_ptr<SGReaderWriterOptions> options;
        options = SGReaderWriterOptions::copyOrCreate(opt);

        osg::ref_ptr<osg::Group> terrainGroup = new osg::Group;
        terrainGroup->setDataVariance(osg::Object::STATIC);
        terrainGroup->setName("terrain");

        if (_foundBase) {
            for (std::list<_Object>::iterator i = _objectList.begin(); i != _objectList.end(); ++i) {
                osg::ref_ptr<osg::Node> node;
                node = osgDB::readRefNodeFile(i->_name, i->_options.get());
                if (!node.valid()) {
                    SG_LOG(SG_TERRAIN, SG_ALERT, i->_errorLocation << ": Failed to load "
                           << i->_token << " '" << i->_name << "'");
                    continue;
                }
                terrainGroup->addChild(node.get());
            }
        } else {
            SG_LOG(SG_TERRAIN, SG_INFO, "  Generating ocean tile: " << bucket.gen_base_path() << "/" << bucket.gen_index_str());

            osg::Node* node = SGOceanTile(bucket, options->getMaterialLib());
            if (node) {
                node->setName("SGOceanTile");
                terrainGroup->addChild(node);
            } else {
                SG_LOG( SG_TERRAIN, SG_ALERT,
                        "Warning: failed to generate ocean tile!" );
            }
        }

        for (std::list<_ObjectStatic>::iterator i = _objectStaticList.begin(); i != _objectStaticList.end(); ++i) {
            if (!i->_agl)
                continue;
            i->_elev += elevation(*terrainGroup, SGGeod::fromDeg(i->_lon, i->_lat));
        }

        for (std::list<_Sign>::iterator i = _signList.begin(); i != _signList.end(); ++i) {
            if (!i->_agl)
                continue;
            i->_elev += elevation(*terrainGroup, SGGeod::fromDeg(i->_lon, i->_lat));
        }

        if (_objectStaticList.empty() && _signList.empty()) {
            // The simple case, just return the terrain group
            return terrainGroup.release();
        } else {
            osg::PagedLOD* pagedLOD = new osg::PagedLOD;
            pagedLOD->setCenterMode(osg::PagedLOD::USE_BOUNDING_SPHERE_CENTER);
            pagedLOD->setName("pagedObjectLOD");

            // This should be visible in any case.
            // If this is replaced by some lower level of detail, the parent LOD node handles this.
            pagedLOD->addChild(terrainGroup, 0, std::numeric_limits<float>::max());

            // we just need to know about the read file callback that itself holds the data
            osg::ref_ptr<DelayLoadReadFileCallback> readFileCallback = new DelayLoadReadFileCallback;
            readFileCallback->_objectStaticList = _objectStaticList;
            readFileCallback->_signList = _signList;
            readFileCallback->_options = options;
            readFileCallback->_bucket = bucket;
            osg::ref_ptr<osgDB::Options> callbackOptions = new osgDB::Options;
            callbackOptions->setReadFileCallback(readFileCallback.get());
            pagedLOD->setDatabaseOptions(callbackOptions.get());

            pagedLOD->setFileName(pagedLOD->getNumChildren(), "Dummy name - use the stored data in the read file callback");

            // Objects may end up displayed up to 2x the object range.
            pagedLOD->setRange(pagedLOD->getNumChildren(), 0, 2.0 * _object_range_rough + SG_TILE_RADIUS);

            return pagedLOD;
        }
    }

    double _object_range_bare;
    double _object_range_rough;
    double _object_range_detailed;
    bool _building_mesh_enabled;
    bool _foundBase;
    std::list<_Object> _objectList;
    std::list<_ObjectStatic> _objectStaticList;
    std::list<_Sign> _signList;
};

ReaderWriterSTG::ReaderWriterSTG()
{
    supportsExtension("stg", "SimGear stg database format");
}

ReaderWriterSTG::~ReaderWriterSTG()
{
}

const char* ReaderWriterSTG::className() const
{
    return "STG Database reader";
}

osgDB::ReaderWriter::ReadResult
ReaderWriterSTG::readNode(const std::string& fileName, const osgDB::Options* options) const
{
    _ModelBin modelBin;
    SGBucket bucket(bucketIndexFromFileName(fileName));

    // We treat 123.stg different than ./123.stg.
    // The difference is that ./123.stg as well as any absolute path
    // really loads the given stg file and only this.
    // In contrast 123.stg uses the search paths to load a set of stg
    // files spread across the scenery directories.
    if (osgDB::getSimpleFileName(fileName) != fileName) {
        if (!modelBin.read(fileName, options))
            return ReadResult::FILE_NOT_FOUND;
    } else {
        // For stg meta files, we need options for the search path.
        if (!options)
            return ReadResult::FILE_NOT_FOUND;

        SG_LOG(SG_TERRAIN, SG_INFO, "Loading tile " << fileName);

        std::string basePath = bucket.gen_base_path();

        // Stop scanning once an object base is found
        // This is considered a meta file, so apply the scenery path search
        const osgDB::FilePathList& filePathList = options->getDatabasePathList();
        for (osgDB::FilePathList::const_iterator i = filePathList.begin();
             i != filePathList.end() && !modelBin._foundBase; ++i) {
            SGPath objects(*i);
            objects.append("Objects");
            objects.append(basePath);
            objects.append(fileName);
            modelBin.read(objects.local8BitStr(), options);

            SGPath terrain(*i);
            terrain.append("Terrain");
            terrain.append(basePath);
            terrain.append(fileName);
            modelBin.read(terrain.local8BitStr(), options);
        }
    }

    return modelBin.load(bucket, options);
}

}
