// skymoon.hxx -- draw a moon object
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


#ifndef _SKYMOON_HXX_
#define _SKYMOON_HXX_


#include <plib/ssg.h>
#include <simgear/misc/fgpath.hxx>


class FGSkyMoon {

    // scene graph root for the skymoon
    ssgRoot *skymoon;

    ssgSelector *moon_selector;
    ssgTransform *moon_transform;
    ssgSimpleState *orb_state;
    ssgSimpleState *halo_state;

    ssgColourArray *cl;

    ssgVertexArray *halo_vl;
    ssgTexCoordArray *halo_tl;

    GLuint moon_texid;
    GLubyte *moon_texbuf;

public:

    // Constructor
    FGSkyMoon( void );

    // Destructor
    ~FGSkyMoon( void );

    // initialize the moon object and connect it into our scene graph
    // root
    bool initialize( const FGPath& path );

    // repaint the moon colors based on current value of moon_anglein
    // degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = moon rise/set
    // 180 degrees = darkest midnight
    bool repaint( double moon_angle );

    // reposition the moon at the specified right ascension and
    // declination, offset by our current position (p) so that it
    // appears fixed at a great distance from the viewer.  Also add in
    // an optional rotation (i.e. for the current time of day.)
    bool reposition( sgVec3 p, double angle,
		     double rightAscension, double declination );

    // Draw the moon
    bool draw();

    // enable the moon in the scene graph (default)
    void enable() { moon_selector->select( 1 ); }

    // disable the moon in the scene graph.  The leaf node is still
    // there, how ever it won't be traversed on the cullandrender
    // phase.
    void disable() { moon_selector->select( 0 ); }

};


#endif // _SKYMOON_HXX_














