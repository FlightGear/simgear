// waypoint.cxx -- Class to hold data and return info relating to a waypoint
//
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


#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_geodesy.hxx>

#include "waypoint.hxx"


// Constructor
SGWayPoint::SGWayPoint( const double lon, const double lat, const double alt,
			const modetype m, const string s ) {
    target_lon = lon;
    target_lat = lat;
    target_alt = alt;
    mode = m;
    id = s;
}


SGWayPoint::SGWayPoint() {
    SGWayPoint( 0.0, 0.0, 0.0, WGS84, "" );
}


// Destructor
SGWayPoint::~SGWayPoint() {
}


// Calculate course and distances.  For WGS84 and SPHERICAL
// coordinates lat, lon, and course are in degrees, alt and distance
// are in meters.  For CARTESIAN coordinates x = lon, y = lat.  Course
// is in degrees and distance is in what ever units x and y are in.
void SGWayPoint::CourseAndDistance( const double cur_lon,
				    const double cur_lat,
				    const double cur_alt,
				    double *course, double *distance ) {
    if ( mode == WGS84 ) {
	double reverse;
	geo_inverse_wgs_84( cur_alt, cur_lat, cur_lon, target_lat, target_lon,
			    course, &reverse, distance );
    } else if ( mode == SPHERICAL ) {
	Point3D current( cur_lon * DEG_TO_RAD, cur_lat * DEG_TO_RAD, 0.0 );
	Point3D target( target_lon * DEG_TO_RAD, target_lat * DEG_TO_RAD, 0.0 );
	calc_gc_course_dist( current, target, course, distance );
	*course = 360.0 - *course * RAD_TO_DEG;
    } else if ( mode == CARTESIAN ) {
	double dx = target_lon - cur_lon;
	double dy = target_lat - cur_lat;
	*course = -atan2( dy, dx ) * RAD_TO_DEG - 90;
	while ( *course < 0 ) {
	    *course += 360.0;
	}
	while ( *course > 360.0 ) {
	    *course -= 360.0;
	}
	*distance = sqrt( dx * dx + dy * dy );
    }
}
