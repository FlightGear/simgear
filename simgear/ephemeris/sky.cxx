// sky.cxx -- model sky with an upside down "bowl"
//
// Written by Curtis Olson, started December 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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

#include "sky.hxx"


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


static float inner_vertex[12][3];
static float middle_vertex[12][3];
static float outer_vertex[12][3];
static float bottom_vertex[12][3];

static GLubyte upper_color[12][4];
static GLubyte middle_color[12][4];
static GLubyte lower_color[12][4];


// Constructor
FGSky::FGSky( void ) {
}


// Destructor
FGSky::~FGSky( void ) {
}


// initialize the sky object and connect it into the scene graph
bool FGSky::initialize() {
    sgVec3 vertex;
    sgVec3 color;

    float theta;
    int i;

    // initialize arrays
    upper_ring_vl = new ssgVertexArray( 12 );
    upper_ring_cl = new ssgColourArray( 12 );

    middle_ring_vl = new ssgVertexArray( 12 );
    middle_ring_cl = new ssgColourArray( 12 );

    lower_ring_vl = new ssgVertexArray( 12 );
    lower_ring_cl = new ssgColourArray( 12 );

    bottom_ring_vl = new ssgVertexArray( 12 );
    bottom_ring_cl = new ssgColourArray( 12 );

    // generate the sky dome vertices

    sgSetVec3( color, 0.0, 0.0, 1.0 ); // seed to all blue

    for ( i = 0; i < 12; i++ ) {
	theta = (i * 30.0) * DEG_TO_RAD;

	sgSetVec3( vertex,
		   cos(theta) * UPPER_RADIUS,
		   sin(theta) * UPPER_RADIUS,
		   UPPER_ELEV );
	upper_ring_vl->add( vertex );
	upper_ring_cl->add( color );
	
	sgSetVec3( vertex,
		   cos((double)theta) * MIDDLE_RADIUS,
		   sin((double)theta) * MIDDLE_RADIUS,
		   MIDDLE_ELEV );
	middle_ring_vl->add( vertex );
	middle_ring_cl->add( color );

	sgSetVec3( vertex,
		   cos((double)theta) * LOWER_RADIUS,
		   sin((double)theta) * LOWER_RADIUS,
		   LOWER_ELEV );
	lower_ring_vl->add( vertex );
	lower_ring_cl->add( color );

	sgSetVec3( vertex,
		   cos((double)theta) * BOTTOM_RADIUS,
		   sin((double)theta) * BOTTOM_RADIUS,
		   BOTTOM_ELEV );
	bottom_ring_vl->add( vertex );
	bottom_ring_cl->add( color );
    }

    // force a rebuild of the colors
    rebuild();

    return true;
}


// rebuild the sky colors based on current value of sun_angle, sky,
// and fog colors.  This updates the color arrays for ssgVtxTable.
bool FGSky::rebuild() {
    double diff;
    sgVec3 outer_param, outer_amt, outer_diff;
    sgVec3 middle_param, middle_amt, middle_diff;
    int i, j;

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

    // float *upper_color, *middle_color, *lower_color;

    for ( i = 0; i < 6; i++ ) {
	// inner_color = 
	for ( j = 0; j < 3; j++ ) {
	    diff = sky_color[j] - fog_color[j];

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        l->sky_color[j], l->fog_color[j], diff);

	    upper_color[i][j] = (GLubyte)((sky_color[j] - diff * 0.3) * 255);
	    middle_color[i][j] = (GLubyte)((sky_color[j] - diff * 0.9 
					   + middle_amt[j]) * 255);
	    lower_color[i][j] = (GLubyte)((fog_color[j] + outer_amt[j]) 
					  * 255);

	    if ( upper_color[i][j] > 255 ) { upper_color[i][j] = 255; }
	    if ( upper_color[i][j] < 25 ) { upper_color[i][j] = 25; }
	    if ( middle_color[i][j] > 255 ) { middle_color[i][j] = 255; }
	    if ( middle_color[i][j] < 25 ) { middle_color[i][j] = 25; }
	    if ( lower_color[i][j] > 255 ) { lower_color[i][j] = 255; }
	    if ( lower_color[i][j] < 25 ) { lower_color[i][j] = 25; }
	}
	upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 
	    (GLubyte)(sky_color[3] * 255);

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

	    upper_color[i][j] = (GLubyte)((sky_color[j] - diff * 0.3) * 255);
	    middle_color[i][j] = (GLubyte)((sky_color[j] - diff * 0.9 
					    + middle_amt[j]) * 255);
	    lower_color[i][j] = (GLubyte)((fog_color[j] + outer_amt[j]) 
					  * 255);

	    if ( upper_color[i][j] > 255 ) { upper_color[i][j] = 255; }
	    if ( upper_color[i][j] < 25 ) { upper_color[i][j] = 25; }
	    if ( middle_color[i][j] > 255 ) { middle_color[i][j] = 255; }
	    if ( middle_color[i][j] < 25 ) { middle_color[i][j] = 25; }
	    if ( lower_color[i][j] > 255 ) { lower_color[i][j] = 255; }
	    if ( lower_color[i][j] < 35 ) { lower_color[i][j] = 35; }
	}
	upper_color[i][3] = middle_color[i][3] = lower_color[i][3] = 
	    (GLubyte)(sky_color[3] * 255);

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

    return true;
}


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


