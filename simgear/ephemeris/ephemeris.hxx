// ephemeris.hxx -- Top level class for calculating current positions of
//                  astronomical objects
//
// Written by Curtis Olson, started March 2000.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
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


#ifndef _EPHEMERIS_HXX
#define _EPHEMERIS_HXX


#include <Time/fg_time.hxx>

#include "star.hxx"
#include "moon.hxx"


class FGEphemeris {

    Star *our_sun;
    Moon *moon;

public:

    // Constructor
    FGEphemeris( void );

    // Destructor
    ~FGEphemeris( void );

    // Update (recalculate) the positions of all objects for the
    // specified time
    void update(FGTime *t);

    // sun position
    inline double getSunRightAscension() {
	return our_sun->getRightAscension();
    }
    inline double getSunDeclination() {
	return our_sun->getDeclination();
    }

    // moon position
    inline double getMoonRightAscension() {
	return moon->getRightAscension();
    }
    inline double getMoonDeclination() {
	return moon->getDeclination();
    }
};


#endif // _EPHEMERIS_HXX


