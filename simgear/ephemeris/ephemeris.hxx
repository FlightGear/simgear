/** \file ephemeris.hxx
 * Top level class for calculating current positions of astronomical objects.
 */

// Top level interface written by Curtis Olson, started March 2000.
//
// All the core code underneath this is written by Durk Talsma.  See
// the headers of all the other individual files for details.
//
// Copyright (C) 2000  Curtis L. Olson - http://www.flightgear.org/~curt
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _EPHEMERIS_HXX
#define _EPHEMERIS_HXX

#include <string>

#include <simgear/ephemeris/star.hxx>
#include <simgear/ephemeris/moonpos.hxx>
#include <simgear/ephemeris/mercury.hxx>
#include <simgear/ephemeris/venus.hxx>
#include <simgear/ephemeris/mars.hxx>
#include <simgear/ephemeris/jupiter.hxx>
#include <simgear/ephemeris/saturn.hxx>
#include <simgear/ephemeris/uranus.hxx>
#include <simgear/ephemeris/neptune.hxx>
#include <simgear/ephemeris/stardata.hxx>

#include <simgear/math/SGMath.hxx>
#include <simgear/misc/sg_path.hxx>


/** Ephemeris class
 *
 * Written by Durk Talsma <d.talsma@direct.a2000.nl> and Curtis Olson
 * <http://www.flightgear.org/~curt>
 *
 * Introduction 
 *
 * The SGEphemeris class computes and stores the positions of the Sun,
 * the Moon, the planets, and the brightest stars.  These positions
 * can then be used to accurately render the dominant visible items in
 * the Earth's sky. Note, this class isn't intended for use in an
 * interplanetary/interstellar/intergalactic type application. It is
 * calculates everything relative to the Earth and is therefore best
 * suited for Earth centric applications.
 *
 * The positions of the various astronomical objects are time
 * dependent, so to maintain accuracy, you will need to periodically
 * call the update() method. The SGTime class conveniently provides
 * the two time related values you need to pass to the update()
 * method.
 */

class SGEphemeris {

    Star *our_sun;
    MoonPos *moon;
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
    SGVec3d planets[7];

    SGStarData *stars;

public:

    /**
     * Constructor.
     * This creates an instance of the SGEphemeris object. When
     * calling the constructor you need to provide a path pointing to
     * your star database file.
     * @param path path to your star database */
    SGEphemeris( const std::string &path );

    /** Destructor */
    ~SGEphemeris( void );

    /**
     * Update (recalculate) the positions of all objects for the
     * specified time.  The update() method requires you to pass in
     * the current modified Julian date, the current local sidereal
     * time, and the current latitude. The update() method is designed
     * to be called by the host application before every frame.
     * @param mjd modified julian date
     * @param lst current local sidereal time
     * @param lat current latitude
     */
    void update(double mjd, double lst, double lat);

    /**
     * @return a pointer to a Star class containing all the positional
     * information for Earth's Sun.
     */
    inline Star *get_sun() const { return our_sun; }

    /** @return the right ascension of the Sun. */
    inline double getSunRightAscension() const {
	return our_sun->getRightAscension();
    }

    /** @return the declination of the Sun. */
    inline double getSunDeclination() const {
	return our_sun->getDeclination();
    }

    /**
     * @return a pointer to a Moon class containing all the positional
     * information for Earth's Moon.
     */
    inline MoonPos *get_moon() const { return moon; }

    /** @return the right ascension of the Moon. */
    inline double getMoonRightAscension() const {
	return moon->getRightAscension();
    }

    /** @return the declination of the Moon. */
    inline double getMoonDeclination() const {
	return moon->getDeclination();
    }

    /** @return the numbers of defined planets. */
    inline int getNumPlanets() const { return nplanets; }

    /**
     * Returns a pointer to an array of planet data in sgdVec3
     * format. (See plib.sourceforge.net for information on plib and
     * the ``sg'' package.) An sgdVec3 is a 3 element double
     * array. The first element is the right ascension of the planet,
     * the second is the declination, and the third is the magnitude.
     * @return planets array
     */
    inline SGVec3d *getPlanets() { return planets; }
    inline const SGVec3d *getPlanets() const { return planets; }

    /** @return the numbers of defined stars. */
    inline int getNumStars() const { return stars->getNumStars(); }

    /**
     * Returns a pointer to an array of star data in sgdVec3
     * format. An The first element of the sgdVec3 is the right
     * ascension of the planet, the second is the declination, and the
     * third is the magnitude.
     * @returns star array
     */
    inline SGVec3d *getStars() { return stars->getStars(); }
    inline const SGVec3d *getStars() const { return stars->getStars(); }
};


#endif // _EPHEMERIS_HXX


