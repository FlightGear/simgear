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

    string id;

public:

    SGWayPoint();
    SGWayPoint( const double lon, const double lat,
		const modetype m = WGS84, const string s = "" );
    ~SGWayPoint();

    // Calculate course and distances.  Lat, lon, and azimuth are in
    // degrees.  distance in meters
    void CourseAndDistance( const double cur_lon, const double cur_lat,
			    double *course, double *distance );

    inline modetype get_mode() const { return mode; }
    inline double get_target_lon() const { return target_lon; }
    inline double get_target_lat() const { return target_lat; }
    inline string get_id() const { return id; }
};


#endif // _WAYPOINT_HXX
