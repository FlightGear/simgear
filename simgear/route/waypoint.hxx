// waypoint.hxx -- Class to hold data and return info relating to a waypoint
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


#ifndef _WAYPOINT_HXX
#define _WAYPOINT_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include STL_STRING

FG_USING_STD(string);


class SGWayPoint {

public:

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

    string id;

public:

    SGWayPoint();
    SGWayPoint( const double lon, const double lat, const double alt,
		const modetype m = WGS84, const string s = "" );
    ~SGWayPoint();

    // Calculate course and distances.  For WGS84 and SPHERICAL
    // coordinates lat, lon, and course are in degrees, alt and
    // distance are in meters.  For CARTESIAN coordinates x = lon, y =
    // lat.  Course is in degrees and distance is in what ever units x
    // and y are in.
    void CourseAndDistance( const double cur_lon, const double cur_lat,
			    const double cur_alt,
			    double *course, double *distance );

    inline modetype get_mode() const { return mode; }
    inline double get_target_lon() const { return target_lon; }
    inline double get_target_lat() const { return target_lat; }
    inline double get_target_alt() const { return target_alt; }
    inline string get_id() const { return id; }
};


#endif // _WAYPOINT_HXX
