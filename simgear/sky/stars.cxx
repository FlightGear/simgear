// stars.cxx -- model the stars
//
// Written by Durk Talsma. Originally started October 1997, for distribution  
// with the FlightGear project. Version 2 was written in August and 
// September 1998. This code is based upon algorithms and data kindly 
// provided by Mr. Paul Schlyter. (pausch@saaf.se). 
//
// Separated out rendering pieces and converted to ssg by Curt Olson,
// March 2000
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


#include <stdio.h>
#include <iostream>

#include <plib/ssg.h>

#include <simgear/constants.h>

#include "stars.hxx"


// Set up star rendering call backs
static int sgStarPreDraw( ssgEntity *e ) {
    /* cout << endl << "Star pre draw" << endl << "----------------" 
	 << endl << endl; */
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_FOG );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

    return true;
}

static int sgStarPostDraw( ssgEntity *e ) {
    /* cout << endl << "Star post draw" << endl << "----------------" 
	 << endl << endl; */
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_FOG );

    return true;
}


// Constructor
SGStars::SGStars( void ) {
}


// Destructor
SGStars::~SGStars( void ) {
}


// initialize the stars object and connect it into our scene graph root
ssgBranch * SGStars::build( int num, sgdVec3 *star_data, double star_dist ) {
    sgVec4 color;

    if ( star_data == NULL ) {
	cout << "WARNING: null star data passed to SGStars::build()" << endl;
    }

    // set up the orb state
    state = new ssgSimpleState();
    state->disable( GL_LIGHTING );
    state->disable( GL_CULL_FACE );
    state->disable( GL_TEXTURE_2D );
    state->enable( GL_COLOR_MATERIAL );
    state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    state->enable( GL_BLEND );
    state->disable( GL_ALPHA_TEST );

    vl = new ssgVertexArray( num );
    cl = new ssgColourArray( num );
    // cl = new ssgColourArray( 1 );
    // sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
    // cl->add( color );
   
    // Build ssg structure
    sgVec3 p;
    for ( int i = 0; i < num; ++i ) {
	// position seeded to arbitrary values
	sgSetVec3( p, 
		   star_dist * cos( star_data[i][0] )
		       * cos( star_data[i][1] ),
                   star_dist * sin( star_data[i][0] )
		       * cos( star_data[i][1] ),
                   star_dist * sin( star_data[i][1] )
		   );
	vl->add( p );

	// color (magnitude)
	sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
	cl->add( color );
    }

    ssgLeaf *stars_obj = 
	new ssgVtxTable ( GL_POINTS, vl, NULL, NULL, cl );
    stars_obj->setState( state );
    stars_obj->setCallback( SSG_CALLBACK_PREDRAW, sgStarPreDraw );
    stars_obj->setCallback( SSG_CALLBACK_POSTDRAW, sgStarPostDraw );

    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    stars_transform = new ssgTransform;

    stars_transform->addKid( stars_obj );

    cout << "stars = " << stars_transform << endl;

    return stars_transform;
}


// repaint the sun colors based on current value of sun_angle in
// degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool SGStars::repaint( double sun_angle, int num, sgdVec3 *star_data ) {
    // TEST return true;

    float *color;
    for ( int i = 0; i < num; ++i ) {
	// color (magnitude)
	double magnitude = (0.0 - star_data[i][2]) / 5.0 + 1.0;
	magnitude = magnitude * 0.7 + (3 * 0.1);
	if (magnitude > 1.0) magnitude = 1.0;
	if (magnitude < 0.0) magnitude = 0.0;

	color = cl->get( i );
	sgSetVec4( color, 1.0, 1.0, 1.0, magnitude );
	// sgSetVec4( color, 1, 1, 1, 1.0 );
	// cout << "color[" << i << "] = " << magnitude << endl;
    }

    return true;
}


// reposition the stars for the specified time (GST rotation),
// offset by our current position (p) so that it appears fixed at a
// great distance from the viewer.
bool SGStars::reposition( sgVec3 p, double angle )
{
    sgMat4 T1, GST;
    sgVec3 axis;

    sgMakeTransMat4( T1, p );

    sgSetVec3( axis, 0.0, 0.0, -1.0 );
    sgMakeRotMat4( GST, angle, axis );

    sgMat4 TRANSFORM;
    sgCopyMat4( TRANSFORM, T1 );
    sgPreMultMat4( TRANSFORM, GST );

    sgCoord skypos;
    sgSetCoord( &skypos, TRANSFORM );

    stars_transform->setTransform( &skypos );

    return true;
}
