// sky.cxx -- ssg based sky model
//
// Written by Curtis Olson, started December 1997.
// SSG-ified by Curtis Olson, February 2000.
//
// Copyright (C) 1997-2000  Curtis L. Olson  - curt@flightgear.org
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


#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/sg_random.h>

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
    // ramp_up = 4.0;
    // ramp_down = 4.0;
}


// Destructor
SGSky::~SGSky( void )
{
    for (unsigned int i = 0; i < cloud_layers.size(); i++)
        delete cloud_layers[i];
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
    if ( effective_visibility > 1000.0 ) {
	enable();
	dome->repaint( sky_color, fog_color, sun_angle, effective_visibility );
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


// draw background portions of the sky ... do this before you draw the
// rest of your scene.
void SGSky::preDraw() {
    ssgCullAndDraw( pre_root );
}


// draw translucent clouds ... do this after you've drawn all the
// oapaque elements of your scene.
void SGSky::postDraw( float alt ) {
    float slop = 5.0;		// if we are closer than this to a cloud layer,
				// don't draw clouds

    int in_cloud = -1;		// cloud we are in

    int i;

    // check where we are relative to the cloud layers
    for ( i = 0; i < (int)cloud_layers.size(); ++i ) {
	float asl = cloud_layers[i]->getElevation_m();
	float thickness = cloud_layers[i]->getThickness_m();

	if ( alt < asl - slop ) {
	    // below cloud layer
	} else if ( alt < asl + thickness + slop ) {
	    // in cloud layer

	    // bail now and don't draw any clouds
	    in_cloud = i;
	} else {
	    // above cloud layer
	}
    }

    // determine rendering order
    int pos = 0;
    while ( pos < (int)cloud_layers.size() && 
	    alt > cloud_layers[pos]->getElevation_m())
    {
	++pos;
    }

    if ( pos == 0 ) {
	// we are below all the cloud layers, draw top to bottom
	for ( i = cloud_layers.size() - 1; i >= 0; --i ) {
	    if ( i != in_cloud ) {
		cloud_layers[i]->draw();
	    }
	}
    } else if ( pos >= (int)cloud_layers.size() ) {
	// we are above all the cloud layers, draw bottom to top
	for ( i = 0; i < (int)cloud_layers.size(); ++i ) {
	    if ( i != in_cloud ) {
		cloud_layers[i]->draw();
	    }
	}
    } else {
	// we are between cloud layers, draw lower layers bottom to
	// top and upper layers top to bottom
	for ( i = 0; i < pos; ++i ) {
	    if ( i != in_cloud ) {
		cloud_layers[i]->draw();
	    }
	}
	for ( i = cloud_layers.size() - 1; i >= pos; --i ) {
	    if ( i != in_cloud ) {
		cloud_layers[i]->draw();
	    }
	}
    }
}

void
SGSky::add_cloud_layer( SGCloudLayer * layer )
{
    cloud_layers.push_back(layer);
}

const SGCloudLayer *
SGSky::get_cloud_layer (int i) const
{
    return cloud_layers[i];
}

SGCloudLayer *
SGSky::get_cloud_layer (int i)
{
    return cloud_layers[i];
}

int
SGSky::get_cloud_layer_count () const
{
    return cloud_layers.size();
}

// modify the current visibility based on cloud layers, thickness,
// transition range, and simulated "puffs".
void SGSky::modify_vis( float alt, float time_factor ) {
    float effvis = visibility;

    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
	float asl = cloud_layers[i]->getElevation_m();
	float thickness = cloud_layers[i]->getThickness_m();
	float transition = cloud_layers[i]->getTransition_m();

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
		double rnd = sg_random();
		double chance = rnd * rnd * rnd;
		if ( chance > 0.95 /* * (diff - 25) / 50.0 */ ) {
		    in_puff = true;
		    puff_length = sg_random() * 2.0; // up to 2 seconds
		    puff_progression = 0.0;
		}
	    }

	    if ( in_puff ) {
		// modify actual_visibility based on puff envelope

		if ( puff_progression <= ramp_up ) {
		    double x = 0.5 * SGD_PI * puff_progression / ramp_up;
		    double factor = 1.0 - sin( x );
		    // cout << "ramp up = " << puff_progression
		    //      << "  factor = " << factor << endl;
		    effvis = effvis * factor;
		} else if ( puff_progression >= ramp_up + puff_length ) {
		    double x = 0.5 * SGD_PI * 
			(puff_progression - (ramp_up + puff_length)) /
			ramp_down;
		    double factor = sin( x );
		    // cout << "ramp down = " 
		    //      << puff_progression - (ramp_up + puff_length) 
		    //      << "  factor = " << factor << endl;
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
		// cout << "time factor = " << time_factor << endl;

		/* cout << "gml = " << global_multi_loop 
		   << "  speed up = " << current_options.get_speed_up()
		   << "  hz = " << current_options.get_model_hz() << endl;
		   */ 

		if ( puff_progression > puff_length + ramp_up + ramp_down) {
		    in_puff = false; 
		}
	    }

	    // never let visibility drop below 25 meters
	    if ( effvis <= 25.0 ) {
		effvis = 25.0;
	    }
	}
    } // for

    effective_visibility = effvis;
}
