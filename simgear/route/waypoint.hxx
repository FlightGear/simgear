/** 
 * \file waypoint.hxx
 * Provides a class to manage waypoints
 */

// Written by Curtis Olson, started September 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@hfrl.umn.edu
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


#ifndef _WAYPOINT_HXX
#define _WAYPOINT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/compiler.h>

#include STL_STRING

SG_USING_STD(string);


/**
 * A class to manage waypoints.
 */

class SGWayPoint {

public:

    /**
     * Waypoint mode.
     * <li> WGS84 requests all bearing and distance math be done assuming a
     *      WGS84 ellipsoid world.  This is the most expensive computationally,
     *      but also the most accurate.
     * <li> SPHERICAL requests all bearing and distance math be done assuming
     *      the world is a perfect sphere.  This is less compuntationally
     *      expensive than using wgs84 math and still a fairly good
     *      approximation of the real world, especially over shorter distances.
     * <li> CARTESIAN requests all math be done assuming the coordinates specify
     *      position in a Z = up world.
     */
    enum modetype { 
	WGS84 = 0,
	SPHERICAL = 1,
	CARTESIAN = 2
    };

private:

    modetype mode;

    double target_lon;
    double target_lat;
    double target_alt;
    double distance;

    string id;

public:

    /**
     * Construct a waypoint
     * @param lon destination longitude
     * @param lat destination latitude
     * @param alt target altitude
     * @param mode type of coordinates/math to use
     * @param s waypoint identifier
     */
    SGWayPoint( const double lon = 0.0, const double lat = 0.0,
		const double alt = 0.0, const modetype m = WGS84,
		const string s = "" );

    /** Destructor */
    ~SGWayPoint();

    /**
     * Calculate course and distances.  For WGS84 and SPHERICAL
     * coordinates lat, lon, and course are in degrees, alt and
     * distance are in meters.  For CARTESIAN coordinates x = lon, y =
     * lat.  Course is in degrees and distance is in what ever units x
     * and y are in.
     * @param cur_lon (in) current longitude
     * @param cur_lat (in) current latitude
     * @param cur_alt (in) current altitude
     * @param course (out) heading from current location to this waypoint
     * @param distance (out) distance from current location to this waypoint
     */
    void CourseAndDistance( const double cur_lon, const double cur_lat,
			    const double cur_alt,
			    double *course, double *distance ) const;

    /**
     * Calculate course and distances between a specified starting waypoint
     * and this waypoint.
     * @param wp (in) original waypoint
     * @param course (out) heading from current location to this waypoint
     * @param distance (out) distance from current location to this waypoint
     */
    void CourseAndDistance( const SGWayPoint &wp,
			    double *course, double *distance ) const;

    /** @return waypoint mode */
    inline modetype get_mode() const { return mode; }

    /** @return waypoint longitude */
    inline double get_target_lon() const { return target_lon; }

    /** @return waypoint latitude */
    inline double get_target_lat() const { return target_lat; }

    /** @return waypoint altitude */
    inline double get_target_alt() const { return target_alt; }

    /**
     * This value is not calculated by this class.  It is simply a
     * placeholder for the user to stash a distance value.  This is useful
     * when you stack waypoints together into a route.  You can calculate the
     * distance from the previous waypoint once and save it here.  Adding up
     * all the distances here plus the distance to the first waypoint gives you
     * the total route length.  Note, you must update this value yourself, this
     * is for your convenience only.
     * @return waypoint distance holder (what ever the user has stashed here)
     */
    inline double get_distance() const { return distance; }

    /**
     * Set the waypoint distance value to a value of our choice.
     * @param d distance 
     */
    inline void set_distance( double d ) { distance = d; }

    /** @return waypoint id */
    inline string get_id() const { return id; }

};


#endif // _WAYPOINT_HXX
