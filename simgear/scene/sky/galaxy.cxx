// galaxy.cxx -- model the celestial sphere brightness by unresolved
// sources, i.e. the milky way and its nebulae: our Galaxy
//
// Started November 2021 (Chris Ringeval)
//
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <stdio.h>
#include <iostream>

#include <osg/Array>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Node>
#include <osg/ShadeModel>
#include <osg/TexEnv>
#include <osg/Texture2D>

#include <simgear/constants.h>
#include <simgear/screen/colors.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>

#include "sphere.hxx"
#include "galaxy.hxx"

using namespace simgear;
using std::max;
using std::min;

// Constructor
SGGalaxy::SGGalaxy( SGPropertyNode* props )
{
  if (props) {
    _magDarkSkyProperty = props->getNode("darksky-brightness-magnitude");
  }
}


// Destructor
SGGalaxy::~SGGalaxy( void ) {
}


// build the galaxy sphere object
osg::Node*
SGGalaxy::build( SGPath path, double galaxy_size, simgear::SGReaderWriterOptions *options ) {

  simgear::EffectGeode* orb = SGMakeSphere(galaxy_size, 64, 32);
      
  Effect *effect = makeEffect("Effects/galaxy", true, options);
  if (effect) {
    orb->setEffect(effect);
  }

  // set up the orb state
  osg::StateSet* stateSet = orb->getOrCreateStateSet();
  stateSet->setRenderBinDetails(-9, "RenderBin");    
  
  osg::ref_ptr<SGReaderWriterOptions> poptions;
  poptions = SGReaderWriterOptions::fromPath(path);
  
  osg::Texture2D* texture = SGLoadTexture2D("allsky_brightness_magten.png", poptions.get(),true,true);
  texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
  texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);

  stateSet->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

  osg::TexEnv* texEnv = new osg::TexEnv;
  texEnv->setMode(osg::TexEnv::MODULATE);
  stateSet->setTextureAttribute(0, texEnv, osg::StateAttribute::ON);
    
  osg::ShadeModel* shadeModel = new osg::ShadeModel;
  shadeModel->setMode(osg::ShadeModel::SMOOTH);
  stateSet->setAttributeAndModes(shadeModel);
  
  osg::BlendFunc* blendFunc = new osg::BlendFunc;
  blendFunc->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE);
  stateSet->setAttributeAndModes(blendFunc);

  // needed shader side
  zenith_brightness_magnitude = new osg::Uniform("fg_ZenithSkyBrightness", 0.0f);
  stateSet->addUniform(zenith_brightness_magnitude);
  
  // force a repaint of the galaxy colors with arbitrary defaults
  repaint( 0.0, 0.0 );

  // build the scene graph sub tree for the Galaxy
  galaxy_transform = new osg::MatrixTransform;
  galaxy_transform->addChild( orb );

  // reposition the Galaxy's texture, which is in galactic
  // coordinates, into the "fake" geocentric frame (which is carried
  // along our current position (p))
  //
  // Coordinates of the galactic north pole used with Gaia data (from
  // which our Milky Way Texture is built).
  //
  // https://www.cosmos.esa.int/web/gaia-users/archive/gedr3-documentation-pdf
  // Section 4.1.7.1 page 198
  
  const double galactic_north_pole_RA = 192.85948;
  const double galactic_north_pole_DEC = 27.12825;
  const double equatorial_north_pole_THETA = 122.93192;
   
  osg::Matrix RA, DEC, THETA;

  // RA origin at 90 degrees
  RA.makeRotate((galactic_north_pole_RA-90.0)*SGD_DEGREES_TO_RADIANS, osg::Vec3(0, 0, 1));
  // Rotate along rotated x-axis by -(90-DEC)
  DEC.makeRotate((galactic_north_pole_DEC-90.0)*SGD_DEGREES_TO_RADIANS, osg::Vec3(1, 0, 0));
  // Set the origin of the galactic longitude in Sagittarius, rotate
  // along rotated z-axis by -theta
  THETA.makeRotate(-equatorial_north_pole_THETA*SGD_DEGREES_TO_RADIANS, osg::Vec3(0, 0, 1));

  galaxy_transform->setMatrix(THETA*DEC*RA);
  
  return galaxy_transform.get();
}


// Basic evaluation of the dark sky brightness magnitude at zenith,
// according to the sun angles above the horizon. The actual painting
// and coloring of the Galaxy is done within the shaders using this
// value as a starting point, and adding angular dependent scattering
// and Moon illumation effects when relevant. The idea being that the
// intrinsic Galaxy brightness is eventually masked by the brightness
// of the atmosphere and we do see their relative contrast
bool SGGalaxy::repaint( double sun_angle, double altitude_m ) {

  osg::Vec4 rhodcolor;

  double sundeg;
  double mindeg;
  
  double mudarksky;
  double musky;
  
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

  // the darkest possible value of the sky brightness at zenith
  // (darker = larger number). If a prop is defined, we use it, or we
  // default to the one coded here (should be 22 for the darkest skies
  // on Earth)
  if (_magDarkSkyProperty) {
    mudarksky = _magDarkSkyProperty->getDoubleValue();
  }
  else {
    mudarksky = _magDarkSkyDefault;
  }
  
  // initializing sky brightness to a lot
  musky = 0.0;

  // in space, we either have sun in the face or not. If sundeg >=
  // mindeg, the sun is visible, we see nothing, then we do
  // nothing. Otherwise, we have
  if (sundeg <= mindeg) {

  // sun illumination of the atmosphere at zenith (fit to SQM zenital measurements)
  // from http://www.hnsky.org/sqm_twilight.htm, slightly modified
  // to be continuous at 0deg, -12deg and -18deg
  // musky is in magV / arcsec^2    
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
  
  //cout << "sundeg= " << sundeg << endl;
  //cout << "mindeg= " << mindeg << endl;
  //cout << "altitude= " << altitude_m << endl;
  //cout << "musky= mudarksky= " << musky <<" "<<mudarksky << endl;

  //the uniform feeding the shaders
  zenith_brightness_magnitude->set((float)musky);

  return true;
}


