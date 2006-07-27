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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <plib/sg.h>
#include <plib/ssg.h>

// define the following to enable a cheesy lens flare effect for the sun
// #define FG_TEST_CHEESY_LENS_FLARE

#ifdef FG_TEST_CHEESY_LENS_FLARE
#  include <plib/ssgaLensFlare.h>
#endif

#include <simgear/screen/colors.hxx>
#include "oursun.hxx"


static double sun_exp2_punch_through;

// Set up sun rendering call backs
static int sgSunPreDraw( ssgEntity *e ) {
    /* cout << endl << "Sun orb pre draw" << endl << "----------------" 
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

static int sgSunPostDraw( ssgEntity *e ) {
    /* cout << endl << "Sun orb post draw" << endl << "----------------" 
	 << endl << endl; */

    glPopAttrib();
    // cout << "pop error = " << glGetError() << endl;

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
    // glDisable( GL_FOG );
    glFogf (GL_FOG_DENSITY, sun_exp2_punch_through);
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;

    return true;
}

static int sgSunHaloPostDraw( ssgEntity *e ) {
    /* cout << endl << "Sun halo post draw" << endl << "----------------" 
	 << endl << endl; */

    glPopAttrib();
    // cout << "pop error = " << glGetError() << endl;

    return true;
}


// Constructor
SGSun::SGSun( void ) {
    prev_sun_angle = -9999.0;
    visibility = -9999.0;
}


// Destructor
SGSun::~SGSun( void ) {
}


#if 0
// this might be nice to keep, just as an example of how to generate a
// texture on the fly ...
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
        SG_LOG( SG_EVENT, SG_ALERT,
                               "Could not allocate memroy for the sun texture");
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
ssgBranch * SGSun::build( SGPath path, double sun_size, SGPropertyNode *property_tree_Node ) {

    env_node = property_tree_Node;

    SGPath ihalopath = path, ohalopath = path;

    sgVec4 color;
    sgSetVec4( color, 1.0, 1.0, 1.0, 1.0 );

    sun_cl = new ssgColourArray( 1 );
    sun_cl->add( color );

    ihalo_cl = new ssgColourArray( 1 );
    ihalo_cl->add( color );

    ohalo_cl = new ssgColourArray( 1 );
    ohalo_cl->add( color );

    // force a repaint of the sun colors with arbitrary defaults
    repaint( 0.0, 1.0 );


    // set up the sun-state
    path.append( "sun.rgba" );
    sun_state = new ssgSimpleState();
    sun_state->setShadeModel( GL_SMOOTH );
    sun_state->disable( GL_LIGHTING );
    sun_state->disable( GL_CULL_FACE );
    sun_state->setTexture( (char *)path.c_str() );
    sun_state->enable( GL_TEXTURE_2D );
    sun_state->enable( GL_COLOR_MATERIAL );
    sun_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    sun_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    sun_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    sun_state->enable( GL_BLEND );
    sun_state->setAlphaClamp( 0.01 );
    sun_state->enable( GL_ALPHA_TEST );

    // Build ssg structure
    
   sgVec3 va;
   sun_vl = new ssgVertexArray;
   sgSetVec3( va, -sun_size, 0.0, -sun_size );
   sun_vl->add( va );
   sgSetVec3( va, sun_size, 0.0, -sun_size );
   sun_vl->add( va );
   sgSetVec3( va, -sun_size, 0.0,  sun_size );
   sun_vl->add( va );
   sgSetVec3( va, sun_size, 0.0,  sun_size );
   sun_vl->add( va );

   sgVec2 vb;
   sun_tl = new ssgTexCoordArray;
   sgSetVec2( vb, 0.0f, 0.0f );
   sun_tl->add( vb );
   sgSetVec2( vb, 1.0, 0.0 );
   sun_tl->add( vb );
   sgSetVec2( vb, 0.0, 1.0 );
   sun_tl->add( vb );
   sgSetVec2( vb, 1.0, 1.0 );
   sun_tl->add( vb );


   ssgLeaf *sun = 
	new ssgVtxTable ( GL_TRIANGLE_STRIP, sun_vl, NULL, sun_tl, sun_cl );
   sun->setState( sun_state );

   sun->setCallback( SSG_CALLBACK_PREDRAW, sgSunPreDraw );
   sun->setCallback( SSG_CALLBACK_POSTDRAW, sgSunPostDraw );
    


    // build the halo
    // sun_texbuf = new GLubyte[64*64*3];
    // sun_texid = makeHalo( sun_texbuf, 64 );
    // my_glWritePPMFile("sunhalo.ppm", sun_texbuf, 64, 64, RGB);

    // set up the inner-halo state

   ihalopath.append( "inner_halo.rgba" );

   ihalo_state = new ssgSimpleState();
   ihalo_state->setTexture( (char *)ihalopath.c_str() );
   ihalo_state->enable( GL_TEXTURE_2D );
   ihalo_state->disable( GL_LIGHTING );
   ihalo_state->setShadeModel( GL_SMOOTH );
   ihalo_state->disable( GL_CULL_FACE );
   ihalo_state->enable( GL_COLOR_MATERIAL );
   ihalo_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
   ihalo_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
   ihalo_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
   ihalo_state->enable( GL_ALPHA_TEST );
   ihalo_state->setAlphaClamp(0.01);
   ihalo_state->enable ( GL_BLEND ) ;

    // Build ssg structure
    double ihalo_size = sun_size * 2.0;
    sgVec3 vc;
    ihalo_vl = new ssgVertexArray;
    sgSetVec3( vc, -ihalo_size, 0.0, -ihalo_size );
    ihalo_vl->add( vc );
    sgSetVec3( vc, ihalo_size, 0.0, -ihalo_size );
    ihalo_vl->add( vc );
    sgSetVec3( vc, -ihalo_size, 0.0,  ihalo_size );
    ihalo_vl->add( vc );
    sgSetVec3( vc, ihalo_size, 0.0,  ihalo_size );
    ihalo_vl->add( vc );

    sgVec2 vd;
    ihalo_tl = new ssgTexCoordArray;
    sgSetVec2( vd, 0.0f, 0.0f );
    ihalo_tl->add( vd );
    sgSetVec2( vd, 1.0, 0.0 );
    ihalo_tl->add( vd );
    sgSetVec2( vd, 0.0, 1.0 );
    ihalo_tl->add( vd );
    sgSetVec2( vd, 1.0, 1.0 );
    ihalo_tl->add( vd );

    ssgLeaf *ihalo =
        new ssgVtxTable ( GL_TRIANGLE_STRIP, ihalo_vl, NULL, ihalo_tl, ihalo_cl );
    ihalo->setState( ihalo_state );

    
    // set up the outer halo state
    
    ohalopath.append( "outer_halo.rgba" );
  
    ohalo_state = new ssgSimpleState();
    ohalo_state->setTexture( (char *)ohalopath.c_str() );
    ohalo_state->enable( GL_TEXTURE_2D );
    ohalo_state->disable( GL_LIGHTING );
    ohalo_state->setShadeModel( GL_SMOOTH );
    ohalo_state->disable( GL_CULL_FACE );
    ohalo_state->enable( GL_COLOR_MATERIAL );
    ohalo_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    ohalo_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    ohalo_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    ohalo_state->enable( GL_ALPHA_TEST );
    ohalo_state->setAlphaClamp(0.01);
    ohalo_state->enable ( GL_BLEND ) ;

    // Build ssg structure
    double ohalo_size = sun_size * 7.0;
    sgVec3 ve;
    ohalo_vl = new ssgVertexArray;
    sgSetVec3( ve, -ohalo_size, 0.0, -ohalo_size );
    ohalo_vl->add( ve );
    sgSetVec3( ve, ohalo_size, 0.0, -ohalo_size );
    ohalo_vl->add( ve );
    sgSetVec3( ve, -ohalo_size, 0.0,  ohalo_size );
    ohalo_vl->add( ve );
    sgSetVec3( ve, ohalo_size, 0.0,  ohalo_size );
    ohalo_vl->add( ve );

    sgVec2 vf;
    ohalo_tl = new ssgTexCoordArray;
    sgSetVec2( vf, 0.0f, 0.0f );
    ohalo_tl->add( vf );
    sgSetVec2( vf, 1.0, 0.0 );
    ohalo_tl->add( vf );
    sgSetVec2( vf, 0.0, 1.0 );
    ohalo_tl->add( vf );
    sgSetVec2( vf, 1.0, 1.0 );
    ohalo_tl->add( vf );

    ssgLeaf *ohalo =
        new ssgVtxTable ( GL_TRIANGLE_STRIP, ohalo_vl, NULL, ohalo_tl, ohalo_cl );
    ohalo->setState( ohalo_state );


    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    sun_transform = new ssgTransform;

    ihalo->setCallback( SSG_CALLBACK_PREDRAW, sgSunHaloPreDraw );
    ihalo->setCallback( SSG_CALLBACK_POSTDRAW, sgSunHaloPostDraw );
    ohalo->setCallback( SSG_CALLBACK_PREDRAW, sgSunHaloPreDraw );
    ohalo->setCallback( SSG_CALLBACK_POSTDRAW, sgSunHaloPostDraw );

    sun_transform->addKid( ohalo );    
    sun_transform->addKid( ihalo );
    sun_transform->addKid( sun );

#ifdef FG_TEST_CHEESY_LENS_FLARE
    // cheesy lens flair
    sun_transform->addKid( new ssgaLensFlare );
#endif

    return sun_transform;
}


// repaint the sun colors based on current value of sun_angle in
// degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool SGSun::repaint( double sun_angle, double new_visibility ) {
    
	if ( visibility != new_visibility ) {
        	visibility = new_visibility;

        	static const double sqrt_m_log01 = sqrt( -log( 0.01 ) );
        	sun_exp2_punch_through = sqrt_m_log01 / ( visibility * 15 );
    	}

    	if ( prev_sun_angle != sun_angle ) {
        	prev_sun_angle = sun_angle;

		// determine how much aerosols are in the air (rough guess)
		double aerosol_factor;
		if ( visibility < 100 ){
        		aerosol_factor = 8000;
		}
		else {
        		aerosol_factor = 80.5 / log( visibility / 100 );
		}

		// get environmental data from property tree or use defaults
		double rel_humidity, density_avg;

		if ( !env_node )
		{
			rel_humidity = 0.5;
			density_avg = 0.7;
		}
		else
		{
			rel_humidity = env_node->getFloatValue( "relative-humidity" ); 
			density_avg =  env_node->getFloatValue( "atmosphere/density-tropo-avg" );
		}

		// ok, now let's go and generate the sun color
		sgVec4 i_halo_color, o_halo_color, sun_color;

		// Some comments: 
		// When the sunangle changes, light has to travel a longer distance through the atmosphere.
		// So it's scattered more due to raleigh scattering, which affects blue more than green light.
		// Red is almost not scattered and effectively only get's touched when the sun is near the horizon.
		// Visability also affects suncolor inasmuch as more particles are in the air that cause more scattering.
		// We base our calculation on the halo's color, which is most scattered. 
 
		// Red - is almost not scattered 	
		// Lambda is 700 nm
		
		double red_scat_f = ( aerosol_factor * path_distance * density_avg ) / 5E+07;
		sun_color[0] = 1 - red_scat_f;
		i_halo_color[0] = 1 - ( 1.1 * red_scat_f );
		o_halo_color[0] = 1 - ( 1.4 * red_scat_f );

		// Green - 546.1 nm
		double green_scat_f = ( aerosol_factor * path_distance * density_avg ) / 8.8938E+06;
		sun_color[1] = 1 - green_scat_f;
		i_halo_color[1] = 1 - ( 1.1 * green_scat_f );
		o_halo_color[1] = 1 - ( 1.4 * green_scat_f );
 
		// Blue - 435.8 nm
		double blue_scat_f = ( aerosol_factor * path_distance * density_avg ) / 3.607E+06;
		sun_color[2] = 1 - blue_scat_f;
		i_halo_color[2] = 1 - ( 1.1 * blue_scat_f );
		o_halo_color[2] = 1 - ( 1.4 * blue_scat_f );

		// Alpha
		sun_color[3] = 1;
		i_halo_color[3] = 1;

		o_halo_color[3] = blue_scat_f; 
		if ( ( new_visibility < 10000 ) &&  ( blue_scat_f > 1 )){
			o_halo_color[3] = 2 - blue_scat_f; 
		}


		// Now that we have the color calculated 
		// let's consider the saturation which is produced by mie scattering
		double saturation = 1 - ( rel_humidity / 200 );
		sun_color[1] += (( 1 - saturation ) * ( 1 - sun_color[1] ));
		sun_color[2] += (( 1 - saturation ) * ( 1 - sun_color[2] ));

		i_halo_color[1] += (( 1 - saturation ) * ( 1 - i_halo_color[1] ));
		i_halo_color[2] += (( 1 - saturation ) * ( 1 - i_halo_color[2] )); 

		o_halo_color[1] += (( 1 - saturation ) * ( 1 - o_halo_color[1] )); 
		o_halo_color[2] += (( 1 - saturation ) * ( 1 - o_halo_color[2] )); 

		// just to make sure we're in the limits
		if ( sun_color[0] < 0 ) sun_color[0] = 0;
		else if ( sun_color[0] > 1) sun_color[0] = 1;
		if ( i_halo_color[0] < 0 ) i_halo_color[0] = 0;
		else if ( i_halo_color[0] > 1) i_halo_color[0] = 1;
		if ( o_halo_color[0] < 0 ) o_halo_color[0] = 0;
		else if ( o_halo_color[0] > 1) o_halo_color[0] = 1;

		if ( sun_color[1] < 0 ) sun_color[1] = 0;
		else if ( sun_color[1] > 1) sun_color[1] = 1;
		if ( i_halo_color[1] < 0 ) i_halo_color[1] = 0;
		else if ( i_halo_color[1] > 1) i_halo_color[1] = 1;
		if ( o_halo_color[1] < 0 ) o_halo_color[1] = 0;
		else if ( o_halo_color[1] > 1) o_halo_color[1] = 1;

		if ( sun_color[2] < 0 ) sun_color[2] = 0;
		else if ( sun_color[2] > 1) sun_color[2] = 1;
		if ( i_halo_color[2] < 0 ) i_halo_color[2] = 0;
		else if ( i_halo_color[2] > 1) i_halo_color[2] = 1;
		if ( o_halo_color[2] < 0 ) o_halo_color[2] = 0;
		else if ( o_halo_color[2] > 1) o_halo_color[2] = 1;
		if ( o_halo_color[3] < 0 ) o_halo_color[2] = 0;
		else if ( o_halo_color[3] > 1) o_halo_color[3] = 1;

        
		gamma_correct_rgb( i_halo_color );
		gamma_correct_rgb( o_halo_color );
		gamma_correct_rgb( sun_color );	


        	float *ptr;
        	ptr = sun_cl->get( 0 );
        	sgCopyVec4( ptr, sun_color );
		ptr = ihalo_cl->get( 0 );
		sgCopyVec4( ptr, i_halo_color );
		ptr = ohalo_cl->get( 0 );
		sgCopyVec4( ptr, o_halo_color );
    }

    return true;
}


// reposition the sun at the specified right ascension and
// declination, offset by our current position (p) so that it appears
// fixed at a great distance from the viewer.  Also add in an optional
// rotation (i.e. for the current time of day.)
// Then calculate stuff needed for the sun-coloring
bool SGSun::reposition( sgVec3 p, double angle,
			double rightAscension, double declination, 
			double sun_dist, double lat, double alt_asl, double sun_angle)
{
    // GST - GMT sidereal time 
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

    // Suncolor related things:
    if ( prev_sun_angle != sun_angle ) {
      if ( sun_angle == 0 ) sun_angle = 0.1;
         const double r_earth_pole = 6356752.314;
         const double r_tropo_pole = 6356752.314 + 8000;
         const double epsilon_earth2 = 6.694380066E-3;
         const double epsilon_tropo2 = 9.170014946E-3;

         double r_tropo = r_tropo_pole / sqrt ( 1 - ( epsilon_tropo2 * pow ( cos( lat ), 2 )));
         double r_earth = r_earth_pole / sqrt ( 1 - ( epsilon_earth2 * pow ( cos( lat ), 2 )));
 
         double position_radius = r_earth + alt_asl;

         double gamma =  SG_PI - sun_angle;
         double sin_beta =  ( position_radius * sin ( gamma )  ) / r_tropo;
         double alpha =  SG_PI - gamma - asin( sin_beta );

         // OK, now let's calculate the distance the light travels
         path_distance = sqrt( pow( position_radius, 2 ) + pow( r_tropo, 2 )
                        - ( 2 * position_radius * r_tropo * cos( alpha ) ));

         double alt_half = sqrt( pow ( r_tropo, 2 ) + pow( path_distance / 2, 2 ) - r_tropo * path_distance * cos( asin( sin_beta )) ) - r_earth;

         if ( alt_half < 0.0 ) alt_half = 0.0;

         // Push the data to the property tree, so it can be used in the enviromental code
         if ( env_node ){
            env_node->setDoubleValue( "atmosphere/altitude-troposphere-top", r_tropo - r_earth );
            env_node->setDoubleValue( "atmosphere/altitude-half-to-sun", alt_half );
      }
    }

    return true;
}
