// sky.cxx -- ssg based sky model
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/ssg.h>		// plib include

#include "sky.hxx"


// Constructor
SGSky::SGSky( void ) {
}


// Destructor
SGSky::~SGSky( void ) {
}


// initialize the sky and connect the components to the scene graph at
// the provided branch
void SGSky::build(  double sun_size, double moon_size,
		    int nplanets, sgdVec3 *planet_data,
		    double planet_dist,
		    int nstars, sgdVec3 *star_data, double star_dist )
{
    pre_root = new ssgRoot;
    post_root = new ssgRoot;

    pre_selector = new ssgSelector;
    post_selector = new ssgSelector;

    pre_transform = new ssgTransform;
    post_transform = new ssgTransform;

    dome = new SGSkyDome;
    pre_transform -> addKid( dome->build() );

    planets = new SGStars;
    pre_transform -> addKid( planets->build(nplanets, planet_data,
					    planet_dist)
			     );

    stars = new SGStars;
    pre_transform -> addKid( stars->build(nstars, star_data, star_dist) );
    
    moon = new SGMoon;
    pre_transform -> addKid( moon->build(tex_path, moon_size) );

    oursun = new SGSun;
    pre_transform -> addKid( oursun->build(tex_path, sun_size) );

    pre_selector->addKid( pre_transform );
    pre_selector->clrTraversalMaskBits( SSGTRAV_HOT );

    post_selector->addKid( post_transform );
    post_selector->clrTraversalMaskBits( SSGTRAV_HOT );

    pre_root->addKid( pre_selector );
    post_root->addKid( post_selector );
}


// repaint the sky components based on current value of sun_angle,
// sky, and fog colors.
//
// sun angle in degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool SGSky::repaint( sgVec4 sky_color, sgVec4 fog_color,
		     double sun_angle, double moon_angle,
		     int nplanets, sgdVec3 *planet_data,
		     int nstars, sgdVec3 *star_data )
{
    dome->repaint( sky_color, fog_color, sun_angle );
    oursun->repaint( sun_angle );
    moon->repaint( moon_angle );
    planets->repaint( sun_angle, nplanets, planet_data );
    stars->repaint( sun_angle, nstars, star_data );

    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
	cloud_layers[i]->repaint( fog_color );
    }

    return true;
}


// reposition the sky at the specified origin and orientation
//
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (this allows
// additional orientation for the sunrise/set effects and is used by
// the skydome and perhaps clouds.
bool SGSky::reposition( sgVec3 view_pos, sgVec3 zero_elev, sgVec3 view_up,
			double lon, double lat, double spin,
			double gst, 
			double sun_ra, double sun_dec, double sun_dist,
			double moon_ra, double moon_dec, double moon_dist )
{
    double angle = gst * 15;	// degrees
    dome->reposition( zero_elev, lon, lat, spin );
    oursun->reposition( view_pos, angle, sun_ra, sun_dec, sun_dist );
    moon->reposition( view_pos, angle, moon_ra, moon_dec, moon_dist );
    planets->reposition( view_pos, angle );
    stars->reposition( view_pos, angle );

    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
	cloud_layers[i]->reposition( zero_elev, view_up, lon, lat );
    }

    return true;
}


// draw background portions of the sky
void SGSky::draw_background() {
    ssgCullAndDraw( pre_root );
}


// draw scenery elements of the sky
void SGSky::draw_scene() {
    ssgCullAndDraw( post_root );
}

 
void SGSky::add_cloud_layer( double asl ) {
    SGCloudLayer *layer = new SGCloudLayer;
    post_transform -> addKid( layer->build(tex_path, 20000.0f, asl) );
    cloud_layers.push_back( layer );
}
