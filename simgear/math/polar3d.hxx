// polar.hxx -- routines to deal with polar math and transformations
//
// Written by Curtis Olson, started June 1997.
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


#ifndef _POLAR_HXX
#define _POLAR_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/constants.h>
#include <simgear/point3d.hxx>


// Find the Altitude above the Ellipsoid (WGS84) given the Earth
// Centered Cartesian coordinate vector Distances are specified in
// meters.
double fgGeodAltFromCart(const Point3D& cp);


// Convert a polar coordinate to a cartesian coordinate.  Lon and Lat
// must be specified in radians.  The FG convention is for distances
// to be specified in meters
inline Point3D fgPolarToCart3d(const Point3D& p) {
    double tmp = cos( p.lat() ) * p.radius();

    return Point3D( cos( p.lon() ) * tmp,
		    sin( p.lon() ) * tmp,
		    sin( p.lat() ) * p.radius() );
}


// Convert a cartesian coordinate to polar coordinates (lon/lat
// specified in radians.  Distances are specified in meters.
inline Point3D fgCartToPolar3d(const Point3D& cp) {
    return Point3D( atan2( cp.y(), cp.x() ),
		    FG_PI_2 - 
		    atan2( sqrt(cp.x()*cp.x() + cp.y()*cp.y()), cp.z() ),
		    sqrt(cp.x()*cp.x() + cp.y()*cp.y() + cp.z()*cp.z()) );
}


// calc new lon/lat given starting lon/lat, and offset radial, and
// distance.  NOTE: starting point is specifed in radians, distance is
// specified in meters (and converted internally to radians)
// ... assumes a spherical world
inline Point3D calc_lon_lat( const Point3D& orig, double course, double dist ) {
    Point3D result;

    // lat=asin(sin(lat1)*cos(d)+cos(lat1)*sin(d)*cos(tc))
    // IF (cos(lat)=0)
    //   lon=lon1      // endpoint a pole
    // ELSE
    //   lon=mod(lon1-asin(sin(tc)*sin(d)/cos(lat))+pi,2*pi)-pi
    // ENDIF

    // printf("calc_lon_lat()  offset.theta = %.2f offset.dist = %.2f\n",
    //        offset.theta, offset.dist);

    dist *= METER_TO_NM * NM_TO_RAD;
    
    result.sety( asin( sin(orig.y()) * cos(dist) + 
		       cos(orig.y()) * sin(dist) * cos(course) ) );

    if ( cos(result.y()) < FG_EPSILON ) {
        result.setx( orig.x() );      // endpoint a pole
    } else {
        result.setx( 
	    fmod(orig.x() - asin( sin(course) * sin(dist) / 
				  cos(result.y()) ) + FG_PI, FG_2PI) - FG_PI );
    }

    return result;
}


#endif // _POLAR_HXX


