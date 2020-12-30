// ephemeris.cxx -- Top level class for calculating current positions of
//                  astronomical objects
//
// Written by Curtis Olson, started March 2000.
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <iostream>

#include "ephemeris.hxx"


// Constructor
SGEphemeris::SGEphemeris( const SGPath& path ) {
    our_sun = new Star;
    moon = new MoonPos;
    mercury = new Mercury;
    venus = new Venus;
    mars = new Mars;
    jupiter = new Jupiter;
    saturn = new Saturn;
    uranus = new Uranus;
    neptune = new Neptune;
    nplanets = 7;
    for ( int i = 0; i < nplanets; ++i )
      planets[i] = SGVec3d::zeros();
    stars = new SGStarData(path);
}


// Destructor
SGEphemeris::~SGEphemeris( void ) {
    delete our_sun;
    delete moon;
    delete mercury;
    delete venus;
    delete mars;
    delete jupiter;
    delete saturn;
    delete uranus;
    delete neptune;
    delete stars;
}


// Update (recalculate) the positions of all objects for the specified
// time
void SGEphemeris::update( double mjd, double lst, double lat ) {
    // update object positions
    our_sun->updatePosition( mjd );
    //    moon->updatePositionTopo( mjd, lst, lat, our_sun );
    moon->updatePosition( mjd, our_sun );
    mercury->updatePosition( mjd, our_sun );
    venus->updatePosition( mjd, our_sun );
    mars->updatePosition( mjd, our_sun );
    jupiter->updatePosition( mjd, our_sun );
    saturn->updatePosition( mjd, our_sun );
    uranus->updatePosition( mjd, our_sun );
    neptune->updatePosition( mjd, our_sun );

    // update planets list
    nplanets = 7;
    mercury->getPos( &planets[0][0], &planets[0][1], &planets[0][2] );
    venus  ->getPos( &planets[1][0], &planets[1][1], &planets[1][2] );
    mars   ->getPos( &planets[2][0], &planets[2][1], &planets[2][2] );
    jupiter->getPos( &planets[3][0], &planets[3][1], &planets[3][2] );
    saturn ->getPos( &planets[4][0], &planets[4][1], &planets[4][2] );
    uranus ->getPos( &planets[5][0], &planets[5][1], &planets[5][2] );
    neptune->getPos( &planets[6][0], &planets[6][1], &planets[6][2] );
}

