// oursun.hxx -- model earth's sun
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

#include "sphere.hxx"
#include "oursun.hxx"


// Set up sun rendering call backs
static int sgSunOrbPreDraw( ssgEntity *e ) {
    /* cout << endl << "Sun orb pre draw" << endl << "----------------" 
	 << endl << endl; */

    ssgLeaf *f = (ssgLeaf *)e;
    if ( f -> hasState () ) f->getState()->apply() ;

    glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_FOG_BIT );
    // cout << "push error = " << glGetError() << endl;

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_FOG );

    return true;
}

static int sgSunOrbPostDraw( ssgEntity *e ) {
    /* cout << endl << "Sun orb post draw" << endl << "----------------" 
	 << endl << endl; */

    glPopAttrib();
    // cout << "pop error = " << glGetError() << endl;

    // glEnable( GL_DEPTH_TEST );
    // glEnable( GL_FOG );

    return true;
}

static int sgSunHaloPreDraw( ssgEntity *e ) {
    /* cout << endl << "Sun halo pre draw" << endl << "----------------" 
	 << endl << endl; */

    ssgLeaf *f = (ssgLeaf *)e;
    if ( f -> hasState () ) f->getState()->apply() ;

    glPushAttrib( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_FOG_BIT );
    // cout << "push error = " << glGetError() << endl;

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_FOG );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

    return true;
}

static int sgSunHaloPostDraw( ssgEntity *e ) {
    /* cout << endl << "Sun halo post draw" << endl << "----------------" 
	 << endl << endl; */

    glPopAttrib();
    // cout << "pop error = " << glGetError() << endl;

    // glEnable( GL_DEPTH_TEST );
    // glEnable( GL_FOG );

    return true;
}


// Constructor
SGSun::SGSun( void ) {
}


// Destructor
SGSun::~SGSun( void ) {
}


#if 0
static GLuint makeHalo( GLubyte *sun_texbuf, int width ) {
    int texSize;
    GLuint texid;
    GLubyte *p;
    int i,j;
    double radius;
  
    // create a texture id
#ifdef GL_VERSION_1_1
    glGenTextures(1, &texid);
    glBindTexture(GL_TEXTURE_2D, texid);
#elif GL_EXT_texture_object
    glGenTexturesEXT(1, &texid);
    glBindTextureEXT(GL_TEXTURE_2D, texid);
#else
#   error port me
#endif

    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE ) ;
 
    // create the actual texture contents
    texSize = width * width;
  
    if ( !sun_texbuf ) {
	cout << "ouch ..." << endl;
	exit(-1);  // Ugly!
    }

    p = sun_texbuf;
  
    radius = (double)(width / 2);
  
    GLubyte value;
    double x, y, d;
    for ( i = 0; i < width; i++ ) {
	for ( j = 0; j < width; j++ ) {
	    x = fabs((double)(i - (width / 2)));
	    y = fabs((double)(j - (width / 2)));
	    d = sqrt((x * x) + (y * y));
	    if (d < radius) {
		// t is 1.0 at center, 0.0 at edge
		double t = 1.0 - (d / radius);

		// inverse square looks nice 
		value = (int)((double) 0xff * (t*t));
	    } else {
		value = 0x00;
	    }
	    *p = value;
	    *(p+1) = value;
	    *(p+2) = value;
	    // *(p+3) = value;

	    p += 3;
	}
    }

    /* glTexImage2D( GL_TEXTURE_2D,
		  0,
		  GL_RGBA,
		  width, width,
		  0,
		  GL_RGBA, GL_UNSIGNED_BYTE,
		  sun_texbuf ); */

    return texid;
}


#define RGB  3			// 3 bytes of color info per pixel
#define RGBA 4			// 4 bytes of color+alpha info
void my_glWritePPMFile(const char *filename, GLubyte *buffer, int win_width, int win_height, int mode)
{
    int i, j, k, q;
    unsigned char *ibuffer;
    FILE *fp;
    int pixelSize = mode==GL_RGBA?4:3;

    ibuffer = (unsigned char *) malloc(win_width*win_height*RGB);

    fp = fopen(filename, "wb");
    fprintf(fp, "P6\n# CREATOR: glReadPixel()\n%d %d\n%d\n",
	    win_width, win_height, UCHAR_MAX);
    q = 0;
    for (i = 0; i < win_height; i++) {
	for (j = 0; j < win_width; j++) {
	    for (k = 0; k < RGB; k++) {
		ibuffer[q++] = (unsigned char)
		    *(buffer + (pixelSize*((win_height-1-i)*win_width+j)+k));
	    }
	}
    }

    // *(buffer + (pixelSize*((win_height-1-i)*win_width+j)+k));

    fwrite(ibuffer, sizeof(unsigned char), RGB*win_width*win_height, fp);
    fclose(fp);
    free(ibuffer);

    printf("wrote file (%d x %d pixels, %d bytes)\n",
	   win_width, win_height, RGB*win_width*win_height);
}
#endif


// initialize the sun object and connect it into our scene graph root
ssgBranch * SGSun::build( FGPath path, double sun_size ) {

    // set up the orb state
    orb_state = new ssgSimpleState();
    orb_state->setShadeModel( GL_SMOOTH );
    orb_state->disable( GL_LIGHTING );
    // orb_state->enable( GL_LIGHTING );
    orb_state->disable( GL_CULL_FACE );
    orb_state->disable( GL_TEXTURE_2D );
    orb_state->enable( GL_COLOR_MATERIAL );
    orb_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    orb_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    orb_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    orb_state->disable( GL_BLEND );
    orb_state->disable( GL_ALPHA_TEST );

    cl = new ssgColourArray( 1 );
    sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );
    cl->add( color );

    ssgBranch *orb = ssgMakeSphere( orb_state, cl, sun_size, 10, 10, 
				    sgSunOrbPreDraw, sgSunOrbPostDraw );

    // force a repaint of the sun colors with arbitrary defaults
    repaint( 0.0 );

    // build the halo
    // sun_texbuf = new GLubyte[64*64*3];
    // sun_texid = makeHalo( sun_texbuf, 64 );
    // my_glWritePPMFile("sunhalo.ppm", sun_texbuf, 64, 64, RGB);

    // set up the halo state
    path.append( "halo.rgba" );
    halo_state = new ssgSimpleState();
    halo_state->setTexture( (char *)path.c_str() );
    halo_state->enable( GL_TEXTURE_2D );
    halo_state->disable( GL_LIGHTING );
    // halo_state->enable( GL_LIGHTING );
    halo_state->setShadeModel( GL_SMOOTH );
    halo_state->disable( GL_CULL_FACE );
    halo_state->enable( GL_COLOR_MATERIAL );
    halo_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    halo_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    halo_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    halo_state->enable( GL_ALPHA_TEST );
    halo_state->setAlphaClamp(0.01);
    halo_state->enable ( GL_BLEND ) ;

    // Build ssg structure
    double size = sun_size * 10.0;
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

    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    sun_transform = new ssgTransform;

    halo->setCallback( SSG_CALLBACK_PREDRAW, sgSunHaloPreDraw );
    halo->setCallback( SSG_CALLBACK_POSTDRAW, sgSunHaloPostDraw );
    sun_transform->addKid( halo );
    sun_transform->addKid( orb );

    return sun_transform;
}


// repaint the sun colors based on current value of sun_angle in
// degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool SGSun::repaint( double sun_angle ) {
    if ( sun_angle * SGD_RADIANS_TO_DEGREES < 100 ) {
	// else sun is well below horizon (so no point in repainting it)
    
	// x_10 = sun_angle^10
	double x_10 = sun_angle * sun_angle * sun_angle * sun_angle * sun_angle
	    * sun_angle * sun_angle * sun_angle * sun_angle * sun_angle;

	float ambient = (float)(0.4 * pow (1.1, - x_10 / 30.0));
	if (ambient < 0.3) { ambient = 0.3; }
	if (ambient > 1.0) { ambient = 1.0; }

	sgVec4 color;
	sgSetVec4( color,
		   (ambient * 6.0)  - 1.0, // minimum value = 0.8
		   (ambient * 11.0) - 3.0, // minimum value = 0.3
		   (ambient * 12.0) - 3.6, // minimum value = 0.0
		   1.0 );
    
	// temp test, forces the color to always be white
	// sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );

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


// reposition the sun at the specified right ascension and
// declination, offset by our current position (p) so that it appears
// fixed at a great distance from the viewer.  Also add in an optional
// rotation (i.e. for the current time of day.)
bool SGSun::reposition( sgVec3 p, double angle,
			double rightAscension, double declination, 
			double sun_dist )
{
    sgMat4 T1, T2, GST, RA, DEC;
    sgVec3 axis;
    sgVec3 v;

    sgMakeTransMat4( T1, p );

    sgSetVec3( axis, 0.0, 0.0, -1.0 );
    sgMakeRotMat4( GST, angle, axis );

    // xglRotatef( ((SGD_RADIANS_TO_DEGREES * rightAscension)- 90.0),
    //             0.0, 0.0, 1.0);
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( RA, (rightAscension * SGD_RADIANS_TO_DEGREES) - 90.0, axis );

    // xglRotatef((SGD_RADIANS_TO_DEGREES * declination), 1.0, 0.0, 0.0);
    sgSetVec3( axis, 1.0, 0.0, 0.0 );
    sgMakeRotMat4( DEC, declination * SGD_RADIANS_TO_DEGREES, axis );

    // xglTranslatef(0,sun_dist);
    sgSetVec3( v, 0.0, sun_dist, 0.0 );
    sgMakeTransMat4( T2, v );

    sgMat4 TRANSFORM;
    sgCopyMat4( TRANSFORM, T1 );
    sgPreMultMat4( TRANSFORM, GST );
    sgPreMultMat4( TRANSFORM, RA );
    sgPreMultMat4( TRANSFORM, DEC );
    sgPreMultMat4( TRANSFORM, T2 );

    sgCoord skypos;
    sgSetCoord( &skypos, TRANSFORM );

    sun_transform->setTransform( &skypos );

    return true;
}
