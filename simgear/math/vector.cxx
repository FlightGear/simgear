// vector.cxx -- additional vector routines
//
// Written by Curtis Olson, started December 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#include <math.h>
#include <stdio.h>

// #include <Include/fg_types.h>

#include "vector.hxx"


// Given a point p, and a line through p0 with direction vector d,
// find the closest point (p1) on the line
void sgClosestPointToLine( sgVec3 p1, const sgVec3 p, const sgVec3 p0,
			   const sgVec3 d ) {

    sgVec3 u, u1;
    
    // u = p - p0
    sgSubVec3(u, p, p0);

    // calculate the projection, u1, of u along d.
    // u1 = ( dot_prod(u, d) / dot_prod(d, d) ) * d;
    sgScaleVec3( u1, d, sgScalarProductVec3(u,d) / sgScalarProductVec3(d,d) );

    // calculate the point p1 along the line that is closest to p
    // p0 = p1 + u1
    sgAddVec3(p1, p0, u1);
}


// Given a point p, and a line through p0 with direction vector d,
// find the closest point (p1) on the line
void sgdClosestPointToLine( sgdVec3 p1, const sgdVec3 p, const sgdVec3 p0,
			    const sgdVec3 d ) {

    sgdVec3 u, u1;
    
    // u = p - p0
    sgdSubVec3(u, p, p0);

    // calculate the projection, u1, of u along d.
    // u1 = ( dot_prod(u, d) / dot_prod(d, d) ) * d;
    double ud = sgdScalarProductVec3(u, d);
    double dd = sgdScalarProductVec3(d, d);
    double tmp = ud / dd;

    sgdScaleVec3(u1, d, tmp);;

    // calculate the point p1 along the line that is closest to p
    // p0 = p1 + u1
    sgdAddVec3(p1, p0, u1);
}


// Given a point p, and a line through p0 with direction vector d,
// find the shortest distance (squared) from the point to the line
double sgClosestPointToLineDistSquared( const sgVec3 p, const sgVec3 p0,
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
double sgdClosestPointToLineDistSquared( const sgdVec3 p, const sgdVec3 p0,
					 const sgdVec3 d ) {

    sgdVec3 u, u1, v;
    
    // u = p - p0
    sgdSubVec3(u, p, p0);

    // calculate the projection, u1, of u along d.
    // u1 = ( dot_prod(u, d) / dot_prod(d, d) ) * d;
    double ud = sgdScalarProductVec3(u, d);
    double dd = sgdScalarProductVec3(d, d);
    double tmp = ud / dd;

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


