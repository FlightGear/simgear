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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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


// Calculate perpendicular distance from the current route segment
// This routine assumes all points are laying on a flat plane and
// ignores the altitude (or Z) dimension.  For best results, use with
// CARTESIAN way points.
double SGRoute::distance_off_route( double x, double y ) const {
    if ( current_wp > 0 ) {
	int n0 = current_wp - 1;
	int n1 = current_wp;
	sgdVec3 p, p0, p1, d;
	sgdSetVec3( p, x, y, 0.0 );
	sgdSetVec3( p0, 
		    route[n0].get_target_lon(), route[n0].get_target_lat(),
		    0.0 );
	sgdSetVec3( p1,
		    route[n1].get_target_lon(), route[n1].get_target_lat(),
		    0.0 );
	sgdSubVec3( d, p0, p1 );

	return sqrt( sgdClosestPointToLineDistSquared( p, p0, d ) );

    } else {
	// We are tracking the first waypoint so there is no route
	// segment.  If you add the current location as the first
	// waypoint and the actual waypoint as the second, then we
	// will have a route segment and calculate distance from it.

	return 0;
    }
}
