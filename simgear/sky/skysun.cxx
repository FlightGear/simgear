// skysun.hxx -- draw a sun object
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


// #ifdef __BORLANDC__
// #  define exception c_exception
// #endif
// #include <math.h>

// #include <simgear/debug/logstream.hxx>

// #include <Time/sunpos.hxx>
// #include <Time/light.hxx>
// #include <Main/options.hxx>


#include "skysun.hxx"


// Constructor
FGSkySun::FGSkySun( void ) {
}


// Destructor
FGSkySun::~FGSkySun( void ) {
}


// initialize the sun object and connect it into our scene graph root
bool FGSkySun::initialize() {
    sgVec3 color;

    float theta;
    int i;

    // create the scene graph for the dome
    skysun = new ssgRoot;
    skysun->setName( "Sky Sun" );

    // set up the state
    sun_state = new ssgSimpleState();
    sun_state->setShadeModel( GL_SMOOTH );
    sun_state->disable( GL_LIGHTING );
    sun_state->disable( GL_DEPTH_TEST );
    sun_state->disable( GL_CULL_FACE );
    sun_state->disable( GL_TEXTURE_2D );
    sun_state->disable( GL_COLOR_MATERIAL );
    sun_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );

    // initially seed to all white
    sgSetVec3( color, 1.0, 1.0, 1.0 );

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
    dome_selector = new ssgSelector;
    dome_transform = new ssgTransform;

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

    dome_transform->addKid( center_disk );
    dome_transform->addKid( upper_ring );
    dome_transform->addKid( middle_ring );
    dome_transform->addKid( lower_ring );

    dome_selector->addKid( dome_transform );
    dome_selector->clrTraversalMaskBits( SSGTRAV_HOT );

    dome->addKid( dome_selector );

    return true;
}


#if 0
/*************************************************************************
 * Star::Star(FGTime *t)
 * Public constructor for class Star
 * Argument: The current time.
 * the hard coded orbital elements our sun are passed to 
 * CelestialBody::CelestialBody();
 * note that the word sun is avoided, in order to prevent some compilation
 * problems on sun systems 
 ************************************************************************/
Star::Star(FGTime *t) :
  CelestialBody (0.000000,  0.0000000000,
		 0.0000,    0.00000,
		 282.9404,  4.7093500E-5,	
		 1.0000000, 0.000000,	
		 0.016709,  -1.151E-9,
		 356.0470,  0.98560025850, t)
{
    
  FG_LOG( FG_GENERAL, FG_INFO, "Initializing Sun Texture");
#ifdef GL_VERSION_1_1
  xglGenTextures(1, &sun_texid);
  xglBindTexture(GL_TEXTURE_2D, sun_texid);
#elif GL_EXT_texture_object
  xglGenTexturesEXT(1, &sun_texid);
  xglBindTextureEXT(GL_TEXTURE_2D, sun_texid);
#else
#  error port me
#endif

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  setTexture();
  glTexImage2D( GL_TEXTURE_2D,
		0,
		GL_RGBA,
		256, 256,
		0,
		GL_RGBA, GL_UNSIGNED_BYTE,
		sun_texbuf);
     
  SunObject = gluNewQuadric();
  if(SunObject == NULL)
    {
      printf("gluNewQuadric(SunObject) failed  !\n");
      exit(0);
    }
  
  //SunList = 0;
  distance = 0.0;
}

Star::~Star()
{
  //delete SunObject;
  delete [] sun_texbuf;
}



static int texWidth = 256;	/* 64x64 is plenty */

void Star::setTexture()
{
  int texSize;
  //void *textureBuf;
  GLubyte *p;
  int i,j;
  double radius;
  
  texSize = texWidth*texWidth;
  
  sun_texbuf = new GLubyte[texSize*4];
  if (!sun_texbuf) 
    return;  // Ugly!
  
  p = sun_texbuf;
  
  radius = (double)(texWidth / 2);
  
  for (i=0; i < texWidth; i++) {
    for (j=0; j < texWidth; j++) {
      double x, y, d;
	    
      *p = 0xff;
      *(p+1) = 0xff;
      *(p+2) = 0xff;

      x = fabs((double)(i - (texWidth / 2)));
      y = fabs((double)(j - (texWidth / 2)));

      d = sqrt((x * x) + (y * y));
      if (d < radius) {
	  double t = 1.0 - (d / radius); // t is 1.0 at center, 0.0 at edge */
	  // inverse square looks nice 
	  *(p+3) = (int)((double) 0xff * (t*t));
      } else {
	  *(p+3) = 0x00;
      }
      p += 4;
    }
  }
  //gluBuild2DMipmaps(GL_TEXTURE_2D, 1, texWidth, texWidth, 
  //	    GL_LUMINANCE,
  //	    GL_UNSIGNED_BYTE, textureBuf);
  //free(textureBuf);
}
/*************************************************************************
 * void Jupiter::updatePosition(FGTime *t, Star *ourSun)
 * 
 * calculates the current position of our sun.
 *************************************************************************/
void Star::updatePosition(FGTime *t)
{
  double 
    actTime, eccAnom, 
    xv, yv, v, r,
    xe, ye, ze, ecl;

  updateOrbElements(t);
  
  actTime = fgCalcActTime(t);
  ecl = DEG_TO_RAD * (23.4393 - 3.563E-7 * actTime); // Angle in Radians
  eccAnom = fgCalcEccAnom(M, e);  // Calculate the eccentric Anomaly (also known as solving Kepler's equation)
  
  xv = cos(eccAnom) - e;
  yv = sqrt (1.0 - e*e) * sin(eccAnom);
  v = atan2 (yv, xv);                   // the sun's true anomaly
  distance = r = sqrt (xv*xv + yv*yv);  // and its distance

  lonEcl = v + w; // the sun's true longitude
  latEcl = 0;

  // convert the sun's true longitude to ecliptic rectangular 
  // geocentric coordinates (xs, ys)
  xs = r * cos (lonEcl);
  ys = r * sin (lonEcl);

  // convert ecliptic coordinates to equatorial rectangular
  // geocentric coordinates

  xe = xs;
  ye = ys * cos (ecl);
  ze = ys * sin (ecl);

  // And finally, calculate right ascension and declination
  rightAscension = atan2 (ye, xe);
  declination = atan2 (ze, sqrt (xe*xe + ye*ye));
}
  
void Star::newImage(void)
{
  /*static float stars[3];
  stars[0] = 0.0;
  stars[1] = 0.0;
  stars[2] = 1.0;*/

  fgLIGHT *l = &cur_light_params;
  float sun_angle = l->sun_angle;
  
  if( sun_angle*RAD_TO_DEG < 100 ) { // else no need to draw sun
    
    
    double x_2, x_4, x_8, x_10;
    GLfloat ambient;
    GLfloat amb[4];
    int sun_size = 550;
    
    // daily variation sun gets larger near horizon
    /*if(sun_angle*RAD_TO_DEG > 84.0 && sun_angle*RAD_TO_DEG < 95)
      {
      double sun_grow = 9*fabs(94-sun_angle*RAD_TO_DEG);
      sun_size = (int)(sun_size + sun_size * cos(sun_grow*DEG_TO_RAD));
      }*/
    x_2 = sun_angle * sun_angle;
    x_4 = x_2 * x_2;
    x_8 = x_4 * x_4;
    x_10 = x_8 * x_2;
    ambient = (float)(0.4 * pow (1.1, - x_10 / 30.0));
    if (ambient < 0.3) ambient = 0.3;
    if (ambient > 1.0) ambient = 1.0;
    
    amb[0] = ((ambient * 6.0)  - 1.0); // minimum value = 0.8
    amb[1] = ((ambient * 11.0) - 3.0); // minimum value = 0.3
    amb[2] = ((ambient * 12.0) - 3.6); // minimum value = 0.0
    amb[3] = 1.00;
    
    if (amb[0] > 1.0) amb[0] = 1.0;
    if (amb[1] > 1.0) amb[1] = 1.0;
    if (amb[2] > 1.0) amb[2] = 1.0;
    xglColor3fv(amb);
    glPushMatrix();
    {
      xglRotatef(((RAD_TO_DEG * rightAscension)- 90.0), 0.0, 0.0, 1.0);
      xglRotatef((RAD_TO_DEG * declination), 1.0, 0.0, 0.0);
      xglTranslatef(0,60000,0);
      if (current_options.get_textures())
	{
	  glEnable(GL_TEXTURE_2D); // TEXTURE ENABLED
	  glEnable(GL_BLEND);	// BLEND ENABLED
	  
	  // glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	  glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) ;
	  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);  
	  glBindTexture(GL_TEXTURE_2D, sun_texid);

	  glBegin(GL_QUADS);
	  glTexCoord2f(0.0f, 0.0f); glVertex3f(-5000, 0.0, -5000);
	  glTexCoord2f(1.0f, 0.0f); glVertex3f( 5000, 0.0, -5000);
	  glTexCoord2f(1.0f, 1.0f); glVertex3f( 5000, 0.0,  5000);
	  glTexCoord2f(0.0f, 1.0f); glVertex3f(-5000, 0.0,  5000);
	  glEnd();
	}
      xglDisable(GL_TEXTURE_2D); // TEXTURE DISABLED
      xglDisable(GL_BLEND);	// BLEND DISABLED
    }

    glPopMatrix();
    glDisable(GL_LIGHTING);	// LIGHTING DISABLED
    glDisable(GL_BLEND);	// BLEND DISABLED
    glPushMatrix();
    {     
      xglRotatef(((RAD_TO_DEG * rightAscension)- 90.0), 0.0, 0.0, 1.0);
      xglRotatef((RAD_TO_DEG * declination), 1.0, 0.0, 0.0);
      xglColor4fv(amb);
      xglTranslatef(0,60000,0);
      gluSphere( SunObject,  sun_size, 10, 10 );
      }
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);	// TEXTURE DISABLED
    glDisable(GL_BLEND);	// BLEND DISABLED  
  }
}
#endif
