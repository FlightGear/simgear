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


#include "waypoint.hxx"


// Constructor
SGWayPoint::SGWayPoint( const double _lon, const double _lat,
			const modetype _mode ) {
    lon = _lon;
    lat = _lat;
    mode = _mode;
}


SGWayPoint::SGWayPoint( const double _lon, const double _lat ) {
    SGWayPoint( _lon, _lat, LATLON );
}


SGWayPoint::SGWayPoint() {
    SGWayPoint( 0.0, 0.0, LATLON );
}


// Destructor
SGWayPoint::~SGWayPoint() {
}
