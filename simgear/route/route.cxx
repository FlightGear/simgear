// route.cxx -- Class to manage a list of waypoints (route)
//
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


#include <plib/sg.h>

#include <simgear/math/vector.hxx>

#include "route.hxx"


// constructor
SGRoute::SGRoute() {
    route.clear();
}


// destructor
SGRoute::~SGRoute() {
}

/** Update the length of the leg ending at waypoint index */
void SGRoute::update_distance(int index)
{
    SGWayPoint& curr = route[ index ];
    double course, dist;

    if ( index == 0 ) {
	dist = 0;
    } else {
	const SGWayPoint& prev = route[ index - 1 ];
	curr.CourseAndDistance( prev, &course, &dist );
    }

    curr.set_distance( dist );
}

/**
 * Add waypoint (default), or insert waypoint at position n.
 * @param wp a waypoint
 */
void SGRoute::add_waypoint( const SGWayPoint &wp, int n ) {
    int size = route.size();
    if ( n < 0 || n >= size ) {
        n = size;
        route.push_back( wp );
    } else {
        route.insert( route.begin() + n, 1, wp );
        // update distance of next leg if not at end of route
        update_distance( n + 1 );
    }
    update_distance( n );
}

/** Delete waypoint with index n  (last one if n < 0) */
void SGRoute::delete_waypoint( int n ) {
    int size = route.size();
    if ( size == 0 )
        return;
    if ( n < 0 || n >= size )
        n = size - 1;

    route.erase( route.begin() + n );
    // update distance of next leg if not at end of route
    if ( n < size - 1 )
        update_distance( n );
}

double SGRoute::total_distance() const {
  double total = 0.0;
  for (unsigned int i=0; i<route.size(); ++i) {
    total += route[i].get_distance();
  }
  return total;
}
