// dome.cxx -- model sky with an upside down "bowl"
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <math.h>

#include <simgear/compiler.h>

#include SG_GL_H

#include <plib/sg.h>

#include <simgear/debug/logstream.hxx>

#include "dome.hxx"


#ifdef __MWERKS__
#  pragma global_optimizer off
#endif


// proportions of max dimensions fed to the build() routine
static const float center_elev = 1.0;

static const float upper_radius = 0.6;
static const float upper_elev = 0.15;

static const float middle_radius = 0.9;
static const float middle_elev = 0.08;

static const float lower_radius = 1.0;
static const float lower_elev = 0.0;

static const float bottom_radius = 0.8;
static const float bottom_elev = -0.1;


// Set up dome rendering callbacks
static int sgSkyDomePreDraw( ssgEntity *e ) {
    /* cout << endl << "Dome Pre Draw" << endl << "----------------" 
	 << endl << endl; */

    ssgLeaf *f = (ssgLeaf *)e;
    if ( f -> hasState () ) f->getState()->apply() ;

    glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_FOG_BIT );
    // cout << "push error = " << glGetError() << endl;

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_FOG );

    return true;
}

static int sgSkyDomePostDraw( ssgEntity *e ) {
    /* cout << endl << "Dome Post Draw" << endl << "----------------" 
	 << endl << endl; */

    glPopAttrib();
    // cout << "pop error = " << glGetError() << endl;
    
    return true;
}


// Constructor
SGSkyDome::SGSkyDome( void ) {
    asl = 0.0f;
}


// Destructor
SGSkyDome::~SGSkyDome( void ) {
}


// initialize the sky object and connect it into our scene graph
ssgBranch * SGSkyDome::build( double hscale, double vscale ) {
    sgVec4 color;

    double theta;
    int i;

    // set up the state
    dome_state = new ssgSimpleState();
    dome_state->setShadeModel( GL_SMOOTH );
    dome_state->disable( GL_LIGHTING );
    dome_state->disable( GL_CULL_FACE );
    dome_state->disable( GL_TEXTURE_2D );
    dome_state->enable( GL_COLOR_MATERIAL );
    dome_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    dome_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    dome_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    dome_state->disable( GL_BLEND );
    dome_state->disable( GL_ALPHA_TEST );

    // initialize arrays
    center_disk_vl = new ssgVertexArray( 14 );
    center_disk_cl = new ssgColourArray( 14 );
				       
    upper_ring_vl = new ssgVertexArray( 26 );
    upper_ring_cl = new ssgColourArray( 26 );

    middle_ring_vl = new ssgVertexArray( 26 );
    middle_ring_cl = new ssgColourArray( 26 );

    lower_ring_vl = new ssgVertexArray( 26 );
    lower_ring_cl = new ssgColourArray( 26 );

    // initially seed to all blue
    sgSetVec4( color, 0.0, 0.0, 1.0, 1.0 );

    // generate the raw vertex data
    sgVec3 center_vertex;
    sgVec3 upper_vertex[12];
    sgVec3 middle_vertex[12];
    sgVec3 lower_vertex[12];
    sgVec3 bottom_vertex[12];

    sgSetVec3( center_vertex, 0.0, 0.0, center_elev * vscale );

    for ( i = 0; i < 12; i++ ) {
	theta = (i * 30.0) * SGD_DEGREES_TO_RADIANS;

	sgSetVec3( upper_vertex[i],
		   cos(theta) * upper_radius * hscale,
		   sin(theta) * upper_radius * hscale,
		   upper_elev * vscale );

	sgSetVec3( middle_vertex[i],
		   cos(theta) * middle_radius * hscale,
		   sin(theta) * middle_radius * hscale,
		   middle_elev * vscale );

	sgSetVec3( lower_vertex[i],
		   cos(theta) * lower_radius * hscale,
		   sin(theta) * lower_radius * hscale,
		   lower_elev * vscale );

	sgSetVec3( bottom_vertex[i],
		   cos(theta) * bottom_radius * hscale,
		   sin(theta) * bottom_radius * hscale,
		   bottom_elev * vscale );
    }

    // generate the center disk vertex/color arrays
    center_disk_vl->add( center_vertex );
    center_disk_cl->add( color );
    for ( i = 11; i >= 0; i-- ) {
	center_disk_vl->add( upper_vertex[i] );
	center_disk_cl->add( color );
    }
    center_disk_vl->add( upper_vertex[11] );
    center_disk_cl->add( color );

    // generate the upper ring
    for ( i = 0; i < 12; i++ ) {
	upper_ring_vl->add( middle_vertex[i] );
	upper_ring_cl->add( color );

	upper_ring_vl->add( upper_vertex[i] );
	upper_ring_cl->add( color );
    }
    upper_ring_vl->add( middle_vertex[0] );
    upper_ring_cl->add( color );

    upper_ring_vl->add( upper_vertex[0] );
    upper_ring_cl->add( color );

    // generate middle ring
    for ( i = 0; i < 12; i++ ) {
	middle_ring_vl->add( lower_vertex[i] );
	middle_ring_cl->add( color );

	middle_ring_vl->add( middle_vertex[i] );
	middle_ring_cl->add( color );
    }
    middle_ring_vl->add( lower_vertex[0] );
    middle_ring_cl->add( color );

    middle_ring_vl->add( middle_vertex[0] );
    middle_ring_cl->add( color );

    // generate lower ring
    for ( i = 0; i < 12; i++ ) {
	lower_ring_vl->add( bottom_vertex[i] );
	lower_ring_cl->add( color );

	lower_ring_vl->add( lower_vertex[i] );
	lower_ring_cl->add( color );
    }
    lower_ring_vl->add( bottom_vertex[0] );
    lower_ring_cl->add( color );

    lower_ring_vl->add( lower_vertex[0] );
    lower_ring_cl->add( color );

    // force a repaint of the sky colors with ugly defaults
    sgVec4 fog_color;
    sgSetVec4( fog_color, 1.0, 1.0, 1.0, 1.0 );
    repaint( color, fog_color, 0.0, 5000.0 );

    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    ssgVtxTable *center_disk, *upper_ring, *middle_ring, *lower_ring;

    center_disk = new ssgVtxTable( GL_TRIANGLE_FAN, 
				   center_disk_vl, NULL, NULL, center_disk_cl );

    upper_ring = new ssgVtxTable( GL_TRIANGLE_STRIP, 
				  upper_ring_vl, NULL, NULL, upper_ring_cl );

    middle_ring = new ssgVtxTable( GL_TRIANGLE_STRIP, 
				   middle_ring_vl, NULL, NULL, middle_ring_cl );

    lower_ring = new ssgVtxTable( GL_TRIANGLE_STRIP, 
				  lower_ring_vl, NULL, NULL, lower_ring_cl );

    center_disk->setState( dome_state );
    upper_ring->setState( dome_state );
    middle_ring->setState( dome_state );
    lower_ring->setState( dome_state );

    dome_transform = new ssgTransform;
    dome_transform->addKid( center_disk );
    dome_transform->addKid( upper_ring );
    dome_transform->addKid( middle_ring );
    dome_transform->addKid( lower_ring );

    // not entirely satisfying.  We are depending here that the first
    // thing we add to a parent is the first drawn
    center_disk->setCallback( SSG_CALLBACK_PREDRAW, sgSkyDomePreDraw );
    center_disk->setCallback( SSG_CALLBACK_POSTDRAW, sgSkyDomePostDraw );

    upper_ring->setCallback( SSG_CALLBACK_PREDRAW, sgSkyDomePreDraw );
    upper_ring->setCallback( SSG_CALLBACK_POSTDRAW, sgSkyDomePostDraw );

    middle_ring->setCallback( SSG_CALLBACK_PREDRAW, sgSkyDomePreDraw );
    middle_ring->setCallback( SSG_CALLBACK_POSTDRAW, sgSkyDomePostDraw );

    lower_ring->setCallback( SSG_CALLBACK_PREDRAW, sgSkyDomePreDraw );
    lower_ring->setCallback( SSG_CALLBACK_POSTDRAW, sgSkyDomePostDraw );

    return dome_transform;
}

static void fade_to_black( sgVec4 sky_color[], float asl, int count) {
    const float ref_asl = 10000.0f;
    const sgVec3 space_color = {0.0f, 0.0f, 0.0f};
    float d = exp( - asl / ref_asl );
    for(int i = 0; i < count ; i++)
        sgLerpVec3( sky_color[i], sky_color[i], space_color, 1.0 - d);
}

// repaint the sky colors based on current value of sun_angle, sky,
// and fog colors.  This updates the color arrays for ssgVtxTable.
// sun angle in degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool SGSkyDome::repaint( sgVec4 sky_color, sgVec4 fog_color, double sun_angle,
			 double vis )
{
    double diff, prev_sun_angle = 999.0;
    sgVec3 outer_param, outer_amt, outer_diff;
    sgVec3 middle_param, middle_amt, middle_diff;
    int i, j;

    if (prev_sun_angle == sun_angle)
        return true;

    prev_sun_angle = sun_angle;

    // Check for sunrise/sunset condition
    if (sun_angle > 80.0) // && (sun_angle < 100.0) )
    {
	// 0.0 - 0.4
	sgSetVec3( outer_param,
		   (10.0 - fabs(90.0 - sun_angle)) / 20.0,
		   (10.0 - fabs(90.0 - sun_angle)) / 40.0,
		   -(10.0 - fabs(90.0 - sun_angle)) / 30.0 );

	sgSetVec3( middle_param,
		   (10.0 - fabs(90.0 - sun_angle)) / 40.0,
		   (10.0 - fabs(90.0 - sun_angle)) / 80.0,
		   0.0 );

	sgScaleVec3( outer_diff, outer_param, 1.0 / 6.0 );

	sgScaleVec3( middle_diff, middle_param, 1.0 / 6.0 );
    } else {
	sgSetVec3( outer_param, 0.0, 0.0, 0.0 );
	sgSetVec3( middle_param, 0.0, 0.0, 0.0 );

	sgSetVec3( outer_diff, 0.0, 0.0, 0.0 );
	sgSetVec3( middle_diff, 0.0, 0.0, 0.0 );
    }
    // printf("  outer_red_param = %.2f  outer_red_diff = %.2f\n", 
    //        outer_red_param, outer_red_diff);

    // calculate transition colors between sky and fog
    sgCopyVec3( outer_amt, outer_param );
    sgCopyVec3( middle_amt, middle_param );

    //
    // First, recalulate the basic colors
    //

    sgVec4 center_color;
    sgVec4 upper_color[12];
    sgVec4 middle_color[12];
    sgVec4 lower_color[12];
    sgVec4 bottom_color[12];

    double vis_factor, cvf = vis;

    if (cvf > 45000)
        cvf = 45000;

    vis_factor = (vis - 1000.0) / 2000.0;
    if ( vis_factor < 0.0 ) {
        vis_factor = 0.0;
    } else if ( vis_factor > 1.0) {
        vis_factor = 1.0;
    }

    for ( j = 0; j < 3; j++ ) {
	diff = sky_color[j] - fog_color[j];
	center_color[j] = sky_color[j]; // - diff * ( 1.0 - vis_factor );
    }
    center_color[3] = 1.0;

    for ( i = 0; i < 6; i++ ) {
	for ( j = 0; j < 3; j++ ) {
            double saif = sun_angle/SG_PI;
	    diff = (sky_color[j] - fog_color[j]) * (0.8 + j * 0.2) * (0.8 + saif - ((6-i)/10));

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        l->sky_color[j], l->fog_color[j], diff);

	    upper_color[i][j] = sky_color[j] - diff *
                                 ( 1.0 - vis_factor * (0.7 + 0.3 * cvf/45000) );
	    middle_color[i][j] = sky_color[j] - diff *
                                 ( 1.0 - vis_factor * (0.1 + 0.85 * cvf/45000) )
		                 + middle_amt[j];
	    lower_color[i][j] = fog_color[j] + outer_amt[j];

	    if ( upper_color[i][j] > 1.0 ) { upper_color[i][j] = 1.0; }
	    if ( upper_color[i][j] < 0.0 ) { upper_color[i][j] = 0.0; }
	    if ( middle_color[i][j] > 1.0 ) { middle_color[i][j] = 1.0; }
	    if ( middle_color[i][j] < 0.0 ) { middle_color[i][j] = 0.0; }
	    if ( lower_color[i][j] > 1.0 ) { lower_color[i][j] = 1.0; }
	    if ( lower_color[i][j] < 0.0 ) { lower_color[i][j] = 0.0; }
	}
	upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 1.0;

	for ( j = 0; j < 3; j++ ) {
	    outer_amt[j] -= outer_diff[j];
	    middle_amt[j] -= middle_diff[j];
	}

	/*
	printf("upper_color[%d] = %.2f %.2f %.2f %.2f\n", i, upper_color[i][0],
	       upper_color[i][1], upper_color[i][2], upper_color[i][3]);
	printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	       middle_color[i][3]);
	printf("lower_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       lower_color[i][0], lower_color[i][1], lower_color[i][2], 
	       lower_color[i][3]);
	*/
    }

    sgSetVec3( outer_amt, 0.0, 0.0, 0.0 );
    sgSetVec3( middle_amt, 0.0, 0.0, 0.0 );

    for ( i = 6; i < 12; i++ ) {
	for ( j = 0; j < 3; j++ ) {
            double saif = sun_angle/SG_PI;
            diff = (sky_color[j] - fog_color[j]) * (0.8 + j * 0.2) * (0.8 + saif - ((-i+12)/10));

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        sky_color[j], fog_color[j], diff);

	    upper_color[i][j] = sky_color[j] - diff *
                                 ( 1.0 - vis_factor * (0.7 + 0.3 * cvf/45000) );
	    middle_color[i][j] = sky_color[j] - diff *
                                 ( 1.0 - vis_factor * (0.1 + 0.85 * cvf/45000) )
                                  + middle_amt[j];
	    lower_color[i][j] = fog_color[j] + outer_amt[j];

	    if ( upper_color[i][j] > 1.0 ) { upper_color[i][j] = 1.0; }
	    if ( upper_color[i][j] < 0.0 ) { upper_color[i][j] = 0.0; }
	    if ( middle_color[i][j] > 1.0 ) { middle_color[i][j] = 1.0; }
	    if ( middle_color[i][j] < 0.0 ) { middle_color[i][j] = 0.0; }
	    if ( lower_color[i][j] > 1.0 ) { lower_color[i][j] = 1.0; }
	    if ( lower_color[i][j] < 0.0 ) { lower_color[i][j] = 0.0; }
	}
	upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 1.0;

	for ( j = 0; j < 3; j++ ) {
	    outer_amt[j] += outer_diff[j];
	    middle_amt[j] += middle_diff[j];
	}

	/*
	printf("upper_color[%d] = %.2f %.2f %.2f %.2f\n", i, upper_color[i][0],
	       upper_color[i][1], upper_color[i][2], upper_color[i][3]);
	printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	       middle_color[i][3]);
	printf("lower_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       lower_color[i][0], lower_color[i][1], lower_color[i][2], 
	       lower_color[i][3]);
 	*/
   }

    fade_to_black( (sgVec4 *) center_color, asl * center_elev, 1);
    fade_to_black( upper_color, (asl+0.05f) * upper_elev, 12);
    fade_to_black( middle_color, (asl+0.05f) * middle_elev, 12);
    fade_to_black( lower_color, (asl+0.05f) * lower_elev, 12);

    for ( i = 0; i < 12; i++ ) {
	sgCopyVec4( bottom_color[i], fog_color );
    }

    //
    // Second, assign the basic colors to the object color arrays
    //

    float *slot;
    int counter;

    // update the center disk color arrays
    counter = 0;
    slot = center_disk_cl->get( counter++ );
    // sgVec4 red;
    // sgSetVec4( red, 1.0, 0.0, 0.0, 1.0 );
    sgCopyVec4( slot, center_color );
    for ( i = 11; i >= 0; i-- ) {
	slot = center_disk_cl->get( counter++ );
	sgCopyVec4( slot, upper_color[i] );
    }
    slot = center_disk_cl->get( counter++ );
    sgCopyVec4( slot, upper_color[11] );

    // generate the upper ring
    counter = 0;
    for ( i = 0; i < 12; i++ ) {
	slot = upper_ring_cl->get( counter++ );
	sgCopyVec4( slot, middle_color[i] );

	slot = upper_ring_cl->get( counter++ );
	sgCopyVec4( slot, upper_color[i] );
    }
    slot = upper_ring_cl->get( counter++ );
    sgCopyVec4( slot, middle_color[0] );

    slot = upper_ring_cl->get( counter++ );
    sgCopyVec4( slot, upper_color[0] );

    // generate middle ring
    counter = 0;
    for ( i = 0; i < 12; i++ ) {
	slot = middle_ring_cl->get( counter++ );
	sgCopyVec4( slot, lower_color[i] );

	slot = middle_ring_cl->get( counter++ );
	sgCopyVec4( slot, middle_color[i] );
    }
    slot = middle_ring_cl->get( counter++ );
    sgCopyVec4( slot, lower_color[0] );

    slot = middle_ring_cl->get( counter++ );
    sgCopyVec4( slot, middle_color[0] );

    // generate lower ring
    counter = 0;
    for ( i = 0; i < 12; i++ ) {
	slot = lower_ring_cl->get( counter++ );
	sgCopyVec4( slot, bottom_color[i] );

	slot = lower_ring_cl->get( counter++ );
	sgCopyVec4( slot, lower_color[i] );
    }
    slot = lower_ring_cl->get( counter++ );
    sgCopyVec4( slot, bottom_color[0] );

    slot = lower_ring_cl->get( counter++ );
    sgCopyVec4( slot, lower_color[0] );

    return true;
}


// reposition the sky at the specified origin and orientation
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (and orients the
// sunrise/set effects
bool SGSkyDome::reposition( sgVec3 p, double lon, double lat, double spin ) {
    sgMat4 T, LON, LAT, SPIN;
    sgVec3 axis;

    // Translate to view position
    // Point3D zero_elev = current_view.get_cur_zero_elev();
    // xglTranslatef( zero_elev.x(), zero_elev.y(), zero_elev.z() );
    sgMakeTransMat4( T, p );

    // printf("  Translated to %.2f %.2f %.2f\n", 
    //        zero_elev.x, zero_elev.y, zero_elev.z );

    // Rotate to proper orientation
    // printf("  lon = %.2f  lat = %.2f\n",
    //        lon * SGD_RADIANS_TO_DEGREES,
    //        lat * SGD_RADIANS_TO_DEGREES);
    // xglRotatef( lon * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0 );
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( LON, lon * SGD_RADIANS_TO_DEGREES, axis );

    // xglRotatef( 90.0 - f->get_Latitude() * SGD_RADIANS_TO_DEGREES,
    //             0.0, 1.0, 0.0 );
    sgSetVec3( axis, 0.0, 1.0, 0.0 );
    sgMakeRotMat4( LAT, 90.0 - lat * SGD_RADIANS_TO_DEGREES, axis );

    // xglRotatef( l->sun_rotation * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0 );
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( SPIN, spin * SGD_RADIANS_TO_DEGREES, axis );

    sgMat4 TRANSFORM;

    sgCopyMat4( TRANSFORM, T );
    sgPreMultMat4( TRANSFORM, LON );
    sgPreMultMat4( TRANSFORM, LAT );
    sgPreMultMat4( TRANSFORM, SPIN );

    sgCoord skypos;
    sgSetCoord( &skypos, TRANSFORM );

    dome_transform->setTransform( &skypos );
    asl = - skypos.xyz[2];
    return true;
}
