// ephemeris.hxx -- Top level class for calculating current positions of
//                  astronomical objects
//
// Top level interface written by Curtis Olson, started March 2000.
//
// All the core code underneath this is written by Durk Talsma.  See
// the headers of all the other individual files for details.
//
// Copyright (C) 2000  Curtis L. Olson - curt@flightgear.org
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _EPHEMERIS_HXX
#define _EPHEMERIS_HXX


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/sg.h>

#include <simgear/ephemeris/star.hxx>
#include <simgear/ephemeris/moon.hxx>
#include <simgear/ephemeris/mercury.hxx>
#include <simgear/ephemeris/venus.hxx>
#include <simgear/ephemeris/mars.hxx>
#include <simgear/ephemeris/jupiter.hxx>
#include <simgear/ephemeris/saturn.hxx>
#include <simgear/ephemeris/uranus.hxx>
#include <simgear/ephemeris/neptune.hxx>
#include <simgear/ephemeris/stars.hxx>


class SGEphemeris {

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

    SGStarData *stars;

public:

    // Constructor
    SGEphemeris( const string &path );

    // Destructor
    ~SGEphemeris( void );

    // Update (recalculate) the positions of all objects for the
    // specified time
    void update(double mjd, double lst, double lat);

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

    // planets
    inline int getNumStars() const { return stars->getNumStars(); }
    inline sgdVec3 *getStars() { return stars->getStars(); }
};


#endif // _EPHEMERIS_HXX


