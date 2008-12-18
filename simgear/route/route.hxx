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

#include <vector>

using std::vector;

#include <simgear/route/waypoint.hxx>

/**
 * A class to manage a list of waypoints (i.e. a route).
 */

class SGRoute {

private:

    typedef vector < SGWayPoint > route_list;
    route_list route;
    int current_wp;

    void update_distance(int index);

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
     * Add waypoint (default), or insert waypoint at position n.
     * @param wp a waypoint
     */
    void add_waypoint( const SGWayPoint &wp, int n = -1 );
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

    inline int current_index() const {
        return current_wp;
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
    inline void delete_first() { delete_waypoint(0); }

    /** Delete waypoint waypoint with index n  (last one if n < 0) */
    void delete_waypoint( int n = 0 );

    /**
     * Calculate perpendicular distance from the current route segment
     * This routine assumes all points are laying on a flat plane and
     * ignores the altitude (or Z) dimension.  For most accurate
     * results, use with CARTESIAN way points.
     */
    double distance_off_route( double x, double y ) const;
};


#endif // _ROUTE_HXX
