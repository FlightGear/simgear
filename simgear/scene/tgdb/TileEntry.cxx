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
#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/scene/material/mat.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/model/ModelRegistry.hxx>
#include <simgear/scene/tgdb/apt_signs.hxx>
#include <simgear/scene/tgdb/obj.hxx>
#include <simgear/scene/tgdb/SGReaderWriterBTGOptions.hxx>

#include "ReaderWriterSTG.hxx"
#include "TileEntry.hxx"

using std::string;
using namespace simgear;

ModelLoadHelper *TileEntry::_modelLoader=0;

namespace {
osgDB::RegisterReaderWriterProxy<ReaderWriterSTG> g_readerWriterSTGProxy;
ModelRegistryCallbackProxy<LoadOnlyCallback> g_stgCallbackProxy("stg");
}

namespace
{
// Update the timestamp on a tile whenever it is in view.

class TileCullCallback : public osg::NodeCallback
{
public:
    TileCullCallback() : _timeStamp(0) {}
    TileCullCallback(const TileCullCallback& tc, const osg::CopyOp& copyOp) :
        NodeCallback(tc, copyOp), _timeStamp(tc._timeStamp)
    {
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
    double getTimeStamp() const { return _timeStamp; }
    void setTimeStamp(double timeStamp) { _timeStamp = timeStamp; }
protected:
    double _timeStamp;
};
}

void TileCullCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    if (nv->getFrameStamp())
        _timeStamp = nv->getFrameStamp()->getReferenceTime();
    traverse(node, nv);
}

double TileEntry::get_timestamp() const
{
    if (_node.valid()) {
        return (dynamic_cast<TileCullCallback*>(_node->getCullCallback()))
            ->getTimeStamp();
    } else
        return DBL_MAX;
}

void TileEntry::set_timestamp(double time_ms)
{
    if (_node.valid()) {
        TileCullCallback* cb
            = dynamic_cast<TileCullCallback*>(_node->getCullCallback());
        if (cb)
            cb->setTimeStamp(time_ms);
    }
}

// Constructor
TileEntry::TileEntry ( const SGBucket& b )
    : tile_bucket( b ),
      tileFileName(b.gen_index_str()),
      _node( new osg::LOD ),
      is_inner_ring(false)
{
    _node->setCullCallback(new TileCullCallback);
    tileFileName += ".stg";
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

static void WorldCoordinate(osg::Matrix& obj_pos, double lat,
                            double lon, double elev, double hdg)
{
    SGGeod geod = SGGeod::fromDegM(lon, lat, elev);
    obj_pos = geod.makeZUpFrame();
    // hdg is not a compass heading, but a counter-clockwise rotation
    // around the Z axis
    obj_pos.preMult(osg::Matrix::rotate(hdg * SGD_DEGREES_TO_RADIANS,
                                        0.0, 0.0, 1.0));
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

bool TileEntry::obj_load(const string& path, osg::Group *geometry, bool is_base,
                         const osgDB::ReaderWriter::Options* options)
{
    osg::Node* node = osgDB::readNodeFile(path, options);
    if (node)
      geometry->addChild(node);

    return node != 0;
}


typedef enum {
    OBJECT,
    OBJECT_SHARED,
    OBJECT_STATIC,
    OBJECT_SIGN,
    OBJECT_RUNWAY_SIGN
} object_type;


// storage class for deferred object processing in TileEntry::load()
struct Object {
    Object(object_type t, const string& token, const SGPath& p,
           std::istream& in)
        : type(t), path(p)
    {
        in >> name;
        if (type != OBJECT)
            in >> lon >> lat >> elev >> hdg;
        in >> ::skipeol;

        if (type == OBJECT)
            SG_LOG(SG_TERRAIN, SG_BULK, "    " << token << "  " << name);
        else
            SG_LOG(SG_TERRAIN, SG_BULK, "    " << token << "  " << name << "  lon=" <<
                    lon << "  lat=" << lat << "  elev=" << elev << "  hdg=" << hdg);
    }
    object_type type;
    string name;
    SGPath path;
    double lon, lat, elev, hdg;
};

// Work in progress... load the tile based entirely by name cuz that's
// what we'll want to do with the database pager.

osg::Node*
TileEntry::loadTileByFileName(const string& fileName,
                              const osgDB::ReaderWriter::Options* options)
{
    std::string index_str = osgDB::getNameLessExtension(fileName);
    index_str = osgDB::getSimpleFileName(index_str);

    long tileIndex;
    {
        std::istringstream idxStream(index_str);
        idxStream >> tileIndex;
    }
    SGBucket tile_bucket(tileIndex);
    const string basePath = tile_bucket.gen_base_path();

    bool found_tile_base = false;

    SGPath object_base;
    vector<const Object*> objects;

    SG_LOG( SG_TERRAIN, SG_INFO, "Loading tile " << index_str );

    osgDB::FilePathList path_list=options->getDatabasePathList();
    // Make sure we find the original filename here...
    std::string filePath = osgDB::getFilePath(fileName);
    if (!filePath.empty())
        path_list.push_front(filePath);

    // scan and parse all files and store information
    for (unsigned int i = 0; i < path_list.size(); i++) {
        // If we found a terrain tile in Terrain/, we have to process the
        // Objects/ dir in the same group, too, before we can stop scanning.
        // FGGlobals::set_fg_scenery() inserts an empty string to path_list
        // as marker.

        if (path_list[i].empty()) {
            if (found_tile_base)
                break;
            else
                continue;
        }

        bool has_base = false;

        SGPath tile_path = path_list[i];
        tile_path.append(basePath);

        SGPath basename = tile_path;
        basename.append( index_str );

        SG_LOG( SG_TERRAIN, SG_DEBUG, "  Trying " << basename.str() );


        // Check for master .stg (scene terra gear) file
        SGPath stg_name = basename;
        stg_name.concat( ".stg" );

        sg_gzifstream in( stg_name.str() );
        if ( !in.is_open() )
            continue;

        while ( ! in.eof() ) {
            string token;
            in >> token;

            if ( token.empty() || token[0] == '#' ) {
               in >> ::skipeol;
               continue;
            }
                            // Load only once (first found)
            if ( token == "OBJECT_BASE" ) {
                string name;
                in >> name >> ::skipws;
                SG_LOG( SG_TERRAIN, SG_BULK, "    " << token << " " << name );

                if (!found_tile_base) {
                    found_tile_base = true;
                    has_base = true;

                    object_base = tile_path;
                    object_base.append(name);

                } else
                    SG_LOG(SG_TERRAIN, SG_BULK, "    (skipped)");

                            // Load only if base is not in another file
            } else if ( token == "OBJECT" ) {
                if (!found_tile_base || has_base)
                    objects.push_back(new Object(OBJECT, token, tile_path, in));
                else {
                    string name;
                    in >> name >> ::skipeol;
                    SG_LOG(SG_TERRAIN, SG_BULK, "    " << token << "  "
                            << name << "  (skipped)");
                }

                            // Always OK to load
            } else if ( token == "OBJECT_STATIC" ) {
                objects.push_back(new Object(OBJECT_STATIC, token, tile_path, in));

            } else if ( token == "OBJECT_SHARED" ) {
                objects.push_back(new Object(OBJECT_SHARED, token, tile_path, in));

            } else if ( token == "OBJECT_SIGN" ) {
                objects.push_back(new Object(OBJECT_SIGN, token, tile_path, in));

            } else if ( token == "OBJECT_RUNWAY_SIGN" ) {
                objects.push_back(new Object(OBJECT_RUNWAY_SIGN, token, tile_path, in));

            } else {
                SG_LOG( SG_TERRAIN, SG_DEBUG,
                        "Unknown token '" << token << "' in " << stg_name.str() );
                in >> ::skipws;
            }
        }
    }

    const SGReaderWriterBTGOptions* btgOpt;
    btgOpt = dynamic_cast<const SGReaderWriterBTGOptions *>(options);
    osg::ref_ptr<SGReaderWriterBTGOptions> opt;
    if (btgOpt)
        opt = new SGReaderWriterBTGOptions(*btgOpt);
    else
        opt = new SGReaderWriterBTGOptions;

    // obj_load() will generate ground lighting for us ...
    osg::Group* new_tile = new osg::Group;

    if (found_tile_base) {
        // load tile if found ...
        opt->setCalcLights(true);
        obj_load( object_base.str(), new_tile, true, opt);

    } else {
        // ... or generate an ocean tile on the fly
        SG_LOG(SG_TERRAIN, SG_INFO, "  Generating ocean tile");
        if ( !SGGenTile( path_list[0], tile_bucket,
                        opt->getMatlib(), new_tile ) ) {
            SG_LOG( SG_TERRAIN, SG_ALERT,
                    "Warning: failed to generate ocean tile!" );
        }
    }


    // now that we have a valid center, process all the objects
    for (unsigned int j = 0; j < objects.size(); j++) {
        const Object *obj = objects[j];

        if (obj->type == OBJECT) {
            SGPath custom_path = obj->path;
            custom_path.append( obj->name );
            opt->setCalcLights(true);
            obj_load( custom_path.str(), new_tile, false, opt);

        } else if (obj->type == OBJECT_SHARED || obj->type == OBJECT_STATIC) {
            // object loading is deferred to main render thread,
            // but lets figure out the paths right now.
            SGPath custom_path;
            if ( obj->type == OBJECT_STATIC ) {
                custom_path = obj->path;
            } else {
                // custom_path = globals->get_fg_root();
            }
            custom_path.append( obj->name );

            osg::Matrix obj_pos;
            WorldCoordinate( obj_pos, obj->lat, obj->lon, obj->elev, obj->hdg );

            osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
            obj_trans->setDataVariance(osg::Object::STATIC);
            obj_trans->setMatrix( obj_pos );

            // wire as much of the scene graph together as we can
            new_tile->addChild( obj_trans );

            osg::Node* model = 0;
            if(_modelLoader)
                model = _modelLoader->loadTileModel(custom_path.str(),
                                                    obj->type == OBJECT_SHARED);
            if (model)
                obj_trans->addChild(model);
        } else if (obj->type == OBJECT_SIGN || obj->type == OBJECT_RUNWAY_SIGN) {
            // load the object itself
            SGPath custom_path = obj->path;
            custom_path.append( obj->name );

            osg::Matrix obj_pos;
            WorldCoordinate( obj_pos, obj->lat, obj->lon, obj->elev, obj->hdg );

            osg::MatrixTransform *obj_trans = new osg::MatrixTransform;
            obj_trans->setDataVariance(osg::Object::STATIC);
            obj_trans->setMatrix( obj_pos );

            osg::Node *custom_obj = 0;
            if (obj->type == OBJECT_SIGN)
                custom_obj = SGMakeSign(opt->getMatlib(), custom_path.str(), obj->name);
            else
                custom_obj = SGMakeRunwaySign(opt->getMatlib(), custom_path.str(), obj->name);

            // wire the pieces together
            if ( custom_obj != NULL ) {
                obj_trans -> addChild( custom_obj );
            }
            new_tile->addChild( obj_trans );

        }
        delete obj;
    }
    return new_tile;
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

