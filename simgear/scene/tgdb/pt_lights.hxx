// pt_lights.hxx -- build a 'directional' light on the fly
//
// Written by Curtis Olson, started March 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _SG_PT_LIGHTS_HXX
#define _SG_PT_LIGHTS_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>

#include <string>
#include <vector>		// STL

#include <osg/Drawable>
#include <osg/Node>
#include <osg/Point>

#include <simgear/math/sg_types.hxx>
#include <simgear/scene/material/matlib.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>

#include "SGLightBin.hxx"
#include "SGDirectionalLightBin.hxx"

SG_USING_STD(string);
SG_USING_STD(vector);

// Specify the way we want to draw directional point lights (assuming the
// appropriate extensions are available.)

inline void SGConfigureDirectionalLights( bool use_point_sprites,
                                   bool enhanced_lighting,
                                   bool distance_attenuation ) {
  static SGSceneFeatures* sceneFeatures = SGSceneFeatures::instance();
  sceneFeatures->setEnablePointSpriteLights(use_point_sprites);
  sceneFeatures->setEnableDistanceAttenuationLights(distance_attenuation);
}

class SGPointSpriteLightCullCallback : public osg::NodeCallback {
public:
  SGPointSpriteLightCullCallback(const osg::Vec3& da = osg::Vec3(1, 0.001, 0.0002),
                                 float sz = 4);
  SGPointSpriteLightCullCallback(osg::Point* point);

  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);

private:
  osg::ref_ptr<osg::StateSet> _pointSpriteStateSet;
  osg::ref_ptr<osg::StateSet> _distanceAttenuationStateSet;
};

class SGLightFactory {
public:

  static osg::Node*
  getLight(const SGLightBin::Light& light);

  static osg::Node*
  getLight(const SGDirectionalLightBin::Light& light);

  static osg::Drawable*
  getLights(const SGLightBin& lights, unsigned inc = 1, float alphaOff = 0);

  static osg::Drawable*
  getLights(const SGDirectionalLightBin& lights);

  static osg::Drawable*
  getVasi(const SGVec3f& up, const SGDirectionalLightBin& lights,
          const SGVec4f& red, const SGVec4f& white);

  static osg::Node*
  getSequenced(const SGDirectionalLightBin& lights);

  static osg::Node*
  getOdal(const SGLightBin& lights);
};

#endif // _SG_PT_LIGHTS_HXX
