// ephemeris.cxx -- Top level class for calculating current positions of
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


#include "ephemeris.hxx"


// Constructor
FGEphemeris::FGEphemeris( void ) {
    our_sun = new Star;
    moon = new Moon;
    mercury = new Mercury;
    venus = new Venus;
    mars = new Mars;
    jupiter = new Jupiter;
    saturn = new Saturn;
    uranus = new Uranus;
    neptune = new Neptune;
}


// Destructor
FGEphemeris::~FGEphemeris( void ) {
    delete our_sun;
    delete moon;
    delete mercury;
    delete venus;
    delete mars;
    delete jupiter;
    delete saturn;
    delete uranus;
    delete neptune;
}


// Update (recalculate) the positions of all objects for the specified
// time
void FGEphemeris::update( FGTime *t ) {
    // update object positions
    our_sun->updatePosition( t );
    moon->updatePosition( t, our_sun );
    mercury->updatePosition( t, our_sun );
    venus->updatePosition( t, our_sun );
    mars->updatePosition( t, our_sun );
    jupiter->updatePosition( t, our_sun );
    saturn->updatePosition( t, our_sun );
    uranus->updatePosition( t, our_sun );
    neptune->updatePosition( t, our_sun );

    // update planets list
    mercury->getPos( &planets[0][0], &planets[0][1], &planets[0][2] );
    venus  ->getPos( &planets[1][0], &planets[1][1], &planets[1][2] );
    mars   ->getPos( &planets[2][0], &planets[2][1], &planets[2][2] );
    jupiter->getPos( &planets[3][0], &planets[3][1], &planets[3][2] );
    saturn ->getPos( &planets[4][0], &planets[4][1], &planets[4][2] );
    uranus ->getPos( &planets[5][0], &planets[5][1], &planets[5][2] );
    neptune->getPos( &planets[6][0], &planets[6][1], &planets[6][2] );
    
}

