/**
 * \file polar3d.hxx
 * Routines to deal with polar math and transformations.
 */

// Written by Curtis Olson, started June 1997.
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _POLAR3D_HXX
#define _POLAR3D_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/constants.h>
#include <simgear/math/point3d.hxx>

#include "SGMath.hxx"

/** 
 * Find the Altitude above the Ellipsoid (WGS84) given the Earth
 * Centered Cartesian coordinate vector Distances are specified in
 * meters.
 * @param cp point specified in cartesian coordinates
 * @return altitude above the (wgs84) earth in meters
 */
inline double sgGeodAltFromCart(const Point3D& p)
{
  SGGeod geod;
  SGGeodesy::SGCartToGeod(SGVec3<double>(p.x(), p.y(), p.z()), geod);
  return geod.getElevationM();
}


/**
 * Convert a polar coordinate to a cartesian coordinate.  Lon and Lat
 * must be specified in radians.  The SG convention is for distances
 * to be specified in meters
 * @param p point specified in polar coordinates
 * @return the same point in cartesian coordinates
 */
inline Point3D sgPolarToCart3d(const Point3D& p)
{
  SGVec3<double> cart;
  SGGeodesy::SGGeocToCart(SGGeoc::fromRadM(p.lon(), p.lat(), p.radius()), cart);
  return Point3D::fromSGVec3(cart);
}


/**
 * Convert a cartesian coordinate to polar coordinates (lon/lat
 * specified in radians.  Distances are specified in meters.
 * @param cp point specified in cartesian coordinates
 * @return the same point in polar coordinates
 */
inline Point3D sgCartToPolar3d(const Point3D& p)
{
  SGGeoc geoc;
  SGGeodesy::SGCartToGeoc(SGVec3<double>(p.x(), p.y(), p.z()), geoc);
  return Point3D::fromSGGeoc(geoc);
}


/**
 * Calculate new lon/lat given starting lon/lat, and offset radial, and
 * distance.  NOTE: starting point is specifed in radians, distance is
 * specified in meters (and converted internally to radians)
 * ... assumes a spherical world.
 * @param orig specified in polar coordinates
 * @param course offset radial
 * @param dist offset distance
 * @return destination point in polar coordinates
 */
Point3D calc_gc_lon_lat( const Point3D& orig, double course, double dist );


/**
 * Calculate course/dist given two spherical points.
 * @param start starting point
 * @param dest ending point
 * @param course resulting course
 * @param dist resulting distance
 */
void calc_gc_course_dist( const Point3D& start, const Point3D& dest, 
                                 double *course, double *dist );

#if 0
/**
 * Calculate course/dist given two spherical points.
 * @param start starting point
 * @param dest ending point
 * @param course resulting course
 * @param dist resulting distance
 */
void calc_gc_course_dist( const Point3D& start, const Point3D& dest, 
				 double *course, double *dist );
#endif // 0

#endif // _POLAR3D_HXX

