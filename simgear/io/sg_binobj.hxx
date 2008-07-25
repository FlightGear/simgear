/**
 * \file sg_binobj.hxx
 * Routines to read and write the low level (binary) simgear 3d object format.
 */

// Written by Curtis Olson, started January 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _SG_BINOBJ_HXX
#define _SG_BINOBJ_HXX


#include <plib/sg.h>

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/bucket/newbucket.hxx>

#include <stdio.h>
#include <time.h>

#include <list>
#include <string>



/** STL Structure used to store object information */
typedef vector < int_list > group_list;
typedef group_list::iterator group_list_iterator;
typedef group_list::const_iterator const_group_list_iterator;


/** Magic Number for our file format */
#define SG_FILE_MAGIC_NUMBER  ( ('S'<<24) + ('G'<<16) + SG_BINOBJ_VERSION )


/**
 * A class to manipulate the simgear 3d object format.
 * This class provides functionality to both read and write the binary format.
 *
 * Here is a really quick overview of the file syntax:
 *
 * - scenery-file: magic, nobjects, object+
 *
 * - magic: "TG" + version
 *
 * - object: obj_typecode, nproperties, nelements, property+, element+
 *
 * - element: nbytes, BYTE+
 *
 * - property: prop_typecode, nbytes, BYTE+
 *
 * - obj_typecode: bounding sphere | vertices | normals | texcoords |
 *                 points | triangles | fans | strips 
 *
 * - prop_typecode: material_name | ???
 *
 * - nelements: USHORT (Gives us 65536 which ought to be enough, right?)
 *
 * - nproperties: USHORT
 *
 * - *_typecode: CHAR
 *
 * - nbytes: INTEGER (If we used short here that would mean 65536 bytes = 16384
 *                    floats = 5461 vertices which is not enough for future
 *	              growth)
 *
 * - vertex: FLOAT, FLOAT, FLOAT
*/
class SGBinObject {
    unsigned short version;

    SGVec3d gbs_center;
    float gbs_radius;

    std::vector<SGVec3d> wgs84_nodes;	// vertex list
    std::vector<SGVec4f> colors;	// color list
    std::vector<SGVec3f> normals;	// normal list
    std::vector<SGVec2f> texcoords;	// texture coordinate list

    group_list pts_v;		// points vertex index
    group_list pts_n;		// points normal index
    group_list pts_c;		// points color index
    group_list pts_tc;		// points texture coordinate index
    string_list pt_materials;	// points materials

    group_list tris_v;		// triangles vertex index
    group_list tris_n;		// triangles normal index
    group_list tris_c;		// triangles color index
    group_list tris_tc;		// triangles texture coordinate index
    string_list tri_materials;	// triangles materials

    group_list strips_v;	// tristrips vertex index
    group_list strips_n;	// tristrips normal index
    group_list strips_c;	// tristrips color index
    group_list strips_tc;	// tristrips texture coordinate index
    string_list strip_materials;// tristrips materials

    group_list fans_v;		// fans vertex index
    group_list fans_n;		// fans normal index
    group_list fans_c;		// fans color index
    group_list fans_tc;		// fans texture coordinate index
    string_list fan_materials;	// fans materials

public:

    inline unsigned short get_version() const { return version; }

    inline Point3D get_gbs_center() const { return Point3D::fromSGVec3(gbs_center); }
    inline void set_gbs_center( const Point3D& p ) { gbs_center = p.toSGVec3d(); }
    inline const SGVec3d& get_gbs_center2() const { return gbs_center; }
    inline void set_gbs_center( const SGVec3d& p ) { gbs_center = p; }

    inline float get_gbs_radius() const { return gbs_radius; }
    inline void set_gbs_radius( float r ) { gbs_radius = r; }

    inline const std::vector<SGVec3d>& get_wgs84_nodes() const
    { return wgs84_nodes; }
    inline void set_wgs84_nodes( const std::vector<SGVec3d>& n )
    { wgs84_nodes = n; }
    inline void set_wgs84_nodes( const point_list& n )
    {
      wgs84_nodes.resize(n.size());
      for (unsigned i = 0; i < wgs84_nodes.size(); ++i)
        wgs84_nodes[i] = n[i].toSGVec3d();
    }

    inline const std::vector<SGVec4f>& get_colors() const { return colors; }
    inline void set_colors( const std::vector<SGVec4f>& c ) { colors = c; }
    inline void set_colors( const point_list& c )
    {
      colors.resize(c.size());
      for (unsigned i = 0; i < colors.size(); ++i)
        colors[i] = SGVec4f(c[i].toSGVec3f(), 1);
    }

    inline const std::vector<SGVec3f>& get_normals() const { return normals; }
    inline void set_normals( const std::vector<SGVec3f>& n ) { normals = n; }
    inline void set_normals( const point_list& n )
    {
      normals.resize(n.size());
      for (unsigned i = 0; i < normals.size(); ++i)
        normals[i] = n[i].toSGVec3f();
    }

    inline const std::vector<SGVec2f>& get_texcoords() const { return texcoords; }
    inline void set_texcoords( const std::vector<SGVec2f>& t ) { texcoords = t; }
    inline void set_texcoords( const point_list& t )
    {
      texcoords.resize(t.size());
      for (unsigned i = 0; i < texcoords.size(); ++i)
        texcoords[i] = t[i].toSGVec2f();
    }

    inline const group_list& get_pts_v() const { return pts_v; }
    inline void set_pts_v( const group_list& g ) { pts_v = g; }
    inline const group_list& get_pts_n() const { return pts_n; }
    inline void set_pts_n( const group_list& g ) { pts_n = g; }
    inline const group_list& get_pts_c() const { return pts_c; }
    inline void set_pts_c( const group_list& g ) { pts_c = g; }
    inline const group_list& get_pts_tc() const { return pts_tc; }
    inline void set_pts_tc( const group_list& g ) { pts_tc = g; }
    inline const string_list& get_pt_materials() const { return pt_materials; }
    inline void set_pt_materials( const string_list& s ) { pt_materials = s; }

    inline const group_list& get_tris_v() const { return tris_v; }
    inline void set_tris_v( const group_list& g ) { tris_v = g; }
    inline const group_list& get_tris_n() const { return tris_n; }
    inline void set_tris_n( const group_list& g ) { tris_n = g; }
    inline const group_list& get_tris_c() const { return tris_c; }
    inline void set_tris_c( const group_list& g ) { tris_c = g; }
    inline const group_list& get_tris_tc() const { return tris_tc; }
    inline void set_tris_tc( const group_list& g ) { tris_tc = g; }
    inline const string_list& get_tri_materials() const { return tri_materials; }
    inline void set_tri_materials( const string_list& s ) { tri_materials = s; }
    
    inline const group_list& get_strips_v() const { return strips_v; }
    inline void set_strips_v( const group_list& g ) { strips_v = g; }
    inline const group_list& get_strips_n() const { return strips_n; }
    inline void set_strips_n( const group_list& g ) { strips_n = g; }
    inline const group_list& get_strips_c() const { return strips_c; }
    inline void set_strips_c( const group_list& g ) { strips_c = g; }

    inline const group_list& get_strips_tc() const { return strips_tc; }
    inline void set_strips_tc( const group_list& g ) { strips_tc = g; }
    inline const string_list& get_strip_materials() const { return strip_materials; }
    inline void set_strip_materials( const string_list& s ) { strip_materials = s; }
    
    inline const group_list& get_fans_v() const { return fans_v; }
    inline void set_fans_v( const group_list& g ) { fans_v = g; }
    inline const group_list& get_fans_n() const { return fans_n; }
    inline void set_fans_n( const group_list& g ) { fans_n = g; }
    inline const group_list& get_fans_c() const { return fans_c; }
    inline void set_fans_c( const group_list& g ) { fans_c = g; }

    inline const group_list& get_fans_tc() const { return fans_tc; }
    inline void set_fans_tc( const group_list& g ) { fans_tc = g; }
    inline const string_list& get_fan_materials() const { return fan_materials; }
    inline void set_fan_materials( const string_list& s ) { fan_materials = s; }

    /**
     * Read a binary file object and populate the provided structures.
     * @param file input file name
     * @return result of read
     */
    bool read_bin( const string& file );

    /** 
     * Write out the structures to a binary file.  We assume that the
     * groups come to us sorted by material property.  If not, things
     * don't break, but the result won't be as optimal.
     * @param base name of output path
     * @param name name of output file
     * @param b bucket for object location
     * @return result of write
     */
    bool write_bin( const string& base, const string& name, const SGBucket& b );

    /**
     * Write out the structures to an ASCII file.  We assume that the
     * groups come to us sorted by material property.  If not, things
     * don't break, but the result won't be as optimal.
     * @param base name of output path
     * @param name name of output file
     * @param b bucket for object location
     * @return result of write
     */
    bool write_ascii( const string& base, const string& name,
		      const SGBucket& b );
};


/**
 * \relates SGBinObject
 * Calculate the center of a list of points, by taking the halfway
 * point between the min and max points.
 * @param wgs84_nodes list of points in wgs84 coordinates
 * @return center point
 */
Point3D sgCalcCenter( point_list& wgs84_nodes );


/**
 * \relates SGBinObject
 * Calculate the bounding sphere of a set of nodes.
 * Center is the center of the tile and zero elevation.
 * @param center center of our bounding radius
 * @param wgs84_nodes list of points in wgs84 coordinates
 * @return radius
 */
double sgCalcBoundingRadius( Point3D center, point_list& wgs84_nodes );


#endif // _SG_BINOBJ_HXX
