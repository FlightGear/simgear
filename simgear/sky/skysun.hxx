// skysun.hxx -- draw a sun object
//
// Written by Durk Talsma. Originally started October 1997, for distribution  
// with the FlightGear project. Version 2 was written in August and 
// September 1998. This code is based upon algorithms and data kindly 
// provided by Mr. Paul Schlyter. (pausch@saaf.se). 
//
// Separated out rendering pieces and converted to ssg by Curt Olson,
// March 2000
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


#ifndef _SKYSUN_HXX_
#define _SKYSUN_HXX_


#include <plib/ssg.h>

class FGSkySun {

    // scene graph root for the skysun
    ssgRoot *skysun;

    ssgSelector *sun_selector;
    ssgTransform *sun_transform;
    ssgSimpleState *orb_state;
    ssgSimpleState *halo_state;

    ssgColourArray *cl;

public:

    // Constructor
    FGSkySun( void );

    // Destructor
    ~FGSkySun( void );

    // initialize the sun object and connect it into our scene graph
    // root
    bool initialize();

    // repaint the sun colors based on current value of sun_anglein
    // degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( double sun_angle );

    // reposition the sun at the specified right ascension and
    // declination, offset by our current position (p) so that it
    // appears fixed at a great distance from the viewer.  Also add in
    // an optional rotation (i.e. for the current time of day.)
    bool reposition( sgVec3 p, double angle,
		     double rightAscension, double declination );

    // Draw the sun
    bool draw();

    // enable the sun in the scene graph (default)
    void enable() { sun_selector->select( 1 ); }

    // disable the sun in the scene graph.  The leaf node is still
    // there, how ever it won't be traversed on the cullandrender
    // phase.
    void disable() { sun_selector->select( 0 ); }

};


#if 0
class Star : public CelestialBody
{
private:
  //double longitude;  // the sun's true longitude - this is depreciated by
                       // CelestialBody::lonEcl 
  double xs, ys;     // the sun's rectangular geocentric coordinates
  double distance;   // the sun's distance to the earth
   GLUquadricObj *SunObject;
  GLuint sun_texid;
  GLubyte *sun_texbuf;

  void setTexture();
public:
  Star (FGTime *t);
  ~Star();
  void updatePosition(FGTime *t);
  double getM();
  double getw();
  //double getLon();
  double getxs();
  double getys();
  double getDistance();
  void newImage();
};



inline double Star::getM()
{
  return M;
}

inline double Star::getw()
{
  return w;
}

inline double Star::getxs()
{
  return xs;
}

inline double Star::getys()
{
  return ys;
}

inline double Star::getDistance()
{
  return distance;
}
#endif


#endif // _SKYSUN_HXX_














