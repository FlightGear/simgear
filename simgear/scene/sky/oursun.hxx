// oursun.hxx -- model earth's sun
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


#ifndef _SG_SUN_HXX_
#define _SG_SUN_HXX_


#include <plib/ssg.h>

#include <simgear/misc/sg_path.hxx>

class SGSun {

    ssgTransform *sun_transform;
    ssgSimpleState *orb_state;
    ssgSimpleState *halo_state;

    ssgColourArray *cl;

    ssgVertexArray *halo_vl;
    ssgTexCoordArray *halo_tl;

    GLuint sun_texid;
    GLubyte *sun_texbuf;

    double visibility;
    double prev_sun_angle;

public:

    // Constructor
    SGSun( void );

    // Destructor
    ~SGSun( void );

    // return the sun object
    ssgBranch *build( SGPath path, double sun_size );

    // repaint the sun colors based on current value of sun_anglein
    // degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( double sun_angle, double new_visibility );

    // reposition the sun at the specified right ascension and
    // declination, offset by our current position (p) so that it
    // appears fixed at a great distance from the viewer.  Also add in
    // an optional rotation (i.e. for the current time of day.)
    bool reposition( sgVec3 p, double angle,
		     double rightAscension, double declination,
		     double sun_dist );

    // retrun the current color of the sun
    inline float *get_color() { return  cl->get( 0 ); }

    // return the texture id of the sun halo texture
    inline GLuint get_texture_id() { return halo_state->getTextureHandle(); }
};


#endif // _SG_SUN_HXX_
