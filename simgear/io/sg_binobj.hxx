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


// calculate the bounding sphere.  Center is the center of the
// tile and zero elevation
double sgCalcBoundingRadius( Point3D center, point_list& wgs84_nodes );


// write out the structures to an ASCII file.  We assume that the
// groups come to us sorted by material property.  If not, things
// don't break, but the result won't be as optimal.
bool sgWriteAsciiObj( const string& base, const string& name, const SGBucket& b,
		      Point3D gbs_center, float gbs_radius,
		      const point_list& wgs84_nodes, const point_list& normals,
		      const point_list& texcoords, 
		      const group_list& tris_v, const group_list& tris_tc, 
		      const string_list& tri_materials,
		      const group_list& strips_v, const group_list& strips_tc, 
		      const string_list& strip_materials,
		      const group_list& fans_v, const group_list& fans_tc,
		      const string_list& fan_materials );


// read a binary file object and populate the provided structures.
bool sgReadBinObj( const string& file,
		    Point3D &gbs_center, float *gbs_radius,
		    point_list& wgs84_nodes, point_list& normals,
		    point_list& texcoords, 
		    group_list& tris_v, group_list& tris_tc, 
		    string_list& tri_materials,
		    group_list& strips_v, group_list& strips_tc, 
		    string_list& strip_materials,
		    group_list& fans_v, group_list& fans_tc,
		    string_list& fan_materials );

// write out the structures to a binary file.  We assume that the
// groups come to us sorted by material property.  If not, things
// don't break, but the result won't be as optimal.
bool sgWriteBinObj( const string& base, const string& name, const SGBucket& b,
		    Point3D gbs_center, float gbs_radius,
		    const point_list& wgs84_nodes, const point_list& normals,
		    const point_list& texcoords, 
		    const group_list& tris_v, const group_list& tris_tc, 
		    const string_list& tri_materials,
		    const group_list& strips_v, const group_list& strips_tc, 
		    const string_list& strip_materials,
		    const group_list& fans_v, const group_list& fans_tc,
		    const string_list& fan_materials );


#endif // _SG_BINOBJ_HXX
