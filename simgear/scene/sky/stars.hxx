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


#ifndef _SG_STARS_HXX_
#define _SG_STARS_HXX_


#include <plib/ssg.h>


class SGStars {

    ssgTransform *stars_transform;
    ssgSimpleState *state;

    ssgColourArray *cl;
    ssgVertexArray *vl;

    int old_phase;		// data for optimization

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
