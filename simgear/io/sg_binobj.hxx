// sg_binobj.hxx -- routines to read and write low level flightgear 3d objects
//
// Written by Curtis Olson, started January 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$
//


#ifndef _SG_BINOBJ_HXX
#define _SG_BINOBJ_HXX


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <plib/sg.h>

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/math/sg_types.hxx>
#include <simgear/bucket/newbucket.hxx>

#include <stdio.h>
#include <time.h>

#include <list>
#include STL_STRING



typedef vector < int_list > group_list;
typedef group_list::iterator group_list_iterator;
typedef group_list::const_iterator const_group_list_iterator;


#define SG_FILE_MAGIC_NUMBER  ( ('S'<<24) + ('G'<<16) + SG_BINOBJ_VERSION )


/*
   scenery-file: magic, nobjects, object+
   magic: "TG" + version
   object: obj_typecode, nproperties, nelements, property+, element+
   element: nbytes, BYTE+
   property: prop_typecode, nbytes, BYTE+
   obj_typecode: bounding sphere | vertices | normals | texcoords | triangles |
                fans | strips 
   prop_typecode: material_name | ???
   nelements: SHORT (Gives us 65536 which ought to be enough, right?)
   nproperties: SHORT
   *_typecode: CHAR
   nbytes: INTEGER (If we used short here that would mean 65536 bytes = 16384
                    floats = 5461 vertices which is not enough for future
		    growth)
   vertex: FLOAT, FLOAT, FLOAT
*/


class SGBinObject {
    Point3D gbs_center;
    float gbs_radius;
    point_list wgs84_nodes;
    point_list normals;
    point_list texcoords;
    group_list tris_v;
    group_list tris_tc; 
    string_list tri_materials;
    group_list strips_v;
    group_list strips_tc; 
    string_list strip_materials;
    group_list fans_v;
    group_list fans_tc;
    string_list fan_materials;

public:

    inline Point3D get_gbs_center() const { return gbs_center; }
    inline void set_gbs_center( Point3D p ) { gbs_center = p; }

    inline float get_gbs_radius() const { return gbs_radius; }
    inline void set_gbs_radius( float r ) { gbs_radius = r; }

    inline point_list get_wgs84_nodes() const { return wgs84_nodes; }
    inline void set_wgs84_nodes( point_list n ) { wgs84_nodes = n; }

    inline point_list get_normals() const { return normals; }
    inline void set_normals( point_list n ) { normals = n; }

    inline point_list get_texcoords() const { return texcoords; }
    inline void set_texcoords( point_list t ) { texcoords = t; }

    inline group_list get_tris_v() const { return tris_v; }
    inline void set_tris_v( group_list g ) { tris_v = g; }
    inline group_list get_tris_tc() const { return tris_tc; }
    inline void set_tris_tc( group_list g ) { tris_tc = g; }
    inline string_list get_tri_materials() const { return tri_materials; }
    inline void set_tri_materials( string_list s ) { tri_materials = s; }
    
    inline group_list get_strips_v() const { return strips_v; }
    inline void set_strips_v( group_list g ) { strips_v = g; }
    inline group_list get_strips_tc() const { return strips_tc; }
    inline void set_strips_tc( group_list g ) { strips_tc = g; }
    inline string_list get_strip_materials() const { return strip_materials; }
    inline void set_strip_materials( string_list s ) { strip_materials = s; }
    
    inline group_list get_fans_v() const { return fans_v; }
    inline void set_fans_v( group_list g ) { fans_v = g; }
    inline group_list get_fans_tc() const { return fans_tc; }
    inline void set_fans_tc( group_list g ) { fans_tc = g; }
    inline string_list get_fan_materials() const { return fan_materials; }
    inline void set_fan_materials( string_list s ) { fan_materials = s; }

    // read a binary file object and populate the provided structures.
    bool read_bin( const string& file );

    // write out the structures to a binary file.  We assume that the
    // groups come to us sorted by material property.  If not, things
    // don't break, but the result won't be as optimal.
    bool write_bin( const string& base, const string& name, const SGBucket& b );

    // write out the structures to an ASCII file.  We assume that the
    // groups come to us sorted by material property.  If not, things
    // don't break, but the result won't be as optimal.
    bool write_ascii( const string& base, const string& name,
		      const SGBucket& b );
};


// calculate the bounding sphere.  Center is the center of the
// tile and zero elevation
double sgCalcBoundingRadius( Point3D center, point_list& wgs84_nodes );


#endif // _SG_BINOBJ_HXX
