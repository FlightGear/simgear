// sky.cxx -- ssg based sky model
//
// Written by Curtis Olson, started December 1997.
// SSG-ified by Curtis Olson, February 2000.
//
// Copyright (C) 1997-2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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

    in_cloud  = -1;
}


// Destructor
SGSky::~SGSky( void )
{
    for (unsigned int i = 0; i < cloud_layers.size(); i++)
        delete cloud_layers[i];
}


// initialize the sky and connect the components to the scene graph at
// the provided branch
void SGSky::build( double h_radius_m, double v_radius_m,
                   double sun_size, double moon_size,
		   int nplanets, sgdVec3 *planet_data,
		   int nstars, sgdVec3 *star_data )
{
    pre_root = new ssgRoot;
    post_root = new ssgRoot;

    pre_selector = new ssgSelector;
    post_selector = new ssgSelector;

    pre_transform = new ssgTransform;
    post_transform = new ssgTransform;

    dome = new SGSkyDome;
    pre_transform -> addKid( dome->build( h_radius_m, v_radius_m ) );

    planets = new SGStars;
    pre_transform -> addKid(planets->build(nplanets, planet_data, h_radius_m));

    stars = new SGStars;
    pre_transform -> addKid( stars->build(nstars, star_data, h_radius_m) );
    
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
bool SGSky::repaint( const SGSkyColor &sc )
{
    if ( effective_visibility > 1000.0 ) {
	enable();
	dome->repaint( sc.sky_color, sc.fog_color, sc.sun_angle,
                       effective_visibility );

        stars->repaint( sc.sun_angle, sc.nstars, sc.star_data );
        planets->repaint( sc.sun_angle, sc.nplanets, sc.planet_data );

	oursun->repaint( sc.sun_angle, effective_visibility );
	moon->repaint( sc.moon_angle );

	for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
            if (cloud_layers[i]->getCoverage() != SGCloudLayer::SG_CLOUD_CLEAR){
                cloud_layers[i]->repaint( sc.cloud_color );
            }
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
bool SGSky::reposition( SGSkyState &st, double dt )
{

    double angle = st.gst * 15;	// degrees

    dome->reposition( st.zero_elev, st.lon, st.lat, st.spin );

    stars->reposition( st.view_pos, angle );
    planets->reposition( st.view_pos, angle );

    oursun->reposition( st.view_pos, angle,
                        st.sun_ra, st.sun_dec, st.sun_dist );

    moon->reposition( st.view_pos, angle,
                      st.moon_ra, st.moon_dec, st.moon_dist );

    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
        if ( cloud_layers[i]->getCoverage() != SGCloudLayer::SG_CLOUD_CLEAR ) {
            cloud_layers[i]->reposition( st.zero_elev, st.view_up,
                                         st.lon, st.lat, st.alt, dt );
        }
    }

    return true;
}


// draw background portions of the sky ... do this before you draw the
// rest of your scene.
void SGSky::preDraw( float alt, float fog_exp2_density ) {
    ssgCullAndDraw( pre_root );

    	// if we are closer than this to a cloud layer, don't draw clouds
    static const float slop = 5.0;
    int i;

    // check where we are relative to the cloud layers
    in_cloud = -1;
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
    cur_layer_pos = 0;
    while ( cur_layer_pos < (int)cloud_layers.size() &&
           alt > cloud_layers[cur_layer_pos]->getElevation_m())
    {
       ++cur_layer_pos;
    }

    // FIXME: This should not be needed, but at this time (08/15/2003)
    //        certain NVidia drivers don't seem to implement
    //        glPushAttrib(FG_FOG_BIT) properly. The result is that
    //        there is not fog when looking at the sun.
    glFogf ( GL_FOG_DENSITY, fog_exp2_density );
}

void SGSky::drawUpperClouds( ) {
    // draw the cloud layers that are above us, top to bottom
    for ( int i = (int)cloud_layers.size() - 1; i >= cur_layer_pos; --i ) {
        if ( i != in_cloud ) {
            cloud_layers[i]->draw( false );
        }
    }
}


// draw translucent clouds ... do this after you've drawn all the
// oapaque elements of your scene.
void SGSky::drawLowerClouds() {

    // draw the cloud layers that are below us, bottom to top
    for ( int i = 0; i < cur_layer_pos; ++i ) {
        if ( i != in_cloud ) {
            cloud_layers[i]->draw( true );
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

        if ( cloud_layers[i]->getCoverage() == SGCloudLayer::SG_CLOUD_CLEAR || cloud_layers[i]->getCoverage() == SGCloudLayer::SG_CLOUD_FEW || cloud_layers[i]->getCoverage() == SGCloudLayer::SG_CLOUD_SCATTERED) {
	    // less than 50% coverage -- assume we're in the clear for now
	    ratio = 1.0;
        } else if ( alt < asl - transition ) {
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
