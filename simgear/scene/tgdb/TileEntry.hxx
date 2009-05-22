// tileentry.hxx -- routines to handle an individual scenery tile
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
// $Id$


#ifndef _TILEENTRY_HXX
#define _TILEENTRY_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <vector>
#include <string>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/model/placementtrans.hxx>

#include <osg/ref_ptr>
#include <osgDB/ReaderWriter>
#include <osg/Group>
#include <osg/LOD>

#if defined( sgi )
#include <strings.h>
#endif

using std::string;
using std::vector;

namespace simgear {

class ModelLoadHelper;

/**
 * A class to encapsulate everything we need to know about a scenery tile.
 */
class TileEntry {

public:
    // this tile's official location in the world
    SGBucket tile_bucket;
    std::string tileFileName;

    typedef vector < Point3D > point_list;
    typedef point_list::iterator point_list_iterator;
    typedef point_list::const_iterator const_point_list_iterator;

private:

    // pointer to ssg range selector for this tile
    osg::ref_ptr<osg::LOD> _node;
    // Reference to DatabaseRequest object set and used by the
    // osgDB::DatabasePager.
    osg::ref_ptr<osg::Referenced> _databaseRequest;

    static bool obj_load( const std::string& path,
                          osg::Group* geometry,
                          bool is_base,
                          const osgDB::ReaderWriter::Options* options);

    /**
     * this value is used by the tile scheduler/loader to mark which
     * tiles are in the primary ring (i.e. the current tile or the
     * surrounding eight.)  Other routines then can use this as an
     * optimization and not do some operation to tiles outside of this
     * inner ring.  (For instance vasi color updating)
     */
    bool is_inner_ring;

    static ModelLoadHelper *_modelLoader;

public:

    // Constructor
    TileEntry( const SGBucket& b );

    // Destructor
    ~TileEntry();

    static void setModelLoadHelper(ModelLoadHelper *m) { _modelLoader=m; }

    // Clean up the memory used by this tile and delete the arrays
    // used by ssg as well as the whole ssg branch.  This does a
    // partial clean up and exits so we can spread the load across
    // multiple frames.  Returns false if work remaining to be done,
    // true if dynamically allocated memory used by this tile is
    // completely freed.
    bool free_tile();

    // Update the ssg transform node for this tile so it can be
    // properly drawn relative to our (0,0,0) point
    void prep_ssg_node(float vis);

    /**
     * Transition to OSG database pager
     */
    static osg::Node* loadTileByName(const std::string& index_str,
                                     const osgDB::ReaderWriter::Options*);
    /**
     * Return true if the tile entry is loaded, otherwise return false
     * indicating that the loading thread is still working on this.
     */
    inline bool is_loaded() const
    {
        return _node->getNumChildren() > 0;
    }

    /**
     * Return the "bucket" for this tile
     */
    inline const SGBucket& get_tile_bucket() const { return tile_bucket; }

    /**
     * Add terrain mesh and ground lighting to scene graph.
     */
    void addToSceneGraph( osg::Group *terrain_branch);

    /**
     * disconnect terrain mesh and ground lighting nodes from scene
     * graph for this tile.
     */
    void removeFromSceneGraph();

	
    /**
     * return the scenegraph node for the terrain
     */
    osg::LOD *getNode() const { return _node.get(); }

    double get_timestamp() const;
    void set_timestamp( double time_ms );

    inline bool get_inner_ring() const { return is_inner_ring; }
    inline void set_inner_ring( bool val ) { is_inner_ring = val; }

    // Get the ref_ptr to the DatabaseRequest object, in order to pass
    // this to the pager.
    osg::ref_ptr<osg::Referenced>& getDatabaseRequest()
    {
        return _databaseRequest;
    }
};

class ModelLoadHelper {
public:
    virtual osg::Node *loadTileModel(const string& modelPath, bool cacheModel)=0;

};

}

#endif // _TILEENTRY_HXX 
