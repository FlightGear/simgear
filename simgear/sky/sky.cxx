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

#include <simgear/constants.h>
#include <simgear/math/fg_random.h>

#include <Objects/matlib.hxx>

#include "sky.hxx"


// Constructor
SGSky::SGSky( void ) {
    effective_visibility = visibility = 10000.0;

    // near cloud visibility state variables
    in_puff = false;
    puff_length = 0;
    puff_progression = 0;
    ramp_up = 0.15;
    ramp_down = 0.15;
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

    // add the cloud ssgStates to the material lib
    FGPath cloud_path;
    ssgSimpleState *cloud_state;

    cloud_path.set( tex_path.str() );
    cloud_path.append( "cirrus.rgba" );
    cloud_state = SGCloudMakeState( cloud_path.str() );
    material_lib.add_item( "CloudCirrus", cloud_state );

    cloud_path.set( tex_path.str() );
    cloud_path.append( "mostlycloudy.rgba" );
    cloud_state = SGCloudMakeState( cloud_path.str() );
    material_lib.add_item( "CloudMostlyCloudy", cloud_state );

    cloud_path.set( tex_path.str() );
    cloud_path.append( "mostlysunny.rgba" );
    cloud_state = SGCloudMakeState( cloud_path.str() );
    material_lib.add_item( "CloudMostlySunny", cloud_state );

    cloud_path.set( tex_path.str() );
    cloud_path.append( "overcast.rgb" );
    cloud_state = SGCloudMakeState( cloud_path.str() );
    material_lib.add_item( "CloudOvercast", cloud_state );
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
    if ( effective_visibility > 1000.0 ) {
	enable();
	dome->repaint( sky_color, fog_color, sun_angle );
	oursun->repaint( sun_angle );
	moon->repaint( moon_angle );
	planets->repaint( sun_angle, nplanets, planet_data );
	stars->repaint( sun_angle, nstars, star_data );

	for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
	    cloud_layers[i]->repaint( fog_color );
	}
    } else {
	// turn off sky
	disable();
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
			double lon, double lat, double alt, double spin,
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
	cloud_layers[i]->reposition( zero_elev, view_up, lon, lat, alt );
    }

    return true;
}


// draw background portions of the sky
void SGSky::draw_background() {
    ssgCullAndDraw( pre_root );
}


// draw scenery elements of the sky
void SGSky::draw_scene( float alt ) {

    if ( effective_visibility < 4000.0 ) {
	// bail and don't draw clouds
	return;
    }

    // determine rendering order
    int pos = 0;
    while ( pos < (int)cloud_layers.size() && 
	    alt > cloud_layers[pos]->get_asl())
    {
	++pos;
    }

    if ( pos == 0 ) {
	// we are below all the cloud layers, draw top to bottom
	for ( int i = cloud_layers.size() - 1; i >= 0; --i ) {
	    cloud_layers[i]->draw();
	}
    } else if ( pos >= (int)cloud_layers.size() ) {
	// we are above all the cloud layers, draw bottom to top
	for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
	    cloud_layers[i]->draw();
	}
    } else {
	// we are between cloud layers, draw lower layers bottom to
	// top and upper layers top to bottom
	for ( int i = 0; i < pos; ++i ) {
	    cloud_layers[i]->draw();
	}
	for ( int i = cloud_layers.size() - 1; i >= pos; --i ) {
	    cloud_layers[i]->draw();
	}
    }
}

 
void SGSky::add_cloud_layer( double asl, double thickness, double transition,
			     SGCloudType type ) {
    SGCloudLayer *layer = new SGCloudLayer;
    layer->build(tex_path, 40000.0f, asl, thickness, transition, type );

    layer_list_iterator current = cloud_layers.begin();
    layer_list_iterator last = cloud_layers.end();
    while ( current != last && (*current)->get_asl() < asl ) {
	++current;
    }

    if ( current != last ) {
	cloud_layers.insert( current, layer );
    } else {
	cloud_layers.push_back( layer );
    }

    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
	cout << "layer " << i << " = " << cloud_layers[i]->get_asl() << endl;
    }
    cout << endl;
}


// modify the current visibility based on cloud layers, thickness,
// transition range, and simulated "puffs".
void SGSky::modify_vis( float alt, float time_factor ) {
    float effvis = visibility;

    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
	float asl = cloud_layers[i]->get_asl();
	float thickness = cloud_layers[i]->get_thickness();
	float transition = cloud_layers[i]->get_transition();

	double ratio = 1.0;

	if ( alt < asl - transition ) {
	    // below cloud layer
	    ratio = 1.0;
	} else if ( alt < asl ) {
	    // in lower transition
	    ratio = (asl - alt) / transition;
	} else if ( alt < asl + thickness ) {
	    // in cloud layer
	    ratio = 0.0;
	} else if ( alt < asl + thickness + transition ) {
	    // in upper transition
	    ratio = (alt - (asl + thickness)) / transition;
	} else {
	    // above cloud layer
	    ratio = 1.0;
	}

	// accumulate effects from multiple cloud layers
	effvis *= ratio;

	if ( ratio < 1.0 ) {
	    if ( ! in_puff ) {
		// calc chance of entering cloud puff
		double rnd = fg_random();
		double chance = rnd * rnd * rnd;
		if ( chance > 0.95 /* * (diff - 25) / 50.0 */ ) {
		    in_puff = true;
		    do {
			puff_length = fg_random() * 2.0; // up to 2 seconds
		    } while ( puff_length <= 0.0 );
		    puff_progression = 0.0;
		}
	    }

	    if ( in_puff ) {
		// modify actual_visibility based on puff envelope
	    
		if ( puff_progression <= ramp_up ) {
		    double x = FG_PI_2 * puff_progression / ramp_up;
		    double factor = 1.0 - sin( x );
		    effvis = effvis * factor;
		} else if ( puff_progression >= ramp_up + puff_length ) {
		    double x = FG_PI_2 * 
			(puff_progression - (ramp_up + puff_length)) /
			ramp_down;
		    double factor = sin( x );
		    effvis = effvis * factor;
		} else {
		    effvis = 0.0;
		}

		/* cout << "len = " << puff_length
		   << "  x = " << x 
		   << "  factor = " << factor
		   << "  actual_visibility = " << actual_visibility 
		   << endl; */

		// time_factor = ( global_multi_loop * 
		//                 current_options.get_speed_up() ) /
		//                (double)current_options.get_model_hz();

		puff_progression += time_factor;

		/* cout << "gml = " << global_multi_loop 
		   << "  speed up = " << current_options.get_speed_up()
		   << "  hz = " << current_options.get_model_hz() << endl;
		   */ 

		if ( puff_progression > puff_length + ramp_up + ramp_down) {
		    in_puff = false; 
		}
	    }

	    // never let visibility drop below zero
	    if ( effvis <= 0 ) {
		effvis = 0.1;
	    }
	}
    } // for

    effective_visibility = effvis;
}

