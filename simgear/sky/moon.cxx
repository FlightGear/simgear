// moon.hxx -- model earth's moon
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

#include "sphere.hxx"
#include "moon.hxx"


// Set up moon rendering call backs
static int sgMoonOrbPreDraw( ssgEntity *e ) {
    /* cout << endl << "Moon orb pre draw" << endl << "----------------" 
	 << endl << endl; */
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_FOG );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE ) ;
    // sgVec4 color;
    // sgSetVec4( color, 0.0, 0.0, 0.0, 1.0 );
    // glMaterialfv ( GL_FRONT_AND_BACK, GL_AMBIENT, color ) ;

    return true;
}

static int sgMoonOrbPostDraw( ssgEntity *e ) {
    /* cout << endl << "Moon orb post draw" << endl << "----------------" 
	 << endl << endl; */
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_FOG );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

    return true;
}

static int sgMoonHaloPreDraw( ssgEntity *e ) {
    /* cout << endl << "Moon halo pre draw" << endl << "----------------" 
	 << endl << endl; */
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_FOG );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

    return true;
}

static int sgMoonHaloPostDraw( ssgEntity *e ) {
    /* cout << endl << "Moon halo post draw" << endl << "----------------" 
	 << endl << endl; */
    glEnable( GL_DEPTH_TEST );
    glEnable( GL_FOG );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

    return true;
}


// Constructor
SGMoon::SGMoon( void ) {
}


// Destructor
SGMoon::~SGMoon( void ) {
}


// build the moon object
ssgBranch * SGMoon::build( FGPath path, double moon_size ) {

    // set up the orb state
    path.append( "moon.rgba" );
    orb_state = new ssgSimpleState();
    orb_state->setTexture( (char *)path.c_str() );
    orb_state->setShadeModel( GL_SMOOTH );
    orb_state->enable( GL_LIGHTING );
    orb_state->enable( GL_CULL_FACE );
    orb_state->enable( GL_TEXTURE_2D );
    orb_state->enable( GL_COLOR_MATERIAL );
    orb_state->setColourMaterial( GL_DIFFUSE );
    orb_state->setMaterial( GL_AMBIENT, 0.0, 0.0, 0.0, 1.0 );
    orb_state->setMaterial( GL_SPECULAR, 0.0, 0.0, 0.0, 1.0 );
    orb_state->enable( GL_BLEND );
    orb_state->enable( GL_ALPHA_TEST );
    orb_state->setAlphaClamp( 0.01 );

    cl = new ssgColourArray( 1 );
    sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
    cl->add( color );

    ssgBranch *orb = ssgMakeSphere( orb_state, cl, moon_size, 15, 15,
				    sgMoonOrbPreDraw, sgMoonOrbPostDraw );

    // force a repaint of the moon colors with arbitrary defaults
    repaint( 0.0 );

    // build the halo
    // moon_texbuf = new GLubyte[64*64*3];
    // moon_texid = makeHalo( moon_texbuf, 64 );
    // my_glWritePPMFile("moonhalo.ppm", moon_texbuf, 64, 64, RGB);

#if 0
    // set up the halo state
    halo_state = new ssgSimpleState();
    halo_state->setTexture( "halo.rgb" );
    // halo_state->setTexture( moon_texid );
    halo_state->enable( GL_TEXTURE_2D );
    halo_state->disable( GL_LIGHTING );
    halo_state->setShadeModel( GL_SMOOTH );
    halo_state->disable( GL_CULL_FACE );

    halo_state->disable( GL_COLOR_MATERIAL );
    halo_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    halo_state->setMaterial ( GL_AMBIENT_AND_DIFFUSE, 1, 1, 1, 1 ) ;
    halo_state -> setMaterial ( GL_EMISSION, 0, 0, 0, 1 ) ;
    halo_state -> setMaterial ( GL_SPECULAR, 0, 0, 0, 1 ) ;
    // halo_state -> setShininess ( 0 ) ;
    halo_state->enable( GL_ALPHA_TEST );
    halo_state->setAlphaClamp(0.01);
    halo_state->enable ( GL_BLEND ) ;


    // Build ssg structure
    double size = moon_size * 10.0;
    sgVec3 v3;
    halo_vl = new ssgVertexArray;
    sgSetVec3( v3, -size, 0.0, -size );
    halo_vl->add( v3 );
    sgSetVec3( v3, size, 0.0, -size );
    halo_vl->add( v3 );
    sgSetVec3( v3, -size, 0.0,  size );
    halo_vl->add( v3 );
    sgSetVec3( v3, size, 0.0,  size );
    halo_vl->add( v3 );

    sgVec2 v2;
    halo_tl = new ssgTexCoordArray;
    sgSetVec2( v2, 0.0f, 0.0f );
    halo_tl->add( v2 );
    sgSetVec2( v2, 1.0, 0.0 );
    halo_tl->add( v2 );
    sgSetVec2( v2, 0.0, 1.0 );
    halo_tl->add( v2 );
    sgSetVec2( v2, 1.0, 1.0 );
    halo_tl->add( v2 );

    ssgLeaf *halo = 
	new ssgVtxTable ( GL_TRIANGLE_STRIP, halo_vl, NULL, halo_tl, cl );
    halo->setState( halo_state );
#endif

    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    moon_transform = new ssgTransform;

    // moon_transform->addKid( halo );
    moon_transform->addKid( orb );

    return moon_transform;
}


// repaint the moon colors based on current value of moon_angle in
// degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = moon rise/set
// 180 degrees = darkest midnight
bool SGMoon::repaint( double moon_angle ) {
    if ( moon_angle * RAD_TO_DEG < 100 ) {
	// else moon is well below horizon (so no point in repainting it)
    
	// x_10 = moon_angle^10
	double x_10 = moon_angle * moon_angle * moon_angle * moon_angle
	    * moon_angle * moon_angle * moon_angle * moon_angle * moon_angle
	    * moon_angle;

	float ambient = (float)(0.4 * pow (1.1, - x_10 / 30.0));
	if (ambient < 0.3) { ambient = 0.3; }
	if (ambient > 1.0) { ambient = 1.0; }

	sgVec4 color;
	sgSetVec4( color,
		   (ambient * 6.0)  - 1.0, // minimum value = 0.8
		   (ambient * 11.0) - 3.0, // minimum value = 0.3
		   (ambient * 12.0) - 3.6, // minimum value = 0.0
		   0.5 );
    
	if (color[0] > 1.0) color[0] = 1.0;
	if (color[1] > 1.0) color[1] = 1.0;
	if (color[2] > 1.0) color[2] = 1.0;

	// cout << "color = " << color[0] << " " << color[1] << " " 
	//      << color[2] << endl;

	float *ptr;
	ptr = cl->get( 0 );
	sgCopyVec4( ptr, color );
    }

    return true;
}


// reposition the moon at the specified right ascension and
// declination, offset by our current position (p) so that it appears
// fixed at a great distance from the viewer.  Also add in an optional
// rotation (i.e. for the current time of day.)
bool SGMoon::reposition( sgVec3 p, double angle,
			 double rightAscension, double declination,
			 double moon_dist )
{
    sgMat4 T1, T2, GST, RA, DEC;
    sgVec3 axis;
    sgVec3 v;

    sgMakeTransMat4( T1, p );

    sgSetVec3( axis, 0.0, 0.0, -1.0 );
    sgMakeRotMat4( GST, angle, axis );

    // xglRotatef(((RAD_TO_DEG * rightAscension)- 90.0), 0.0, 0.0, 1.0);
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( RA, (rightAscension * RAD_TO_DEG) - 90.0, axis );

    // xglRotatef((RAD_TO_DEG * declination), 1.0, 0.0, 0.0);
    sgSetVec3( axis, 1.0, 0.0, 0.0 );
    sgMakeRotMat4( DEC, declination * RAD_TO_DEG, axis );

    // xglTranslatef(0,60000,0);
    sgSetVec3( v, 0.0, 60000.0, 0.0 );
    sgMakeTransMat4( T2, v );

    sgMat4 TRANSFORM;
    sgCopyMat4( TRANSFORM, T1 );
    sgPreMultMat4( TRANSFORM, GST );
    sgPreMultMat4( TRANSFORM, RA );
    sgPreMultMat4( TRANSFORM, DEC );
    sgPreMultMat4( TRANSFORM, T2 );

    sgCoord skypos;
    sgSetCoord( &skypos, TRANSFORM );

    moon_transform->setTransform( &skypos );

    return true;
}
