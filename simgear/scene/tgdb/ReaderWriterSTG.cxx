// ReaderWriterSTG.cxx -- routines to handle a scenery tile
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

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/math/sg_random.h>

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/scene/util/OptionsReadFileCallback.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/QuadTreeBuilder.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/tgdb/apt_signs.hxx>
#include <simgear/scene/tgdb/obj.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/tgdb/SGBuildingBin.hxx>

#include "SGOceanTile.hxx"

#define BUILDING_ROUGH "OBJECT_BUILDING_MESH_ROUGH"
#define BUILDING_DETAILED "OBJECT_BUILDING_MESH_DETAILED"
#define ROAD_ROUGH "OBJECT_ROAD_ROUGH"
#define ROAD_DETAILED "OBJECT_ROAD_DETAILED"
#define RAILWAY_ROUGH "OBJECT_RAILWAY_ROUGH"
#define RAILWAY_DETAILED "OBJECT_RAILWAY_DETAILED"
#define BUILDING_LIST "BUILDING_LIST"

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

/**
 * callback per STG token, with access synced by a lock.
 */
using TokenCallbackMap = std::map<std::string,ReaderWriterSTG::STGObjectCallback>;
static TokenCallbackMap globalStgObjectCallbacks = {};
static OpenThreads::Mutex globalStgObjectCallbackLock;

struct ReaderWriterSTG::_ModelBin {
    struct _Object {
        SGPath _errorLocation;
        std::string _token;
        std::string _name;
        osg::ref_ptr<SGReaderWriterOptions> _options;
    };
    struct _ObjectStatic {
        _ObjectStatic() : _agl(false), _proxy(false), _lon(0), _lat(0), _elev(0), _hdg(0), _pitch(0), _roll(0), _range(SG_OBJECT_RANGE_ROUGH) { }
        SGPath _errorLocation;
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
        SGPath _errorLocation;
        std::string _token;
        std::string _name;
        bool _agl;
        double _lon, _lat, _elev;
        double _hdg;
        int _size;
    };
    struct _BuildingList {
      _BuildingList() : _lon(0), _lat(0), _elev(0) { }
      std::string _filename;
      std::string _material_name;
      double _lon, _lat, _elev;
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
                proxy->setDatabaseOptions(o._options);

                // Give the node some values so the Quadtree builder has
                // a BoundingBox to work with prior to the model being loaded.
                proxy->setCenter(osg::Vec3f(0.0f,0.0f,0.0f));
                proxy->setRadius(10);
                proxy->setCenterMode(osg::ProxyNode::UNION_OF_BOUNDING_SPHERE_AND_USER_DEFINED);
                node = proxy;
            } else {
                ErrorReportContext ec("terrain-stg", o._errorLocation.utf8Str());
#if OSG_VERSION_LESS_THAN(3,4,0)
                node = osgDB::readNodeFile(o._name, o._options.get());
#else
                node = osgDB::readRefNodeFile(o._name, o._options.get());
#endif
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
            ErrorReportContext ec("terrain-bucket", _bucket.gen_index_str());

            STGObjectsQuadtree quadtree((GetModelLODCoord()), (AddModelLOD()));
            quadtree.buildQuadTree(_objectStaticList.begin(), _objectStaticList.end());
            osg::ref_ptr<osg::Group> group = quadtree.getRoot();
            string group_name = string("STG-group-A ").append(_bucket.gen_index_str());
            group->setName(group_name);
            group->setDataVariance(osg::Object::STATIC);

            simgear::AirportSignBuilder signBuilder(_options->getMaterialLib(), _bucket.get_center());
            for (std::list<_Sign>::iterator i = _signList.begin(); i != _signList.end(); ++i)
                signBuilder.addSign(SGGeod::fromDegM(i->_lon, i->_lat, i->_elev), i->_hdg, i->_name, i->_size);
            if (signBuilder.getSignsGroup())
                group->addChild(signBuilder.getSignsGroup());

            if (!_buildingList.empty()) {
                SGMaterialLibPtr matlib = _options->getMaterialLib();
                bool useVBOs = (_options->getPluginStringData("SimGear::USE_VBOS") == "ON");

                if (!matlib) {
                    SG_LOG( SG_TERRAIN, SG_ALERT, "Unable to get materials definition for buildings");
                } else {
                    for (const auto& b : _buildingList) {
                        // Build buildings for each list of buildings
                        SGGeod geodPos = SGGeod::fromDegM(b._lon, b._lat, 0.0);
                        SGSharedPtr<SGMaterial> mat = matlib->find(b._material_name, geodPos);

                        // trying to avoid crash on null material, see:
                        // https://sentry.io/organizations/flightgear/issues/1867075869
                        if (!mat) {
                            SG_LOG(SG_TERRAIN, SG_ALERT, "Building specifies unknown material: " << b._material_name);
                            continue;
                        }

                        const auto path = SGPath(b._filename);
                        SGBuildingBin* buildingBin = new SGBuildingBin(path, mat, useVBOs);

                        SGBuildingBinList bbList;
                        bbList.push_back(buildingBin);

                        osg::MatrixTransform* matrixTransform;
                        matrixTransform = new osg::MatrixTransform(makeZUpFrame(SGGeod::fromDegM(b._lon, b._lat, b._elev)));
                        matrixTransform->setName("rotateBuildings");
                        matrixTransform->setDataVariance(osg::Object::STATIC);
                        matrixTransform->addChild(createRandomBuildings(bbList, osg::Matrix::identity(), _options));
                        group->addChild(matrixTransform);

                        std::for_each(bbList.begin(), bbList.end(), [](SGBuildingBin* bb) {
                            delete bb;
                        });
                    }
                }
            }

            return group.release();
        }

        mt _seed;
        std::list<_ObjectStatic> _objectStaticList;
        std::list<_Sign> _signList;
        std::list<_BuildingList> _buildingList;

        /// The original options to use for this bunch of models
        osg::ref_ptr<SGReaderWriterOptions> _options;
        SGBucket _bucket;
    };

    _ModelBin() :
        _object_range_bare(SG_OBJECT_RANGE_BARE),
        _object_range_rough(SG_OBJECT_RANGE_ROUGH),
        _object_range_detailed(SG_OBJECT_RANGE_DETAILED),
        _foundBase(false)
    { }

    SGReaderWriterOptions* sharedOptions(const std::string& filePath, const osgDB::Options* options)
    {
        osg::ref_ptr<SGReaderWriterOptions> sharedOptions;
        sharedOptions = SGReaderWriterOptions::copyOrCreate(options);
        sharedOptions->getDatabasePathList().clear();

        SGPath path = filePath;
        path.append(".."); path.append(".."); path.append("..");
        sharedOptions->getDatabasePathList().push_back(path.utf8Str());

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

    void checkInsideBucket(const SGPath& absoluteFileName, float lon, float lat) {
        SGBucket bucket = bucketIndexFromFileName(absoluteFileName.file_base().c_str());
        SGBucket correctBucket = SGBucket( SGGeod::fromDeg(lon, lat));

        if (bucket != correctBucket) {
          SG_LOG( SG_TERRAIN, SG_DEV_WARN, absoluteFileName
                  << ": Object at " << lon << ", " << lat <<
                  " in incorrect bucket (" << bucket << ") - should be in " <<
                  correctBucket.gen_index_str() << " (" << correctBucket << ")");
        }
    }

    bool read(const SGPath& absoluteFileName, const osgDB::Options* options)
    {
        if (!absoluteFileName.exists()) {
            return false;
        }

        sg_gzifstream stream(absoluteFileName);
        if (!stream.is_open()) {
            return false;
        }

        // starting with 2018.3 we will use deltas rather than absolutes as it is more intuitive for the user
        // and somewhat easier to visualise
        double detailedRange = atof(options->getPluginStringData("SimGear::LOD_RANGE_DETAILED").c_str());
        double bareRangeDelta = atof(options->getPluginStringData("SimGear::LOD_RANGE_BARE").c_str());
        double roughRangeDelta = atof(options->getPluginStringData("SimGear::LOD_RANGE_ROUGH").c_str());

        // Determine object ranges.  Mesh size of 2000mx2000m needs to be accounted for.
        _object_range_detailed = 1414.0f + detailedRange;
        _object_range_bare = _object_range_detailed + bareRangeDelta;
        _object_range_rough = _object_range_detailed + roughRangeDelta;

        SG_LOG(SG_TERRAIN, SG_INFO, "Loading stg file " << absoluteFileName);

        std::string filePath = osgDB::getFilePath(absoluteFileName.utf8Str());

        // Bucket provides a consistent seed
        // so we have consistent set of pseudo-random numbers for each STG file
        mt seed;
        int bucket = atoi(absoluteFileName.file_base().c_str());
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
                    obj._name = path.utf8Str();
                    obj._options = staticOptions(filePath, options);
                    _objectList.push_back(obj);
                }

            } else if (token == "OBJECT") {
                if (!onlyAirports || isAirportBtg(name)) {
                    _Object obj;
                    obj._errorLocation = absoluteFileName;
                    obj._token = token;
                    obj._name = path.utf8Str();
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
                                        opt->addErrorContext("terrain-stg", absoluteFileName.utf8Str());
                                        obj._errorLocation = absoluteFileName;
                                        obj._token = token;
                                        obj._name = name;
                                        obj._agl = (token == "OBJECT_STATIC_AGL");
                                        obj._proxy = true;
                                        in >> obj._lon >> obj._lat >> obj._elev >> obj._hdg >> obj._pitch >> obj._roll;
                                        obj._range = range;
                                        obj._options = opt;
                    checkInsideBucket(absoluteFileName, obj._lon, obj._lat);
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
                    checkInsideBucket(absoluteFileName, obj._lon, obj._lat);
                    _objectStaticList.push_back(obj);
                } else if (token == "OBJECT_SIGN" || token == "OBJECT_SIGN_AGL") {
										_Sign sign;
										sign._token = token;
										sign._name = name;
										sign._agl = (token == "OBJECT_SIGN_AGL");
										in >> sign._lon >> sign._lat >> sign._elev >> sign._hdg >> sign._size;
										_signList.push_back(sign);
                } else if (token == BUILDING_ROUGH || token == BUILDING_DETAILED ||
                           token == ROAD_ROUGH     || token == ROAD_DETAILED     ||
                           token == RAILWAY_ROUGH  || token == RAILWAY_DETAILED)   {
                    osg::ref_ptr<SGReaderWriterOptions> opt;
                    opt = staticOptions(filePath, options);
                    _ObjectStatic obj;

                    opt->setInstantiateEffects(false);
                    opt->addErrorContext("terrain-stg", absoluteFileName.utf8Str());

                    if (SGPath(name).lower_extension() == "ac") {
                      // Generate material/Effects lookups for raw models, i.e.
                      // those not wrapped in an XML while will include Effects
                      opt->setInstantiateMaterialEffects(true);

                      if (token == BUILDING_ROUGH || token == BUILDING_DETAILED) {
                        opt->setMaterialName("OSM_Building");
                      } else if (token == ROAD_ROUGH || token == ROAD_DETAILED) {
                        opt->setMaterialName("OSM_Road");
                      } else if (token == RAILWAY_ROUGH || token == RAILWAY_DETAILED) {
                        opt->setMaterialName("OSM_Railway");
                      } else {
                        // Programming error.  If we get here then someone has added a verb to the list of
                        // tokens above but not in this set of if-else statements.
                        SG_LOG(SG_TERRAIN, SG_ALERT, "Programming Error - STG token without material branch");
                      }
                    }

                    obj._errorLocation = absoluteFileName;
                    obj._token = token;
                    obj._name = name;
                    obj._agl = false;
                    obj._proxy = true;
                    in >> obj._lon >> obj._lat >> obj._elev >> obj._hdg >> obj._pitch >> obj._roll;

                    opt->setLocation(obj._lon, obj._lat);
                    if (token == BUILDING_DETAILED || token == ROAD_DETAILED || token == RAILWAY_DETAILED ) {
											// Apply a lower LOD range if this is a detailed mesh
											range = _object_range_detailed;
											double lrand = mt_rand(&seed);
											if      (lrand < 0.1) range = range * 2.0;
											else if (lrand < 0.4) range = range * 1.5;
										}

                    obj._range = range;

                    obj._options = opt;
                    checkInsideBucket(absoluteFileName, obj._lon, obj._lat);
                    _objectStaticList.push_back(obj);
                } else if (token == BUILDING_LIST) {
                  _BuildingList buildinglist;
                  buildinglist._filename = path.utf8Str();
                  in >> buildinglist._material_name >> buildinglist._lon >> buildinglist._lat >> buildinglist._elev;
                  checkInsideBucket(absoluteFileName, buildinglist._lon, buildinglist._lat);
                  _buildingListList.push_back(buildinglist);
                  //SG_LOG(SG_TERRAIN, SG_ALERT, "Building list: " << buildinglist._filename << " " << buildinglist._material_name << " " << buildinglist._lon << " " << buildinglist._lat);
                } else {
                    // Check registered callback for token. Keep lock until callback completed to make sure it will not be
                    // executed after a thread successfully executed removeSTGObjectHandler()
                    {
                        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(globalStgObjectCallbackLock);
                        STGObjectCallback callback = globalStgObjectCallbacks[token];

                        if (callback != nullptr) {
                            _ObjectStatic obj;
                            // pitch and roll are not common, so passed in "restofline" only
                            in >> obj._lon >> obj._lat >> obj._elev >> obj._hdg;
                            string_list restofline;
                            std::string buf;
                            while (in >> buf) {
                                restofline.push_back(buf);
                            }
                            callback(token,name, SGGeod::fromDegM(obj._lon, obj._lat, obj._elev), obj._hdg,restofline);
                        } else {
                            // SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName << ": Unknown token '" << token << "'" );
                            simgear::reportFailure(simgear::LoadFailure::Misconfigured, simgear::ErrorCode::BTGLoad,
                                                   "Unknown STG token:" + token, absoluteFileName);
                        }
                    }
                }
            }
        }

        return true;
    }

    osg::Node* load(const SGBucket& bucket, const osgDB::Options* opt)
    {
        osg::ref_ptr<SGReaderWriterOptions> options;
        options = SGReaderWriterOptions::copyOrCreate(opt);
        float pagedLODExpiry = atoi(options->getPluginStringData("SimGear::PAGED_LOD_EXPIRY").c_str());

        osg::ref_ptr<osg::Group> terrainGroup = new osg::Group;
        terrainGroup->setDataVariance(osg::Object::STATIC);
        std::string terrain_name = string("terrain ").append(bucket.gen_index_str());
        terrainGroup->setName(terrain_name);

        simgear::ErrorReportContext ec{"terrain-bucket", bucket.gen_index_str()};

        if (_foundBase) {
            for (auto stgObject : _objectList) {
                osg::ref_ptr<osg::Node> node;
                simgear::ErrorReportContext ec("terrain-stg", stgObject._errorLocation.utf8Str());

#if OSG_VERSION_LESS_THAN(3,4,0)
                node = osgDB::readNodeFile(stgObject._name, stgObject._options.get());
#else
                node = osgDB::readRefNodeFile(stgObject._name, stgObject._options.get());
#endif
                if (!node.valid()) {
                    SG_LOG(SG_TERRAIN, SG_ALERT, stgObject._errorLocation << ": Failed to load "
                           << stgObject._token << " '" << stgObject._name << "'");
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

        if (_objectStaticList.empty() && _signList.empty() && (_buildingListList.size() == 0)) {
            // The simple case, just return the terrain group
            return terrainGroup.release();
        } else {
            osg::PagedLOD* pagedLOD = new osg::PagedLOD;
            pagedLOD->setCenterMode(osg::PagedLOD::USE_BOUNDING_SPHERE_CENTER);
            std::string name = string("pagedObjectLOD ").append(bucket.gen_index_str());
            pagedLOD->setName(name);

            // This should be visible in any case.
            // If this is replaced by some lower level of detail, the parent LOD node handles this.
            pagedLOD->addChild(terrainGroup, 0, std::numeric_limits<float>::max());
            pagedLOD->setMinimumExpiryTime(0, pagedLODExpiry);

            // we just need to know about the read file callback that itself holds the data
            osg::ref_ptr<DelayLoadReadFileCallback> readFileCallback = new DelayLoadReadFileCallback;
            readFileCallback->_objectStaticList = _objectStaticList;
            readFileCallback->_buildingList = _buildingListList;
            readFileCallback->_signList = _signList;
            readFileCallback->_options = options;
            readFileCallback->_bucket = bucket;
            osg::ref_ptr<osgDB::Options> callbackOptions = new osgDB::Options;
            callbackOptions->setReadFileCallback(readFileCallback.get());
            pagedLOD->setDatabaseOptions(callbackOptions.get());

            pagedLOD->setFileName(pagedLOD->getNumChildren(), "Dummy name - use the stored data in the read file callback");

            // Objects may end up displayed up to 2x the object range.
            pagedLOD->setRange(pagedLOD->getNumChildren(), 0, 2.0 * _object_range_rough);
            pagedLOD->setMinimumExpiryTime(pagedLOD->getNumChildren(), pagedLODExpiry);
            pagedLOD->setRadius(SG_TILE_RADIUS);
            SG_LOG( SG_TERRAIN, SG_DEBUG, "Tile " << bucket.gen_index_str() << " PagedLOD Center: " << pagedLOD->getCenter().x() << "," << pagedLOD->getCenter().y() << "," << pagedLOD->getCenter().z() );
            SG_LOG( SG_TERRAIN, SG_DEBUG, "Tile " << bucket.gen_index_str() << " PagedLOD Range: " << (2.0 * _object_range_rough));
            SG_LOG( SG_TERRAIN, SG_DEBUG, "Tile " << bucket.gen_index_str() << " PagedLOD Radius: " << SG_TILE_RADIUS);
            return pagedLOD;
        }
    }

    double _object_range_bare;
    double _object_range_rough;
    double _object_range_detailed;
    bool _foundBase;
    std::list<_Object> _objectList;
    std::list<_ObjectStatic> _objectStaticList;
    std::list<_Sign> _signList;
    std::list<_BuildingList> _buildingListList;
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
    simgear::ErrorReportContext ec("terrain-bucket", bucket.gen_index_str());

    // We treat 123.stg different than ./123.stg.
    // The difference is that ./123.stg as well as any absolute path
    // really loads the given stg file and only this.
    // In contrast 123.stg uses the search paths to load a set of stg
    // files spread across the scenery directories.
    if (osgDB::getSimpleFileName(fileName) != fileName) {
        simgear::ErrorReportContext ec("terrain-stg", fileName);
        if (!modelBin.read(fileName, options))
            return ReadResult::FILE_NOT_FOUND;
    }

    // For stg meta files, we need options for the search path.
    if (!options) {
        return ReadResult::FILE_NOT_FOUND;
    }

    const auto sgOpts = dynamic_cast<const SGReaderWriterOptions*>(options);
    if (!sgOpts || sgOpts->getSceneryPathSuffixes().empty()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Loading tile " << fileName << ", no scenery path suffixes were configured so giving up");
        return ReadResult::FILE_NOT_FOUND;
    }

    SG_LOG(SG_TERRAIN, SG_INFO, "Loading tile " << fileName);

    std::string basePath = bucket.gen_base_path();

    // Stop scanning paths once an object base is found
    // But do load all STGs at the same level (i.e from the same scenery path)
    const osgDB::FilePathList& filePathList = options->getDatabasePathList();
    for (auto path : filePathList) {
        if (modelBin._foundBase) {
            break;
        }

        SGPath base(path);
        // check for non-suffixed file, and warn.
        SGPath pathWithoutSuffix = base / basePath / fileName;
        if (pathWithoutSuffix.exists()) {
            SG_LOG(SG_TERRAIN, SG_ALERT, "Found scenery file " << pathWithoutSuffix << " in scenery path " << path
                   << ".\nScenery paths without type subdirectories are no longer supported, please move thse files\n"
                   << "into a an appropriate subdirectory, for example:" << base / "Objects" / basePath / fileName);
        }

        for (auto suffix : sgOpts->getSceneryPathSuffixes()) {
            const auto p = base / suffix / basePath / fileName;
            simgear::ErrorReportContext ec("terrain-stg", p.utf8Str());
            modelBin.read(p, options);
        }
    }

    return modelBin.load(bucket, options);
}


void ReaderWriterSTG::setSTGObjectHandler(const std::string &token, STGObjectCallback callback)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(globalStgObjectCallbackLock);
    globalStgObjectCallbacks[token] = callback;
}

void ReaderWriterSTG::removeSTGObjectHandler(const std::string &token, STGObjectCallback callback)
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(globalStgObjectCallbackLock);
    globalStgObjectCallbacks.erase(token);
}
}
