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


#include <plib/sg.h>

#include <Time/fg_time.hxx>

#include "star.hxx"
#include "moon.hxx"
#include "mercury.hxx"
#include "venus.hxx"
#include "mars.hxx"
#include "jupiter.hxx"
#include "saturn.hxx"
#include "uranus.hxx"
#include "neptune.hxx"


class FGEphemeris {

    Star *our_sun;
    Moon *moon;
    Mercury *mercury;
    Venus *venus;
    Mars *mars;
    Jupiter *jupiter;
    Saturn *saturn;
    Uranus *uranus;
    Neptune *neptune;

    // 9 planets, minus earth, minus pluto which we don't draw = 7
    // planets[i][0] = Right Ascension
    // planets[i][1] = Declination
    // planets[i][2] = Magnitude
    int nplanets;
    sgdVec3 planets[7];
    
public:

    // Constructor
    FGEphemeris( void );

    // Destructor
    ~FGEphemeris( void );

    // Update (recalculate) the positions of all objects for the
    // specified time
    void update(FGTime *t, double lat);

    // sun
    inline Star *get_sun() const { return our_sun; }
    inline double getSunRightAscension() const {
	return our_sun->getRightAscension();
    }
    inline double getSunDeclination() const {
	return our_sun->getDeclination();
    }

    // moon
    inline Moon *get_moon() const { return moon; }
    inline double getMoonRightAscension() const {
	return moon->getRightAscension();
    }
    inline double getMoonDeclination() const {
	return moon->getDeclination();
    }

    // planets
    inline int getNumPlanets() const { return nplanets; }
    inline sgdVec3 *getPlanets() { return planets; }
};


#endif // _EPHEMERIS_HXX


