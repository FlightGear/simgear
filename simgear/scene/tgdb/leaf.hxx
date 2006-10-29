// leaf.hxx -- function to build and ssg leaf from higher level data.
//
// Written by Curtis Olson, started October 1997.
//
// Copyright (C) 1997 - 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _SG_LEAF_HXX
#define _SG_LEAF_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include <simgear/compiler.h>

#include STL_STRING

#include <osg/Array>
#include <osg/Node>

#include <simgear/math/sg_types.hxx>

SG_USING_STD(string);


class SGMaterialLib;            // forward declaration.


// Create a ssg leaf
osg::Node* SGMakeLeaf( const string& path,
                     const GLenum ty,
                     SGMaterialLib *matlib, const string& material,
                     const point_list& nodes, const point_list& normals,
                     const point_list& texcoords,
                     const int_list& node_index,
                     const int_list& normal_index,
                     const int_list& tex_index,
                     const bool calc_lights, osg::Vec3Array *lights );

#endif // _SG_LEAF_HXX
