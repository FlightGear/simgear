// stars.hxx -- model the stars
//
// Written by Durk Talsma. Originally started October 1997, for distribution  
// with the FlightGear project. Version 2 was written in August and 
// September 1998. This code is based upon algorithms and data kindly 
// provided by Mr. Paul Schlyter. (pausch@saaf.se). 
//
// Separated out rendering pieces and converted to ssg by Curt Olson,
// March 2000
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


#ifndef _SG_STARS_HXX_
#define _SG_STARS_HXX_


#include <plib/ssg.h>


class SGStars {

    ssgTransform *stars_transform;
    ssgSimpleState *state;

    ssgColourArray *cl;
    ssgVertexArray *vl;

public:

    // Constructor
    SGStars( void );

    // Destructor
    ~SGStars( void );

    // initialize the stars structure
    ssgBranch *build( int num, sgdVec3 *star_data, double star_dist );

    // repaint the planet magnitudes based on current value of
    // sun_angle in degrees relative to verticle (so we can make them
    // relatively dimmer during dawn and dusk
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( double sun_angle, int num, sgdVec3 *star_data );

    // reposition the stars for the specified time (GST rotation),
    // offset by our current position (p) so that it appears fixed at
    // a great distance from the viewer.
    bool reposition( sgVec3 p, double angle );
};


#endif // _SG_STARS_HXX_
