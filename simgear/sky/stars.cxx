// stars.cxx -- model the stars
//
// Written by Durk Talsma. Originally started October 1997, for distribution  
// with the FlightGear project. Version 2 was written in August and 
// September 1998. This code is based upon algorithms and data kindly 
// provided by Mr. Paul Schlyter. (pausch@saaf.se). 
//
// Separated out rendering pieces and converted to ssg by Curt Olson,
// March 2000
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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <iostream>

#include <plib/sg.h>
#include <plib/ssg.h>

#include <compiler.h>

#include "stars.hxx"

#ifdef _MSC_VER
FG_USING_STD(cout);
FG_USING_STD(endl);
#endif 


// Set up star rendering call backs
static int sgStarPreDraw( ssgEntity *e ) {
    /* cout << endl << "Star pre draw" << endl << "----------------" 
	 << endl << endl; */

    ssgLeaf *f = (ssgLeaf *)e;
    if ( f -> hasState () ) f->getState()->apply() ;

    glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_FOG_BIT );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_FOG );
    // glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

    return true;
}

static int sgStarPostDraw( ssgEntity *e ) {
    /* cout << endl << "Star post draw" << endl << "----------------" 
	 << endl << endl; */

    glPopAttrib();

    // glEnable( GL_DEPTH_TEST );
    // glEnable( GL_FOG );

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
    state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
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
    // double min = 100;
    // double max = -100;
    double mag, nmag, alpha, factor, cutoff;
    float *color;

    // determine which star structure to draw
    if ( sun_angle > (0.5 * SGD_PI + 10.0 * SGD_DEGREES_TO_RADIANS ) ) {
	// deep night
	factor = 1.0;
	cutoff = 4.5;
    } else if ( sun_angle > (0.5 * SGD_PI + 8.8 * SGD_DEGREES_TO_RADIANS ) ) {
	factor = 1.0;
	cutoff = 3.8;
    } else if ( sun_angle > (0.5 * SGD_PI + 7.5 * SGD_DEGREES_TO_RADIANS ) ) {
	factor = 0.95;
	cutoff = 3.1;
    } else if ( sun_angle > (0.5 * SGD_PI + 7.0 * SGD_DEGREES_TO_RADIANS ) ) {
	factor = 0.9;
	cutoff = 2.4;
    } else if ( sun_angle > (0.5 * SGD_PI + 6.5 * SGD_DEGREES_TO_RADIANS ) ) {
	factor = 0.85;
	cutoff = 1.8;
    } else if ( sun_angle > (0.5 * SGD_PI + 6.0 * SGD_DEGREES_TO_RADIANS ) ) {
	factor = 0.8;
	cutoff = 1.2;
    } else if ( sun_angle > (0.5 * SGD_PI + 5.5 * SGD_DEGREES_TO_RADIANS ) ) {
	factor = 0.75;
	cutoff = 0.6;
    } else {
	// early dusk or late dawn
	factor = 0.7;
	cutoff = 0.0;
    }

    for ( int i = 0; i < num; ++i ) {
	// if ( star_data[i][2] < min ) { min = star_data[i][2]; }
	// if ( star_data[i][2] > max ) { max = star_data[i][2]; }

	// magnitude ranges from -1 (bright) to 4 (dim).  The range of
	// star and planet magnitudes can actually go outside of this,
	// but for our purpose, if it is brighter that -1, we'll color
	// it full white/alpha anyway and 4 is a convenient cutoff
	// point which keeps the number of stars drawn at about 500.

	// color (magnitude)
	mag = star_data[i][2];
	if ( mag < cutoff ) {
	    nmag = ( 4.5 - mag ) / 5.5; // translate to 0 ... 1.0 scale
	    // alpha = nmag * 0.7 + 0.3; // translate to a 0.3 ... 1.0 scale
	    alpha = nmag * 0.85 + 0.15; // translate to a 0.15 ... 1.0 scale
	    alpha *= factor;          // dim when the sun is brighter
	} else {
	    alpha = 0.0;
	}

	if (alpha > 1.0) { alpha = 1.0; }
	if (alpha < 0.0) { alpha = 0.0; }

	color = cl->get( i );
	sgSetVec4( color, 1.0, 1.0, 1.0, alpha );
	// cout << "alpha[" << i << "] = " << alpha << endl;
    }

    // cout << "min = " << min << " max = " << max << " count = " << num 
    //      << endl;

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
