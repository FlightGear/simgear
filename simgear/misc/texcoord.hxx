/**
 * \file texcoord.hxx
 * Routine to handle texture coordinate generation.
 */

// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _TEXCOORD_HXX
#define _TEXCOORD_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/sg_types.hxx>


/**
 * Traverse the specified fan/strip/list of vertices and attempt to
 * calculate "none stretching" texture coordinates.
 * @param b the bucket containing the structure
 * @param geod_nodes vertices in geodetic coordinates
 * @param fan integer list of indices pointing into the vertex list
 * @param scale (default = 1.0) scaling factor
 * @return list of texture coordinates
 */
point_list sgCalcTexCoords( const SGBucket& b, const point_list& geod_nodes,
			    const int_list& fan, double scale = 1.0 );


#endif // _TEXCOORD_HXX
