// skydome.cxx -- model sky with an upside down "bowl"
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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <math.h>

#include <GL/glut.h>
#include <simgear/xgl/xgl.h>

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/fg_random.h>

#include <Aircraft/aircraft.hxx>
#include <FDM/flight.hxx>
#include <Main/views.hxx>
#include <Time/event.hxx>
#include <Time/fg_time.hxx>

#include "skydome.hxx"


#ifdef __MWERKS__
#  pragma global_optimizer off
#endif


// in meters of course
#define CENTER_ELEV   25000.0

#define UPPER_RADIUS  50000.0
#define UPPER_ELEV    20000.0

#define MIDDLE_RADIUS 70000.0
#define MIDDLE_ELEV    8000.0

#define LOWER_RADIUS  80000.0
#define LOWER_ELEV        0.0

#define BOTTOM_RADIUS 50000.0
#define BOTTOM_ELEV   -2000.0


// static float inner_vertex[12][3];
// static float middle_vertex[12][3];
// static float outer_vertex[12][3];
// static float bottom_vertex[12][3];

// static GLubyte upper_color[12][4];
// static GLubyte middle_color[12][4];
// static GLubyte lower_color[12][4];


// Defined the shared sky object here
FGSkyDome current_sky;


// Constructor
FGSkyDome::FGSkyDome( void ) {
}


// Destructor
FGSkyDome::~FGSkyDome( void ) {
}


// initialize the sky object and connect it into the scene graph
bool FGSkyDome::initialize( ssgRoot *root ) {
    sgVec3 color;

    float theta;
    int i;

    // set up the state
    sky_state = new ssgSimpleState();
    if ( current_options.get_shading() == 1 ) {
	sky_state->setShadeModel( GL_SMOOTH );
    } else {
	sky_state->setShadeModel( GL_FLAT );
    }
    sky_state->disable( GL_LIGHTING );
    sky_state->disable( GL_DEPTH_TEST );
    sky_state->disable( GL_CULL_FACE );
    sky_state->disable( GL_TEXTURE );
    sky_state->enable( GL_COLOR_MATERIAL );
    sky_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );

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
    sgSetVec3( color, 0.0, 0.0, 1.0 );

    // generate the raw vertex data
    sgVec3 center_vertex;
    sgVec3 upper_vertex[12];
    sgVec3 middle_vertex[12];
    sgVec3 lower_vertex[12];
    sgVec3 bottom_vertex[12];

    sgSetVec3( center_vertex, 0.0, 0.0, CENTER_ELEV );

    for ( i = 0; i < 12; i++ ) {
	theta = (i * 30.0) * DEG_TO_RAD;

	sgSetVec3( upper_vertex[i],
		   cos(theta) * UPPER_RADIUS,
		   sin(theta) * UPPER_RADIUS,
		   UPPER_ELEV );
	
	sgSetVec3( middle_vertex[i],
		   cos((double)theta) * MIDDLE_RADIUS,
		   sin((double)theta) * MIDDLE_RADIUS,
		   MIDDLE_ELEV );

	sgSetVec3( lower_vertex[i],
		   cos((double)theta) * LOWER_RADIUS,
		   sin((double)theta) * LOWER_RADIUS,
		   LOWER_ELEV );

	sgSetVec3( bottom_vertex[i],
		   cos((double)theta) * BOTTOM_RADIUS,
		   sin((double)theta) * BOTTOM_RADIUS,
		   BOTTOM_ELEV );
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
    sgVec3 fog_color;
    sgSetVec3( fog_color, 1.0, 1.0, 1.0 );
    repaint( color, fog_color, 0.0 );

    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    sky_selector = new ssgSelector;
    sky_transform = new ssgTransform;

    ssgVtxTable *center_disk, *upper_ring, *middle_ring, *lower_ring;

    center_disk = new ssgVtxTable( GL_TRIANGLE_FAN, 
				   center_disk_vl, NULL, NULL, center_disk_cl );

    upper_ring = new ssgVtxTable( GL_TRIANGLE_STRIP, 
				  upper_ring_vl, NULL, NULL, upper_ring_cl );

    middle_ring = new ssgVtxTable( GL_TRIANGLE_STRIP, 
				   middle_ring_vl, NULL, NULL, middle_ring_cl );

    lower_ring = new ssgVtxTable( GL_TRIANGLE_STRIP, 
				  lower_ring_vl, NULL, NULL, lower_ring_cl );

    center_disk->setState( sky_state );
    upper_ring->setState( sky_state );
    middle_ring->setState( sky_state );
    lower_ring->setState( sky_state );

    sky_transform->addKid( center_disk );
    sky_transform->addKid( upper_ring );
    sky_transform->addKid( middle_ring );
    sky_transform->addKid( lower_ring );

    sky_selector->addKid( sky_transform );
    sky_selector->clrTraversalMaskBits( SSGTRAV_HOT );

    root->addKid( sky_selector );

    return true;
}


// repaint the sky colors based on current value of sun_angle, sky,
// and fog colors.  This updates the color arrays for ssgVtxTable.
// sun angle in degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool FGSkyDome::repaint( sgVec3 sky_color, sgVec3 fog_color, double sun_angle ) {
    double diff;
    sgVec3 outer_param, outer_amt, outer_diff;
    sgVec3 middle_param, middle_amt, middle_diff;
    int i, j;

    // Check for sunrise/sunset condition
    if ( (sun_angle > 80.0) && (sun_angle < 100.0) ) {
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

    sgVec3 upper_color[12];
    sgVec3 middle_color[12];
    sgVec3 lower_color[12];
    sgVec3 bottom_color[12];

    for ( i = 0; i < 6; i++ ) {
	for ( j = 0; j < 3; j++ ) {
	    diff = sky_color[j] - fog_color[j];

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        l->sky_color[j], l->fog_color[j], diff);

	    upper_color[i][j] = sky_color[j] - diff * 0.3;
	    middle_color[i][j] = sky_color[j] - diff * 0.9 + middle_amt[j];
	    lower_color[i][j] = fog_color[j] + outer_amt[j];

	    if ( upper_color[i][j] > 1.0 ) { upper_color[i][j] = 1.0; }
	    if ( upper_color[i][j] < 0.1 ) { upper_color[i][j] = 0.1; }
	    if ( middle_color[i][j] > 1.0 ) { middle_color[i][j] = 1.0; }
	    if ( middle_color[i][j] < 0.1 ) { middle_color[i][j] = 0.1; }
	    if ( lower_color[i][j] > 1.0 ) { lower_color[i][j] = 1.0; }
	    if ( lower_color[i][j] < 0.1 ) { lower_color[i][j] = 0.1; }
	}
	// upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 
	//                     (GLubyte)(sky_color[3] * 1.0);

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
	    diff = sky_color[j] - fog_color[j];

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        sky_color[j], fog_color[j], diff);

	    upper_color[i][j] = sky_color[j] - diff * 0.3;
	    middle_color[i][j] = sky_color[j] - diff * 0.9 + middle_amt[j];
	    lower_color[i][j] = fog_color[j] + outer_amt[j];

	    if ( upper_color[i][j] > 1.0 ) { upper_color[i][j] = 1.0; }
	    if ( upper_color[i][j] < 0.1 ) { upper_color[i][j] = 0.1; }
	    if ( middle_color[i][j] > 1.0 ) { middle_color[i][j] = 1.0; }
	    if ( middle_color[i][j] < 0.1 ) { middle_color[i][j] = 0.1; }
	    if ( lower_color[i][j] > 1.0 ) { lower_color[i][j] = 1.0; }
	    if ( lower_color[i][j] < 35 ) { lower_color[i][j] = 35; }
	}
	// upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 
	//                     (GLubyte)(sky_color[3] * 1.0);

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

    for ( i = 0; i < 12; i++ ) {
	sgCopyVec3( bottom_color[i], fog_color );
    }

    //
    // Second, assign the basic colors to the object color arrays
    //

    float *slot;
    int counter;

    // update the center disk color arrays
    counter = 0;
    slot = center_disk_cl->get( counter++ );
    // sgVec3 red;
    // sgSetVec3( red, 1.0, 0.0, 0.0 );
    sgCopyVec3( slot, sky_color );
    for ( i = 11; i >= 0; i-- ) {
	slot = center_disk_cl->get( counter++ );
	sgCopyVec3( slot, upper_color[i] );
    }
    slot = center_disk_cl->get( counter++ );
    sgCopyVec3( slot, upper_color[11] );

    // generate the upper ring
    counter = 0;
    for ( i = 0; i < 12; i++ ) {
	slot = upper_ring_cl->get( counter++ );
	sgCopyVec3( slot, middle_color[i] );

	slot = upper_ring_cl->get( counter++ );
	sgCopyVec3( slot, upper_color[i] );
    }
    slot = upper_ring_cl->get( counter++ );
    sgCopyVec3( slot, middle_color[0] );

    slot = upper_ring_cl->get( counter++ );
    sgCopyVec3( slot, upper_color[0] );

    // generate middle ring
    counter = 0;
    for ( i = 0; i < 12; i++ ) {
	slot = middle_ring_cl->get( counter++ );
	sgCopyVec3( slot, lower_color[i] );

	slot = middle_ring_cl->get( counter++ );
	sgCopyVec3( slot, middle_color[i] );
    }
    slot = middle_ring_cl->get( counter++ );
    sgCopyVec3( slot, lower_color[0] );

    slot = middle_ring_cl->get( counter++ );
    sgCopyVec3( slot, middle_color[0] );

    // generate lower ring
    counter = 0;
    for ( i = 0; i < 12; i++ ) {
	slot = lower_ring_cl->get( counter++ );
	sgCopyVec3( slot, bottom_color[i] );

	slot = lower_ring_cl->get( counter++ );
	sgCopyVec3( slot, lower_color[i] );
    }
    slot = lower_ring_cl->get( counter++ );
    sgCopyVec3( slot, bottom_color[0] );

    slot = lower_ring_cl->get( counter++ );
    sgCopyVec3( slot, lower_color[0] );

    return true;
}


// reposition the sky at the specified origin and orientation
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (and orients the
// sunrise/set effects
bool FGSkyDome::reposition( sgVec3 p, double lon, double lat, double spin ) {
    sgMat4 T, LON, LAT, SPIN;
    sgVec3 axis;

    // Translate to view position
    // Point3D zero_elev = current_view.get_cur_zero_elev();
    // xglTranslatef( zero_elev.x(), zero_elev.y(), zero_elev.z() );
    sgMakeTransMat4( T, p );

    // printf("  Translated to %.2f %.2f %.2f\n", 
    //        zero_elev.x, zero_elev.y, zero_elev.z );

    // Rotate to proper orientation
    // printf("  lon = %.2f  lat = %.2f\n", FG_Longitude * RAD_TO_DEG,
    //        FG_Latitude * RAD_TO_DEG);
    // xglRotatef( f->get_Longitude() * RAD_TO_DEG, 0.0, 0.0, 1.0 );
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( LON, lon * RAD_TO_DEG, axis );

    // xglRotatef( 90.0 - f->get_Latitude() * RAD_TO_DEG, 0.0, 1.0, 0.0 );
    sgSetVec3( axis, 0.0, 1.0, 0.0 );
    sgMakeRotMat4( LAT, 90.0 - lat * RAD_TO_DEG, axis );

    // xglRotatef( l->sun_rotation * RAD_TO_DEG, 0.0, 0.0, 1.0 );
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( SPIN, spin * RAD_TO_DEG, axis );

    sgMat4 TRANSFORM;

    sgCopyMat4( TRANSFORM, T );
    sgPreMultMat4( TRANSFORM, LON );
    sgPreMultMat4( TRANSFORM, LAT );
    sgPreMultMat4( TRANSFORM, SPIN );

    sgCoord skypos;
    sgSetCoord( &skypos, TRANSFORM );

    sky_transform->setTransform( &skypos );

    return true;
}


#if 0

// depricated code from here to the end 


// Calculate the sky structure vertices
void fgSkyVerticesInit() {
    float theta;
    int i;

    FG_LOG(FG_ASTRO, FG_INFO, "  Generating the sky dome vertices.");

    for ( i = 0; i < 12; i++ ) {
	theta = (i * 30.0) * DEG_TO_RAD;
	
	inner_vertex[i][0] = cos(theta) * UPPER_RADIUS;
	inner_vertex[i][1] = sin(theta) * UPPER_RADIUS;
	inner_vertex[i][2] = UPPER_ELEV;
	
	// printf("    %.2f %.2f\n", cos(theta) * UPPER_RADIUS, 
	//        sin(theta) * UPPER_RADIUS);

	middle_vertex[i][0] = cos((double)theta) * MIDDLE_RADIUS;
	middle_vertex[i][1] = sin((double)theta) * MIDDLE_RADIUS;
	middle_vertex[i][2] = MIDDLE_ELEV;
	    
	outer_vertex[i][0] = cos((double)theta) * LOWER_RADIUS;
	outer_vertex[i][1] = sin((double)theta) * LOWER_RADIUS;
	outer_vertex[i][2] = LOWER_ELEV;
	    
	bottom_vertex[i][0] = cos((double)theta) * BOTTOM_RADIUS;
	bottom_vertex[i][1] = sin((double)theta) * BOTTOM_RADIUS;
	bottom_vertex[i][2] = BOTTOM_ELEV;
    }
}


// (Re)calculate the sky colors at each vertex
void fgSkyColorsInit() {
    fgLIGHT *l;
    double sun_angle, diff;
    double outer_param[3], outer_amt[3], outer_diff[3];
    double middle_param[3], middle_amt[3], middle_diff[3];
    int i, j;

    l = &cur_light_params;

    FG_LOG( FG_ASTRO, FG_INFO, 
	    "  Generating the sky colors for each vertex." );

    // setup for the possibility of sunset effects
    sun_angle = l->sun_angle * RAD_TO_DEG;
    // fgPrintf( FG_ASTRO, FG_INFO, 
    //           "  Sun angle in degrees = %.2f\n", sun_angle);

    if ( (sun_angle > 80.0) && (sun_angle < 100.0) ) {
	// 0.0 - 0.4
	outer_param[0] = (10.0 - fabs(90.0 - sun_angle)) / 20.0;
	outer_param[1] = (10.0 - fabs(90.0 - sun_angle)) / 40.0;
	outer_param[2] = -(10.0 - fabs(90.0 - sun_angle)) / 30.0;
	// outer_param[2] = 0.0;

	middle_param[0] = (10.0 - fabs(90.0 - sun_angle)) / 40.0;
	middle_param[1] = (10.0 - fabs(90.0 - sun_angle)) / 80.0;
	middle_param[2] = 0.0;

	outer_diff[0] = outer_param[0] / 6.0;
	outer_diff[1] = outer_param[1] / 6.0;
	outer_diff[2] = outer_param[2] / 6.0;

	middle_diff[0] = middle_param[0] / 6.0;
	middle_diff[1] = middle_param[1] / 6.0;
	middle_diff[2] = middle_param[2] / 6.0;
    } else {
	outer_param[0] = outer_param[1] = outer_param[2] = 0.0;
	middle_param[0] = middle_param[1] = middle_param[2] = 0.0;

	outer_diff[0] = outer_diff[1] = outer_diff[2] = 0.0;
	middle_diff[0] = middle_diff[1] = middle_diff[2] = 0.0;
    }
    // printf("  outer_red_param = %.2f  outer_red_diff = %.2f\n", 
    //        outer_red_param, outer_red_diff);

    // calculate transition colors between sky and fog
    for ( j = 0; j < 3; j++ ) {
	outer_amt[j] = outer_param[j];
	middle_amt[j] = middle_param[j];
    }

    for ( i = 0; i < 6; i++ ) {
	for ( j = 0; j < 3; j++ ) {
	    diff = l->sky_color[j] - l->fog_color[j];

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        l->sky_color[j], l->fog_color[j], diff);

	    upper_color[i][j] = (GLubyte)((l->sky_color[j] - diff * 0.3) * 255);
	    middle_color[i][j] = (GLubyte)((l->sky_color[j] - diff * 0.9 
					   + middle_amt[j]) * 255);
	    lower_color[i][j] = (GLubyte)((l->fog_color[j] + outer_amt[j]) 
					  * 255);

	    if ( upper_color[i][j] > 255 ) { upper_color[i][j] = 255; }
	    if ( upper_color[i][j] < 25 ) { upper_color[i][j] = 25; }
	    if ( middle_color[i][j] > 255 ) { middle_color[i][j] = 255; }
	    if ( middle_color[i][j] < 25 ) { middle_color[i][j] = 25; }
	    if ( lower_color[i][j] > 255 ) { lower_color[i][j] = 255; }
	    if ( lower_color[i][j] < 25 ) { lower_color[i][j] = 25; }
	}
	upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 
	    (GLubyte)(l->sky_color[3] * 255);

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

    for ( j = 0; j < 3; j++ ) {
	outer_amt[j] = 0.0;
	middle_amt[j] = 0.0;
    }

    for ( i = 6; i < 12; i++ ) {

	for ( j = 0; j < 3; j++ ) {
	    diff = l->sky_color[j] - l->fog_color[j];

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        l->sky_color[j], l->fog_color[j], diff);

	    upper_color[i][j] = (GLubyte)((l->sky_color[j] - diff * 0.3) * 255);
	    middle_color[i][j] = (GLubyte)((l->sky_color[j] - diff * 0.9 
					    + middle_amt[j]) * 255);
	    lower_color[i][j] = (GLubyte)((l->fog_color[j] + outer_amt[j]) 
					  * 255);

	    if ( upper_color[i][j] > 255 ) { upper_color[i][j] = 255; }
	    if ( upper_color[i][j] < 25 ) { upper_color[i][j] = 25; }
	    if ( middle_color[i][j] > 255 ) { middle_color[i][j] = 255; }
	    if ( middle_color[i][j] < 25 ) { middle_color[i][j] = 25; }
	    if ( lower_color[i][j] > 255 ) { lower_color[i][j] = 255; }
	    if ( lower_color[i][j] < 35 ) { lower_color[i][j] = 35; }
	}
	upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 
	    (GLubyte)(l->sky_color[3] * 255);

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
}


// Initialize the sky structure and colors
void fgSkyInit() {
    FG_LOG( FG_ASTRO, FG_INFO, "Initializing the sky" );

    fgSkyVerticesInit();

    // regester fgSkyColorsInit() as an event to be run periodically
    global_events.Register( "fgSkyColorsInit()", fgSkyColorsInit, 
		            fgEVENT::FG_EVENT_READY, 30000);
}


// Draw the Sky
void fgSkyRender() {
    FGInterface *f;
    fgLIGHT *l;
    GLubyte sky_color[4];
    GLubyte upper_color[4];
    GLubyte middle_color[4];
    GLubyte lower_color[4];
    double diff;
    int i;

    f = current_aircraft.fdm_state;
    l = &cur_light_params;

    // printf("Rendering the sky.\n");

    // calculate the proper colors
    for ( i = 0; i < 3; i++ ) {
	diff = l->sky_color[i] - l->adj_fog_color[i];

	// printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	//        l->sky_color[j], l->adj_fog_color[j], diff);

	upper_color[i] = (GLubyte)((l->sky_color[i] - diff * 0.3) * 255);
	middle_color[i] = (GLubyte)((l->sky_color[i] - diff * 0.9) * 255);
	lower_color[i] = (GLubyte)(l->adj_fog_color[i] * 255);
    }
    upper_color[3] = middle_color[3] = lower_color[3] = 
	(GLubyte)(l->adj_fog_color[3] * 255);

    xglPushMatrix();

    // Translate to view position
    Point3D zero_elev = current_view.get_cur_zero_elev();
    xglTranslatef( zero_elev.x(), zero_elev.y(), zero_elev.z() );
    // printf("  Translated to %.2f %.2f %.2f\n", 
    //        zero_elev.x, zero_elev.y, zero_elev.z );

    // Rotate to proper orientation
    // printf("  lon = %.2f  lat = %.2f\n", FG_Longitude * RAD_TO_DEG,
    //        FG_Latitude * RAD_TO_DEG);
    xglRotatef( f->get_Longitude() * RAD_TO_DEG, 0.0, 0.0, 1.0 );
    xglRotatef( 90.0 - f->get_Latitude() * RAD_TO_DEG, 0.0, 1.0, 0.0 );
    xglRotatef( l->sun_rotation * RAD_TO_DEG, 0.0, 0.0, 1.0 );

    // Draw inner/center section of sky*/
    xglBegin( GL_TRIANGLE_FAN );
    for ( i = 0; i < 4; i++ ) {
	sky_color[i] = (GLubyte)(l->sky_color[i] * 255);
    }
    xglColor4fv(l->sky_color);
    xglVertex3f(0.0, 0.0, CENTER_ELEV);
    for ( i = 11; i >= 0; i-- ) {
	xglColor4ubv( upper_color );
	xglVertex3fv( inner_vertex[i] );
    }
    xglColor4ubv( upper_color );
    xglVertex3fv( inner_vertex[11] );
    xglEnd();

    // Draw the middle ring
    xglBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < 12; i++ ) {
	xglColor4ubv( middle_color );
	// printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	//        middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	//        middle_color[i][3]);
	// xglColor4f(1.0, 0.0, 0.0, 1.0);
	xglVertex3fv( middle_vertex[i] );
	xglColor4ubv( upper_color );
	// printf("upper_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	//        upper_color[i][0], upper_color[i][1], upper_color[i][2], 
	//        upper_color[i][3]);
	// xglColor4f(0.0, 0.0, 1.0, 1.0);
	xglVertex3fv( inner_vertex[i] );
    }
    xglColor4ubv( middle_color );
    // xglColor4f(1.0, 0.0, 0.0, 1.0);
    xglVertex3fv( middle_vertex[0] );
    xglColor4ubv( upper_color );
    // xglColor4f(0.0, 0.0, 1.0, 1.0);
    xglVertex3fv( inner_vertex[0] );
    xglEnd();

    // Draw the outer ring
    xglBegin( GL_TRIANGLE_STRIP );
    for ( i = 0; i < 12; i++ ) {
	xglColor4ubv( lower_color );
	xglVertex3fv( outer_vertex[i] );
	xglColor4ubv( middle_color );
	xglVertex3fv( middle_vertex[i] );
    }
    xglColor4ubv( lower_color );
    xglVertex3fv( outer_vertex[0] );
    xglColor4ubv( middle_color );
    xglVertex3fv( middle_vertex[0] );
    xglEnd();

    // Draw the bottom skirt
    xglBegin( GL_TRIANGLE_STRIP );
    xglColor4ubv( lower_color );
    for ( i = 0; i < 12; i++ ) {
	xglVertex3fv( bottom_vertex[i] );
	xglVertex3fv( outer_vertex[i] );
    }
    xglVertex3fv( bottom_vertex[0] );
    xglVertex3fv( outer_vertex[0] );
    xglEnd();

    xglPopMatrix();
}


#endif
