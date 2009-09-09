// vector.cxx -- additional vector routines
//
// Written by Curtis Olson, started December 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#include <math.h>
#include <stdio.h>

// #include <Include/fg_types.h>

#include "vector.hxx"


// calculate the projection, p, of u along the direction of d.
void sgProjection(sgVec3 p, const sgVec3 u, const sgVec3 d){
    double denom = sgScalarProductVec3(d,d);
    if (denom == 0.) sgCopyVec3(p, u);
    else sgScaleVec3(p, d, sgScalarProductVec3(u,d) / denom);
}

// Same thing, except using double precision
void sgProjection(sgdVec3 p, const sgdVec3 u, const sgdVec3 d){
    double denom = sgdScalarProductVec3(d,d);
    if (denom == 0.) sgdCopyVec3(p, u);
    else sgdScaleVec3(p, d, sgdScalarProductVec3(u,d) / denom);
}

// Given a point p, and a line through p0 with direction vector d,
// find the closest point (p1) on the line
void sgClosestPointToLine( sgVec3 p1, const sgVec3 p, const sgVec3 p0,
			   const sgVec3 d ) {

    sgVec3 u, u1;
    
    // u = p - p0
    sgSubVec3(u, p, p0);

    // calculate the projection, u1, of u along d.
    sgProjection(u1, u, d);

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
    sgProjection(u1, u, d);

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
    sgProjection(u1, u, d);

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
    sgProjection(u1, u, d);

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


