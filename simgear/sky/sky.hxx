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

#include <simgear/compiler.h>
#include <simgear/misc/fgpath.hxx>

#include <vector>

#include "cloud.hxx"
#include "dome.hxx"
#include "moon.hxx"
#include "oursun.hxx"
#include "stars.hxx"

FG_USING_STD(vector);


typedef vector < SGCloudLayer* > layer_list_type;
typedef layer_list_type::iterator layer_list_iterator;
typedef layer_list_type::const_iterator layer_list_const_iterator;


class SGSky {

private:

    // components of the sky
    SGSkyDome *dome;
    SGSun *oursun;
    SGMoon *moon;
    SGStars *planets;
    SGStars *stars;
    layer_list_type cloud_layers;

    ssgRoot *pre_root, *post_root;

    ssgSelector *pre_selector, *post_selector;
    ssgTransform *pre_transform, *post_transform;

    FGPath tex_path;

    // visibility
    float visibility;
    float effective_visibility;

    // near cloud visibility state variables
    bool in_puff;
    double puff_length;
    double puff_progression;
    double ramp_up;
    double ramp_down;

public:

    // Constructor
    SGSky( void );

    // Destructor
    ~SGSky( void );

    // initialize the sky and connect the components to the scene
    // graph at the provided branch
    void build( double sun_size, double moon_size,
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
    bool reposition( sgVec3 view_pos, sgVec3 zero_elev, sgVec3 view_up,
		     double lon, double lat, double alt, double spin,
		     double gst, 
		     double sun_ra, double sun_dec, double sun_dist,
		     double moon_ra, double moon_dec, double moon_dist );

    // modify the given visibility based on cloud layers, thickness,
    // transition range, and simulated "puffs".
    void modify_vis( float alt, float time_factor );

    // draw background portions of the sky
    void draw_background();

    // draw scenery elements of the sky
    void draw_scene( float alt );

    // specify the texture path (optional, defaults to current directory)
    inline void texture_path( const string& path ) {
	tex_path = FGPath( path );
    }

    // enable the sky
    inline void enable() {
	pre_selector->select( 1 );
	post_selector->select( 1 );
    }

    // disable the sky in the scene graph.  The leaf node is still
    // there, how ever it won't be traversed on by ssgCullandRender()
    inline void disable() {
	pre_selector->select( 0 );
	post_selector->select( 0 );
    }

    // add a cloud layer (above sea level in meters)
    void add_cloud_layer( double asl, double thickness, double transition );

    inline int get_num_layers() const { return cloud_layers.size(); }
    inline SGCloudLayer *get_cloud_layer( int i ) const {
	return cloud_layers[i];
    }

    inline float get_visibility() const { return effective_visibility; }
    inline void set_visibility( float v ) {
	effective_visibility = visibility = v;
    }
};


#endif // _SG_SKY_HXX


