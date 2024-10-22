// stars.cxx -- model the stars
//
// Written by Durk Talsma. Originally started October 1997, for distribution  
// with the FlightGear project. Version 2 was written in August and 
// September 1998. This code is based upon algorithms and data kindly 
// provided by Mr. Paul Schlyter. (pausch@saaf.se). 
//
// Separated out rendering pieces and converted to ssg by Curt Olson,
// March 2000
//
// Switch to sky brightness as a mean to sort visible stars to be
// consistent with Milky Way visibility, can be modified from the
// property tree from local lighting environment

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
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include <stdio.h>
#include <iostream>
#include <algorithm>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Image>
#include <osg/Material>
#include <osg/Point>
#include <osg/ShadeModel>
#include <osg/Node>

#include "stars.hxx"

using namespace simgear;
using std::max;
using std::min;

// Constructor
SGStars::SGStars( SGPropertyNode* props ) :
    old_phase(-1)
{
    if (props) {
        // don't create here - if it's not defined, we won't use the value
        // from a property
      _magDarkSkyProperty = props->getNode("darksky-brightness-magnitude");
    }
}


// Destructor
SGStars::~SGStars( void ) {
}


// initialize the stars object and connect it into our scene graph root
osg::Node*
SGStars::build( int num, const SGVec3d star_data[], double star_dist,
                SGReaderWriterOptions* options ) {
    EffectGeode* geode = new EffectGeode;
    geode->setName("Stars");

    Effect* effect = makeEffect("Effects/stars", true, options);
    if (effect)
        geode->setEffect(effect);

    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-9, "RenderBin");

    // set up the star state
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setFunction(osg::BlendFunc::SRC_ALPHA,
                           osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);

//     osg::Point* point = new osg::Point;
//     point->setSize(5);
//     stateSet->setAttributeAndModes(point);

    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);

    // Build scenegraph structure
    
    cl = new osg::Vec4Array;
    osg::Vec3Array* vl = new osg::Vec3Array;

    // Build scenegraph structure
    for ( int i = 0; i < num; ++i ) {
        // position seeded to arbitrary values
        vl->push_back(osg::Vec3(star_dist * cos( star_data[i][0])
                                * cos( star_data[i][1] ),
                                star_dist * sin( star_data[i][0])
                                * cos( star_data[i][1] ),
                                star_dist * sin( star_data[i][1])));

        // color (magnitude)
        cl->push_back(osg::Vec4(1, 1, 1, 1));
    }

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(vl);
    geometry->setColorArray(cl.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vl->size()));
    geode->addDrawable(geometry);

    return geode;
}


// repaint the sun colors based on current value of sun_angle in
// degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool SGStars::repaint( double sun_angle, double altitude_m, int num, const SGVec3d star_data[] )
{
  
  double mag, nmag, alpha, factor, cutoff;

  double magmax;
  double sundeg, mindeg;
  double musky, mudarksky;

  //observer visual acuity in the model used below (F=2)
  const double logF = 0.30;

  // same as moon.cxx
  const double earth_radius_in_meters = 6371000.0;
 
  //sundeg is elevation above the horizon
  sundeg = 90.0 - sun_angle * SGD_RADIANS_TO_DEGREES;

  // mindeg is the elevation above the horizon at which the sun is
  // no longer obstructed by the Earth (mindeg <= 0)
  mindeg = 0.0;
  if (altitude_m >=0) {
    mindeg = -90.0 + SGD_RADIANS_TO_DEGREES * asin(earth_radius_in_meters/(altitude_m + earth_radius_in_meters));
  }
  // if the prop exists, let's use its value, can be real time changed
  // due to lighting conditions. Otherwise, we use the default
  if (_magDarkSkyProperty) {
    mudarksky = _magDarkSkyProperty->getDoubleValue();
  }
  else {
    mudarksky = _magDarkSkyDefault;
  }
  
  // initializing sky brightness to a lot
  musky = 0.0;

  // same as in galaxy.cxx
  // in space, we either have the sun in the face or not. If sundeg >= mindeg,
  // the sun is visible, we see nothing. Otherwise we have:
  if (sundeg <= mindeg) {

    // same little model as galaxy.cxx, sun illumination of the
    // atmosphere at zenith (fit to SQM zenital measurements) from
    // http://www.hnsky.org/sqm_twilight.htm, slightly modified to be
    // continuous at 0deg, -12deg and -18deg musky is in magV /
    // arcsec^2
    
    if ( (sundeg >= -12.0) && (sundeg < 0.0) ) {
      musky =  - 1.057*sundeg + mudarksky - 14.7528;
    }
      
    if ( (sundeg >= -18.0 ) && (sundeg < -12.0) ) {
      musky = -0.0744*sundeg*sundeg - 2.5768*sundeg + mudarksky - 22.2768;
      musky = min(musky,mudarksky);
    }

    if ( sundeg < -18.0) {
      musky = mudarksky;
    }

  }

  //  Simple relation between maximal star magnitudes visible by the
  //  naked eye on Earth, from Eq.(90) and (91) of astro-ph/1405.4209
  //
  // For 19.5 < musky < 22
  // mmax = 0.3834 musky - 1.4400 - 2.5 * log(F)
  //
  // For 18 <  musky < 20
  // mmax = 0.270 musky _0.8 - 2.5 * log(F)
  //
  // Typical values, let's take F = 2 for healthy pilot. With mudarksky ~ 22
  // mag/arcsec^2 => mmax=6.2
  //
  // We use these linear formulae and switch from one to the other at
  // their intersection point
      
    if (musky >= 19.823) {
      magmax = 0.383 * musky - 1.44 - 2.5 * logF;
    }
    else {
      //extrapolated to all bright (small) musky values 
      magmax = 0.270 * musky + 0.80 - 2.5 * logF;
    }
    
    // sirius, brightest star (not brightest object)
    double mag_min = -1.46;
    
    int phase;

    //continously changed at each call to repaint, but we use "phase"
    //to actually check for repainting, not magmax
    
    cutoff = magmax;
        
    // determine which star structure to draw when the sun is not
    // directly visible
    if (sundeg <= mindeg) {
      if ( sun_angle > (SGD_PI_2 + 18.0 * SGD_DEGREES_TO_RADIANS ) ) {
        // deep night, atmosphere is not lighten by the sun
        factor = 1.0;
        phase = 0;
      } else if ( sun_angle > (SGD_PI_2 + 12.0 * SGD_DEGREES_TO_RADIANS ) ) {
        // less than 18deg and more than 12deg is astronomical twilight
        factor = 1.0;
        phase = 1;
      } else if ( sun_angle > (SGD_PI_2 + 9.0 * SGD_DEGREES_TO_RADIANS ) ) {
        // less 12deg and more than 6deg is is nautical twilight 
        factor = 1.0;
        phase = 2;
      } else if ( sun_angle > (SGD_PI_2 + 7.5 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.95;
        phase = 3;
      } else if ( sun_angle > (SGD_PI_2 + 7.0 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.9;
        phase = 4;
      } else if ( sun_angle > (SGD_PI_2 + 6.5 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.85;
        phase = 5;
      } else if ( sun_angle > (SGD_PI_2 + 6.0 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.8;
        phase = 6;
      } else if ( sun_angle > (SGD_PI_2 + 5.5 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.75;
        phase = 7;
      } else {
        // early dusk or late dawn
        factor = 0.7;
        cutoff = 0.0;
        phase = 8;
      }
    } else {
      // at large altitudes (in space), this conditional is triggered
      // for sun >=mindeg, the sun is directly visible, let's call it
      // phase 9
        factor = 1.0;
        cutoff = 0.0;
	phase = 9;
    }
    
    // repaint only for change of phase or if darksky property has been changed
    
    if ((phase != old_phase) || (mudarksky != _cachedMagDarkSky)) {
        old_phase = phase;
        _cachedMagDarkSky = mudarksky;

	//cout << "  phase change -> repainting stars, num = " << num << endl;
	//cout << "mudarksky= musky= cutoff= " << mudarksky << " " << musky << " " << cutoff << endl;
	
        for ( int i = 0; i < num; ++i ) {
            // if ( star_data[i][2] < min ) { min = star_data[i][2]; }
            // if ( star_data[i][2] > max ) { max = star_data[i][2]; }

            // magnitude ranges from -1 (bright) to 6 (dim).  The
            // range of star and planet magnitudes can actually go
            // outside of this, but for our purpose, if it is brighter
            // that magmin, we'll color it full white/alpha anyway

            // color (magnitude)
            mag = star_data[i][2];
            if ( mag < cutoff ) {
                nmag = ( cutoff - mag ) / (cutoff - mag_min); // translate to 0 ... 1.0 scale
		//with Milky Way on, it is more realistic to make the
		//stars fainting to total darkness when matching the
		//background sky brightness
		//
		//alpha = nmag * 0.85 + 0.15; //
		//translate to a 0.15 ... 1.0 scale
		//
		alpha = nmag;
                alpha *= factor;          // dim when the sun is brighter
            } else {
                alpha = 0.0;
            }

            if (alpha > 1.0) { alpha = 1.0; }
            if (alpha < 0.0) { alpha = 0.0; }

            (*cl)[i] = osg::Vec4(1, 1, 1, alpha);
        }
        cl->dirty();
    }

    // cout << "min = " << min << " max = " << max << " count = " << num 
    //      << endl;

    return true;
}
