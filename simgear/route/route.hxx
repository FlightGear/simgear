/**
 * \file route.hxx
 * Provides a class to manage a list of waypoints (i.e. a route).
 */

// Written by Curtis Olson, started October 2000.
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _ROUTE_HXX
#define _ROUTE_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include <simgear/compiler.h>

#include STL_STRING
#include <vector>

SG_USING_STD(string);
SG_USING_STD(vector);

#include <simgear/route/waypoint.hxx>

/**
 * A class to manage a list of waypoints (i.e. a route).
 */

class SGRoute {

private:

    typedef vector < SGWayPoint > route_list;
    route_list route;
    int current_wp;

public:

    /** Constructor */
    SGRoute();

    /** Destructor */
    ~SGRoute();

    /** Clear the entire route */
    inline void clear() {
	route.clear();
	current_wp = 0;
    }

    /**
     * Add a waypoint.
     * @param wp a waypoint
     */
    inline void add_waypoint( const SGWayPoint &wp ) {
	route.push_back( wp );

	int size = route.size();
	if ( size > 1 ) {
	    SGWayPoint next_to_last = route[ size - 2 ];
	    double tmpd, tmpc;
	    wp.CourseAndDistance( next_to_last, &tmpc, &tmpd );
	    route[size - 1].set_distance( tmpd );
	}
    }

    /**
     * Get the number of waypoints (i.e. route length )
     * @return route length
     */
    inline int size() const { return route.size(); }

    /**
     * Get the front waypoint.
     * @return the first waypoint.
     */
    inline SGWayPoint get_first() const {
	if ( route.size() ) {
	    return route[0];
	} else {
	    return SGWayPoint( 0.0, 0.0, 0.0, SGWayPoint::WGS84, "invalid" );
	}
    }

    /**
     * Get the current waypoint
     * @return the current waypoint
     */
    inline SGWayPoint get_current() const {
	if ( current_wp < (int)route.size() ) {
	    return route[current_wp];
	} else {
	    return SGWayPoint( 0.0, 0.0, 0.0, SGWayPoint::WGS84, "invalid" );
	}
    }

    /**
     * Set the current waypoint
     * @param number of waypoint to make current.
     */
    inline void set_current( int n ) {
	if ( n >= 0 && n < (int)route.size() ) {
	    current_wp = n;
	}
    }

    /** Increment the current waypoint pointer. */
    inline void increment_current() {
	if ( current_wp < (int)route.size() - 1 ) {
	    ++current_wp;
	}
    }

    /**
     * Get the nth waypoint
     * @param n waypoint number
     * @return the nth waypoint
     */
    inline SGWayPoint get_waypoint( const int n ) const {
	if ( n < (int)route.size() ) {
	    return route[n];
	} else {
	    return SGWayPoint( 0.0, 0.0, 0.0, SGWayPoint::WGS84, "invalid" );
	}
    }

    /** Delete the front waypoint */
    inline void delete_first() {
	if ( route.size() ) {
	    route.erase( route.begin() );
	}
    }

    /**
     * Calculate perpendicular distance from the current route segment
     * This routine assumes all points are laying on a flat plane and
     * ignores the altitude (or Z) dimension.  For most accurate
     * results, use with CARTESIAN way points.
     */
    double distance_off_route( double x, double y ) const;
};


#endif // _ROUTE_HXX
