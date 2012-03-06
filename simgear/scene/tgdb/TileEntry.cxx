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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <string>
#include <sstream>
#include <istream>

#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osg/Math>
#include <osg/NodeCallback>

#include <osgDB/FileNameUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>
#include <osgDB/Registry>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sgstream.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/tgdb/apt_signs.hxx>
#include <simgear/scene/tgdb/obj.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "ReaderWriterSPT.hxx"
#include "ReaderWriterSTG.hxx"
#include "SGOceanTile.hxx"
#include "TileEntry.hxx"

using std::string;
using namespace simgear;

ModelLoadHelper *TileEntry::_modelLoader=0;

namespace {
osgDB::RegisterReaderWriterProxy<ReaderWriterSTG> g_readerWriterSTGProxy;
ModelRegistryCallbackProxy<LoadOnlyCallback> g_stgCallbackProxy("stg");

osgDB::RegisterReaderWriterProxy<ReaderWriterSPT> g_readerWriterSPTProxy;
ModelRegistryCallbackProxy<LoadOnlyCallback> g_sptCallbackProxy("spt");
}


static SGBucket getBucketFromFileName(const std::string& fileName)
{
    std::istringstream ss(osgDB::getNameLessExtension(fileName));
    long index;
    ss >> index;
    if (ss.fail())
        return SGBucket();
    return SGBucket(index);
}

// Constructor
TileEntry::TileEntry ( const SGBucket& b )
    : tile_bucket( b ),
      tileFileName(b.gen_index_str()),
      _node( new osg::LOD ),
      _priority(-FLT_MAX),
      _current_view(false),
      _time_expired(-1.0)
{
    tileFileName += ".stg";
    _node->setName(tileFileName);
    // Give a default LOD range so that traversals that traverse
    // active children (like the groundcache lookup) will work before
    // tile manager has had a chance to update this node.
    _node->setRange(0, 0.0, 10000.0);
}

TileEntry::TileEntry( const TileEntry& t )
: tile_bucket( t.tile_bucket ),
  tileFileName(t.tileFileName),
  _node( new osg::LOD ),
  _priority(t._priority),
  _current_view(t._current_view),
  _time_expired(t._time_expired)
{
    _node->setName(tileFileName);
    // Give a default LOD range so that traversals that traverse
    // active children (like the groundcache lookup) will work before
    // tile manager has had a chance to update this node.
    _node->setRange(0, 0.0, 10000.0);
}

// Destructor
TileEntry::~TileEntry ()
{
}

// Update the ssg transform node for this tile so it can be
// properly drawn relative to our (0,0,0) point
void TileEntry::prep_ssg_node(float vis) {
    if (!is_loaded())
        return;
    // visibility can change from frame to frame so we update the
    // range selector cutoff's each time.
    float bounding_radius = _node->getChild(0)->getBound().radius();
    _node->setRange( 0, 0, vis + bounding_radius );
}

osg::Node*
TileEntry::loadTileByFileName(const string& fileName,
                              const osgDB::Options* options)
{
    SG_LOG(SG_TERRAIN, SG_INFO, "Loading tile " << fileName);

    // Space for up to two stg file names.
    // There is usually one in the Terrain and one in the Objects subdirectory.
    std::string absoluteFileName[2];

    // We treat 123.stg different than ./123.stg.
    // The difference is that ./123.stg as well as any absolute path
    // really loads the given stg file and only this.
    // In contrast 123.stg uses the search paths to load a set of stg
    // files spread across the scenery directories.
    std::string simpleFileName = osgDB::getSimpleFileName(fileName);
    SGBucket bucket = getBucketFromFileName(simpleFileName);
    bool file_mode = false;
    if (simpleFileName == fileName && options) {
        // This is considered a meta file, so apply the scenery path search
        const osgDB::FilePathList& filePathList = options->getDatabasePathList();
        std::string basePath = bucket.gen_base_path();
        for (osgDB::FilePathList::const_iterator i = filePathList.begin();
             i != filePathList.end(); ++i) {
            SGPath terrain(*i);
            terrain.append("Terrain");
            terrain.append(basePath);
            terrain.append(simpleFileName);

            SGPath objects(*i);
            objects.append("Objects");
            objects.append(basePath);
            objects.append(simpleFileName);

            if (terrain.isFile() || objects.isFile()) {
                absoluteFileName[0] = terrain.str();
                absoluteFileName[1] = objects.str();
                break;
            }
        }
    } else {
        // This is considered a real existing file.
        // We still apply the search path algorithms for relative files.
        absoluteFileName[0] = osgDB::findDataFile(fileName, options);
        // Do not generate an ocean tile if we have no btg
        file_mode = true;
    }

    std::string fg_root = options->getPluginStringData("SimGear::FG_ROOT");
    osg::ref_ptr<SGReaderWriterOptions> opt;
    opt = SGReaderWriterOptions::copyOrCreate(options);

    bool found_tile_base = false;
    osg::ref_ptr<osg::Group> group = new osg::Group;
    for (unsigned i = 0; i < 2; ++i) {

        if (absoluteFileName[i].empty())
            continue;

        sg_gzifstream in( absoluteFileName[i] );
        if ( !in.is_open() )
            continue;

        SG_LOG(SG_TERRAIN, SG_INFO, "Loading stg file " << absoluteFileName[i]);

        std::string filePath = osgDB::getFilePath(absoluteFileName[i]);

        osg::ref_ptr<SGReaderWriterOptions> staticOptions;
        staticOptions = SGReaderWriterOptions::copyOrCreate(options);
        staticOptions->getDatabasePathList().clear();
        staticOptions->getDatabasePathList().push_back(filePath);
        staticOptions->setObjectCacheHint(osgDB::Options::CACHE_NONE);
       
        osg::ref_ptr<SGReaderWriterOptions> sharedOptions;
        sharedOptions = SGReaderWriterOptions::copyOrCreate(options);
        sharedOptions->getDatabasePathList().clear();

        SGPath path = filePath;
        path.append(".."); path.append(".."); path.append("..");
        sharedOptions->getDatabasePathList().push_back(path.str());
        sharedOptions->getDatabasePathList().push_back(fg_root);

        bool has_base = false;
        while ( ! in.eof() ) {
            std::string token;
            in >> token;

            // No comment
            if ( token.empty() || token[0] == '#' ) {
                in >> ::skipeol;
                continue;
            }

            // Then there is always a name
            std::string name;
            in >> name;

            SGPath path = filePath;
            path.append(name);

            osg::ref_ptr<osg::Node> node;
            if ( token == "OBJECT_BASE" ) {
                // Load only once (first found)
                SG_LOG( SG_TERRAIN, SG_BULK, "    " << token << " " << name );

                if (!found_tile_base) {
                    found_tile_base = true;
                    has_base = true;
                    
                    node = osgDB::readRefNodeFile(path.str(),
                                                  staticOptions.get());

                    if (!node.valid()) {
                        SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName[i]
                                << ": Failed to load OBJECT_BASE '"
                                << name << "'" );
                    }
                } else
                    SG_LOG(SG_TERRAIN, SG_BULK, "    (skipped)");

            } else if ( token == "OBJECT" ) {
                // Load only if base is not in another file
                if (!found_tile_base || has_base) {
                    node = osgDB::readRefNodeFile(path.str(),
                                                  staticOptions.get());

                    if (!node.valid()) {
                        SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName[i]
                                << ": Failed to load OBJECT '"
                                << name << "'" );
                    }
                } else {
                    SG_LOG(SG_TERRAIN, SG_BULK, "    " << token << "  "
                           << name << "  (skipped)");
                }

            } else {
                double lon, lat, elev, hdg;
                in >> lon >> lat >> elev >> hdg;
                
                // Always OK to load
                if ( token == "OBJECT_STATIC" ) {
                    /// Hmm, the findDataFile should happen downstream
                    std::string absName = osgDB::findDataFile(name,
                                               staticOptions.get());
                    if(_modelLoader) {
                        node = _modelLoader->loadTileModel(absName, false);
                    } else {
                        node = osgDB::readRefNodeFile(absName,
                                               staticOptions.get());
                    }

                    if (!node.valid()) {
                        SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName[i]
                                << ": Failed to load OBJECT_STATIC '"
                                << name << "'" );
                    }
                    
                } else if ( token == "OBJECT_SHARED" ) {
                    if(_modelLoader) {
                        node = _modelLoader->loadTileModel(name, true);
                    } else {
                        /// Hmm, the findDataFile should happen in the downstream readers
                        std::string absName = osgDB::findDataFile(name,
                                                   sharedOptions.get());
                        node = osgDB::readRefNodeFile(absName,
                                                   sharedOptions.get());
                    }

                    if (!node.valid()) {
                        SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName[i]
                                << ": Failed to load OBJECT_SHARED '"
                                << name << "'" );
                    }
                    
                } else if ( token == "OBJECT_SIGN" ) {
                    node = SGMakeSign(opt->getMaterialLib(), name);
                    
                } else if ( token == "OBJECT_RUNWAY_SIGN" ) {
                    node = SGMakeRunwaySign(opt->getMaterialLib(), name);
                    
                } else {
                    SG_LOG( SG_TERRAIN, SG_ALERT, absoluteFileName[i]
                            << ": Unknown token '" << token << "'" );
                }

                if (node.valid() && token != "OBJECT") {
                    osg::Matrix matrix;
                    matrix = makeZUpFrame(SGGeod::fromDegM(lon, lat, elev));
                    matrix.preMultRotate(osg::Quat(SGMiscd::deg2rad(hdg),
                                                   osg::Vec3(0, 0, 1)));
                    
                    osg::MatrixTransform* matrixTransform;
                    matrixTransform = new osg::MatrixTransform(matrix);
                    matrixTransform->setDataVariance(osg::Object::STATIC);
                    matrixTransform->addChild(node.get());
                    node = matrixTransform;
                }
            }

            if (node.valid())
                group->addChild(node.get());

            in >> ::skipeol;
        }
    }

    if (!found_tile_base && !file_mode) {
        // ... or generate an ocean tile on the fly
        SG_LOG(SG_TERRAIN, SG_INFO, "  Generating ocean tile");

        osg::Node* node = SGOceanTile(bucket, opt->getMaterialLib());
        if ( node ) {
            group->addChild(node);
        } else {
            SG_LOG( SG_TERRAIN, SG_ALERT,
                    "Warning: failed to generate ocean tile!" );
        }
    }

    return group.release();
}

void
TileEntry::addToSceneGraph(osg::Group *terrain_branch)
{
    terrain_branch->addChild( _node.get() );

    SG_LOG( SG_TERRAIN, SG_DEBUG,
            "connected a tile into scene graph.  _node = "
            << _node.get() );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "num parents now = "
            << _node->getNumParents() );
}


void
TileEntry::removeFromSceneGraph()
{
    SG_LOG( SG_TERRAIN, SG_DEBUG, "disconnecting TileEntry nodes" );

    if (! is_loaded()) {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "removing a not-fully loaded tile!" );
    } else {
        SG_LOG( SG_TERRAIN, SG_DEBUG, "removing a fully loaded tile!  _node = " << _node.get() );
    }

    // find the nodes branch parent
    if ( _node->getNumParents() > 0 ) {
        // find the first parent (should only be one)
        osg::Group *parent = _node->getParent( 0 ) ;
        if( parent ) {
            parent->removeChild( _node.get() );
        }
    }
}

void
TileEntry::refresh()
{
    osg::Group *parent = NULL;
    // find the nodes branch parent
    if ( _node->getNumParents() > 0 ) {
        // find the first parent (should only be one)
        parent = _node->getParent( 0 ) ;
        if( parent ) {
            parent->removeChild( _node.get() );
        }
    }
    _node = new osg::LOD;
    if (parent)
        parent->addChild(_node.get());
}
