// vector.cxx -- additional vector routines
//
// Written by Curtis Olson, started December 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include <math.h>
#include <stdio.h>

// #include <Include/fg_types.h>

#include "vector.hxx"


// Given a point p, and a line through p0 with direction vector d,
// find the shortest distance (squared) from the point to the line
double sgPointLineDistSquared( const sgVec3 p, const sgVec3 p0,
			       const sgVec3 d ) {

    sgVec3 u, u1, v;
    
    // u = p - p0
    sgSubVec3(u, p, p0);

    // calculate the projection, u1, of u along d.
    // u1 = ( dot_prod(u, d) / dot_prod(d, d) ) * d;
    sgScaleVec3( u1, d, sgScalarProductVec3(u,d) / sgScalarProductVec3(d,d) );

    // v = u - u1 = vector from closest point on line, p1, to the
    // original point, p.
    sgSubVec3(v, u, u1);

    return ( sgScalarProductVec3(v, v) );
}


// Given a point p, and a line through p0 with direction vector d,
// find the shortest distance (squared) from the point to the line
double sgdPointLineDistSquared( const sgdVec3 p, const sgdVec3 p0,
				const sgdVec3 d ) {

    sgdVec3 u, u1, v;
    double ud, dd, tmp;
    
    // u = p - p0
    sgdSubVec3(u, p, p0);

    // calculate the projection, u1, of u along d.
    // u1 = ( dot_prod(u, d) / dot_prod(d, d) ) * d;
    ud = sgdScalarProductVec3(u, d);
    dd = sgdScalarProductVec3(d, d);
    tmp = ud / dd;

    sgdScaleVec3(u1, d, tmp);;

    // v = u - u1 = vector from closest point on line, p1, to the
    // original point, p.
    sgdSubVec3(v, u, u1);

    return ( sgdScalarProductVec3(v, v) );
}


// This is a quicker form of
// sgMakeMatTrans4( sgMat4 sgTrans, sgVec3 trans )
// sgPostMultMat4( sgMat, sgTRANS );
void sgPostMultMat4ByTransMat4( sgMat4 src, const sgVec3 trans )
{
    for( int i=0; i<4; i++) {
	for( int j=0; j<3; j++ ) {
	    src[i][j] += (src[i][3] * trans[j]);
	}
    }
}


