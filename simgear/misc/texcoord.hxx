// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1999 Curtis L. Olson  - http://www.flightgear.org/~curt

/**
 * @file
 * @brief routine(s) to handle texture coordinate generation
 */

#ifndef _TEXCOORD_HXX
#define _TEXCOORD_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <vector>

#include <simgear/bucket/newbucket.hxx>
#include <simgear/math/sg_types.hxx>

#include <simgear/math/SGMathFwd.hxx>
#include <simgear/math/SGGeod.hxx>
#include <simgear/math/SGVec2.hxx>

/**
 * Traverse the specified fan/strip/list of vertices and attempt to
 * calculate "none stretching" texture coordinates.
 * @param b the bucket containing the structure
 * @param geod_nodes vertices in geodetic coordinates
 * @param fan integer list of indices pointing into the vertex list
 * @param scale (default = 1.0) scaling factor
 * @return list of texture coordinates
 */
std::vector<SGVec2f> sgCalcTexCoords( const SGBucket& b, const std::vector<SGGeod>& geod_nodes,
			    const std::vector<int>& fan, double scale = 1.0 );

std::vector<SGVec2f> sgCalcTexCoords( double centerLat, const std::vector<SGGeod>& geod_nodes,
			    const std::vector<int>& fan, double scale = 1.0 );


#endif // _TEXCOORD_HXX
