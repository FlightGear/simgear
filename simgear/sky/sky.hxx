// sky.hxx -- ssg based sky model
//
// Written by Curtis Olson, started December 1997.
// SSG-ified by Curtis Olson, February 2000.
//
// Copyright (C) 1997-2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _SG_SKY_HXX
#define _SG_SKY_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <plib/ssg.h>		// plib include

#include <simgear/misc/fgpath.hxx>

#include "dome.hxx"
#include "moon.hxx"
#include "oursun.hxx"
#include "stars.hxx"


class SGSky {

    // components of the sky
    SGSkyDome *dome;
    SGSun *oursun;
    SGMoon *moon;
    SGStars *planets;
    SGStars *stars;

    ssgSelector *sky_selector;
    ssgTransform *sky_transform;

    FGPath tex_path;

public:

    // Constructor
    SGSky( void );

    // Destructor
    ~SGSky( void );

    // initialize the sky and connect the components to the scene
    // graph at the provided branch
    ssgBranch *build( double sun_size, double moon_size,
		      int nplanets, sgdVec3 *planet_data, double planet_dist,
		      int nstars, sgdVec3 *star_data, double star_dist );

    // repaint the sky components based on current value of sun_angle,
    // sky, and fog colors.
    //
    // sun angle in degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( sgVec4 sky_color, sgVec4 fog_color, 
		  double sun_angle, double moon_angle,
		  int nplanets, sgdVec3 *planet_data,
		  int nstars, sgdVec3 *star_data );

    // reposition the sky at the specified origin and orientation
    //
    // lon specifies a rotation about the Z axis
    // lat specifies a rotation about the new Y axis
    // spin specifies a rotation about the new Z axis (this allows
    // additional orientation for the sunrise/set effects and is used
    // by the skydome and perhaps clouds.
    bool reposition( sgVec3 view_pos, sgVec3 zero_elev, 
		     double lon, double lat, double spin,
		     double gst, 
		     double sun_ra, double sun_dec, double sun_dist,
		     double moon_ra, double moon_dec, double moon_dist );

    // specify the texture path (optional, defaults to current directory)
    inline void texture_path( const string& path ) {
	tex_path = FGPath( path );
    }

    // enable the sky in the scene graph (default)
    inline void enable() { sky_selector->select( 1 ); }

    // disable the sky in the scene graph.  The leaf node is still
    // there, how ever it won't be traversed on by ssgCullandRender()
    inline void disable() { sky_selector->select( 0 ); }
};


#endif // _SG_SKY_HXX


