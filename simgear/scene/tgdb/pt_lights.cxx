// pt_lights.cxx -- build a 'directional' light on the fly
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "pt_lights.hxx"

#include <map>

#include <osg/Array>
#include <osg/Geometry>
#include <osg/CullFace>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/NodeCallback>
#include <osg/NodeVisitor>
#include <osg/Texture2D>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/TexEnv>
#include <osg/Sequence>
#include <osg/Fog>
#include <osg/FragmentProgram>
#include <osg/VertexProgram>
#include <osg/Point>
#include <osg/Material>
#include <osg/Group>
#include <osg/StateSet>

#include <osgUtil/CullVisitor>

#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>

#include <simgear/math/sg_random.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGEnlargeBoundingBox.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/Technique.hxx>
#include <simgear/scene/material/Pass.hxx>

#include "SGVasiDrawable.hxx"

using OpenThreads::Mutex;
using OpenThreads::ScopedLock;

using namespace osg;
using namespace simgear;

static Mutex lightMutex;

namespace
{
typedef std::tuple<float, osg::Vec3, float, float, bool> PointParams;
typedef std::map<PointParams, observer_ptr<Effect> > EffectMap;

EffectMap effectMap;
}

Effect* getLightEffect(float size, const Vec3& attenuation,
                       float minSize, float maxSize, bool directional,
                       const SGReaderWriterOptions* options)
{
    PointParams pointParams(size, attenuation, minSize, maxSize, directional);
    ScopedLock<Mutex> lock(lightMutex);
    ref_ptr<Effect> effect;
    EffectMap::iterator eitr = effectMap.find(pointParams);
    if (eitr != effectMap.end())
    {
        if (eitr->second.lock(effect))
            return effect.release();
    }

    SGPropertyNode_ptr effectProp = new SGPropertyNode;
    if (directional) {
    	makeChild(effectProp, "inherits-from")->setStringValue("Effects/surface-lights-directional");
    } else {
    	makeChild(effectProp, "inherits-from")->setStringValue("Effects/surface-lights");
    }

    SGPropertyNode* params = makeChild(effectProp, "parameters");
    params->getNode("size",true)->setValue(size);
    params->getNode("attenuation",true)->getNode("x", true)->setValue(attenuation.x());
    params->getNode("attenuation",true)->getNode("y", true)->setValue(attenuation.y());
    params->getNode("attenuation",true)->getNode("z", true)->setValue(attenuation.z());
    params->getNode("min-size",true)->setValue(minSize);
    params->getNode("max-size",true)->setValue(maxSize);
    params->getNode("cull-face",true)->setValue(directional ? "back" : "off");
    params->getNode("light-directional",true)->setValue(directional);

    effect = makeEffect(effectProp, true, options);

    if (eitr == effectMap.end())
        effectMap.insert(std::make_pair(pointParams, effect));
    else
        eitr->second = effect; // update existing, but empty observer
    return effect.release();
}


osg::Drawable*
SGLightFactory::getLightDrawable(const SGLightBin::Light& light)
{
  osg::Vec3Array* vertices = new osg::Vec3Array;
  osg::Vec4Array* colors = new osg::Vec4Array;

  vertices->push_back(toOsg(light.position));
  colors->push_back(toOsg(light.color));

  osg::Geometry* geometry = new osg::Geometry;
  geometry->setDataVariance(osg::Object::STATIC);
  geometry->setVertexArray(vertices);
  geometry->setNormalBinding(osg::Geometry::BIND_OFF);
  geometry->setColorArray(colors);
  geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

  // Enlarge the bounding box to avoid such light nodes being victim to
  // small feature culling.
  geometry->setComputeBoundingBoxCallback(new SGEnlargeBoundingBox(1));

  osg::DrawArrays* drawArrays;
  drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS,
                                   0, vertices->size());
  geometry->addPrimitiveSet(drawArrays);
  return geometry;
}

osg::Drawable*
SGLightFactory::getLightDrawable(const SGDirectionalLightBin::Light& light, bool useTriangles)
{
  osg::Vec3Array* vertices = new osg::Vec3Array;
  osg::Vec4Array* colors = new osg::Vec4Array;

  if (useTriangles) {
    SGVec4f visibleColor(light.color);
    SGVec4f invisibleColor(visibleColor[0], visibleColor[1],
                           visibleColor[2], 0);
    SGVec3f normal = normalize(light.normal);
    SGVec3f perp1 = perpendicular(normal);
    SGVec3f perp2 = cross(normal, perp1);
    SGVec3f position = light.position;
    vertices->push_back(toOsg(position));
    vertices->push_back(toOsg(position + perp1));
    vertices->push_back(toOsg(position + perp2));
    colors->push_back(toOsg(visibleColor));
    colors->push_back(toOsg(invisibleColor));
    colors->push_back(toOsg(invisibleColor));

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setDataVariance(osg::Object::STATIC);
    geometry->setVertexArray(vertices);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    // Enlarge the bounding box to avoid such light nodes being victim to
    // small feature culling.
    geometry->setComputeBoundingBoxCallback(new SGEnlargeBoundingBox(1));

    osg::DrawArrays* drawArrays;
    drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES,
                                     0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);
    return geometry;
  } else {
    // Workaround for driver issue where point sprites cannot be triangles,
    // (which we use to provide a sensible normal).  So fall back to a simple
    // non-directional point sprite.
    const SGLightBin::Light nonDirectionalLight = SGLightBin::Light(light.position, light.color);
    return getLightDrawable(nonDirectionalLight);
  }
}

namespace
{
  ref_ptr<StateSet> simpleLightSS;
}
osg::Drawable*
SGLightFactory::getLights(const SGLightBin& lights, unsigned inc, float alphaOff)
{
  if (lights.getNumLights() <= 0)
    return 0;

  osg::Vec3Array* vertices = new osg::Vec3Array;
  osg::Vec4Array* colors = new osg::Vec4Array;

  for (unsigned i = 0; i < lights.getNumLights(); i += inc) {
    vertices->push_back(toOsg(lights.getLight(i).position));
    SGVec4f color = lights.getLight(i).color;
    color[3] = SGMiscf::max(0, SGMiscf::min(1, color[3] + alphaOff));
    colors->push_back(toOsg(color));
  }

  osg::Geometry* geometry = new osg::Geometry;
  geometry->setDataVariance(osg::Object::STATIC);
  geometry->setVertexArray(vertices);
  geometry->setNormalBinding(osg::Geometry::BIND_OFF);
  geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);

  osg::DrawArrays* drawArrays;
  drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS,
                                   0, vertices->size());
  geometry->addPrimitiveSet(drawArrays);

  {
    ScopedLock<Mutex> lock(lightMutex);
    if (!simpleLightSS.valid()) {
      StateAttributeFactory *attrFact = StateAttributeFactory::instance();
      simpleLightSS = new StateSet;
      simpleLightSS->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");
      simpleLightSS->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
      simpleLightSS->setAttributeAndModes(attrFact->getStandardBlendFunc());
      simpleLightSS->setAttributeAndModes(attrFact->getStandardAlphaFunc());
    }
  }
  geometry->setStateSet(simpleLightSS.get());
  return geometry;
}


osg::Drawable*
SGLightFactory::getLights(const SGDirectionalLightBin& lights)
{
  if (lights.getNumLights() <= 0)
    return 0;

  osg::Vec3Array* vertices = new osg::Vec3Array;
  osg::Vec4Array* colors = new osg::Vec4Array;

  for (unsigned i = 0; i < lights.getNumLights(); ++i) {
	  SGVec4f visibleColor(lights.getLight(i).color);
	  SGVec4f invisibleColor(visibleColor[0], visibleColor[1],
	                         visibleColor[2], 0);
	  SGVec3f normal = normalize(lights.getLight(i).normal);
	  SGVec3f perp1 = perpendicular(normal);
	  SGVec3f perp2 = cross(normal, perp1);
	  SGVec3f position = lights.getLight(i).position;
	  vertices->push_back(toOsg(position));
	  vertices->push_back(toOsg(position + perp1));
	  vertices->push_back(toOsg(position + perp2));
	  colors->push_back(toOsg(visibleColor));
	  colors->push_back(toOsg(invisibleColor));
	  colors->push_back(toOsg(invisibleColor));
  }

  osg::Geometry* geometry = new osg::Geometry;
  geometry->setDataVariance(osg::Object::STATIC);
  geometry->setVertexArray(vertices);
  geometry->setNormalBinding(osg::Geometry::BIND_OFF);
  geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);


  //osg::StateSet* stateSet = geometry->getOrCreateStateSet();
  //stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");
  //stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);


  static SGSceneFeatures* sceneFeatures = SGSceneFeatures::instance();
  bool useTriangles = sceneFeatures->getEnableTriangleDirectionalLights();

  osg::DrawArrays* drawArrays;
  if (useTriangles)
    drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES,
                                     0, vertices->size());
  else
   drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS,
                                     0, vertices->size());

  geometry->addPrimitiveSet(drawArrays);
  return geometry;
}

static SGVasiDrawable*
buildVasi(const SGDirectionalLightBin& lights, const SGVec3f& up,
       const SGVec4f& red, const SGVec4f& white)
{
  unsigned count = lights.getNumLights();
  if ( count == 4 ) {
    SGVasiDrawable* drawable = new SGVasiDrawable(red, white);

    // PAPI configuration
    // papi D
    drawable->addLight(lights.getLight(0).position,
                       lights.getLight(0).normal, up, 3.5);
    // papi C
    drawable->addLight(lights.getLight(1).position,
                       lights.getLight(1).normal, up, 3.167);
    // papi B
    drawable->addLight(lights.getLight(2).position,
                       lights.getLight(2).normal, up, 2.833);
    // papi A
    drawable->addLight(lights.getLight(3).position,
                       lights.getLight(3).normal, up, 2.5);
    return drawable;

  } else if (count == 6) {
    SGVasiDrawable* drawable = new SGVasiDrawable(red, white);

    // probably vasi, first 3 are downwind bar (2.5 deg)
    for (unsigned i = 0; i < 3; ++i)
      drawable->addLight(lights.getLight(i).position,
                         lights.getLight(i).normal, up, 2.5);
    // last 3 are upwind bar (3.0 deg)
    for (unsigned i = 3; i < 6; ++i)
      drawable->addLight(lights.getLight(i).position,
                         lights.getLight(i).normal, up, 3.0);
    return drawable;

  } else if (count == 12) {
    SGVasiDrawable* drawable = new SGVasiDrawable(red, white);

    // probably vasi, first 6 are downwind bar (2.5 deg)
    for (unsigned i = 0; i < 6; ++i)
      drawable->addLight(lights.getLight(i).position,
                         lights.getLight(i).normal, up, 2.5);
    // last 6 are upwind bar (3.0 deg)
    for (unsigned i = 6; i < 12; ++i)
      drawable->addLight(lights.getLight(i).position,
                         lights.getLight(i).normal, up, 3.0);
    return drawable;

  } else {
    // fail safe
    SG_LOG(SG_TERRAIN, SG_ALERT,
           "unknown vasi/papi configuration, count = " << count);
    return 0;
  }
}

osg::Drawable*
SGLightFactory::getVasi(const SGVec3f& up, const SGDirectionalLightBin& lights,
                        const SGVec4f& red, const SGVec4f& white)
{
  SGVasiDrawable* drawable = buildVasi(lights, up, red, white);
  if (!drawable)
    return 0;

  osg::StateSet* stateSet = drawable->getOrCreateStateSet();
  stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");
  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

  osg::BlendFunc* blendFunc = new osg::BlendFunc;
  stateSet->setAttribute(blendFunc);
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);

  osg::AlphaFunc* alphaFunc;
  alphaFunc = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01);
  stateSet->setAttribute(alphaFunc);
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);

  return drawable;
}

osg::Node*
SGLightFactory::getSequenced(const SGDirectionalLightBin& lights, const SGReaderWriterOptions* options)
{
  if (lights.getNumLights() <= 0)
    return 0;

  static SGSceneFeatures* sceneFeatures = SGSceneFeatures::instance();
  bool useTriangles = sceneFeatures->getEnableTriangleDirectionalLights();

  // generate a repeatable random seed
  sg_srandom(unsigned(lights.getLight(0).position[0]));
  float flashTime = 0.065 + 0.003 * sg_random();
  osg::Sequence* sequence = new osg::Sequence;
  sequence->setDefaultTime(flashTime);
  Effect* effect = getLightEffect(24.0f, osg::Vec3(1.0, 0.0001, 0.000001),
                                  1.0f, 24.0f, true, options);
  for (int i = lights.getNumLights() - 1; 0 <= i; --i) {
    EffectGeode* egeode = new EffectGeode;
    egeode->setEffect(effect);
    egeode->addDrawable(getLightDrawable(lights.getLight(i), useTriangles));
    sequence->addChild(egeode, flashTime);
  }
  sequence->addChild(new osg::Group, 1.9 + (0.1 * sg_random()) - (lights.getNumLights() * flashTime));
  sequence->setInterval(osg::Sequence::LOOP, 0, -1);
  sequence->setDuration(1.0f, -1);
  sequence->setMode(osg::Sequence::START);
  sequence->setSync(true);
  return sequence;
}

osg::Node*
SGLightFactory::getReil(const SGDirectionalLightBin& lights, const SGReaderWriterOptions* options)
{
  if (lights.getNumLights() <= 0)
    return 0;

  static SGSceneFeatures* sceneFeatures = SGSceneFeatures::instance();
  bool useTriangles = sceneFeatures->getEnableTriangleDirectionalLights();

  // generate a repeatable random seed
  sg_srandom(unsigned(lights.getLight(0).position[0]));
  float flashTime = 0.065 + 0.003 * sg_random();
  osg::Sequence* sequence = new osg::Sequence;
  sequence->setDefaultTime(flashTime);
  Effect* effect = getLightEffect(24.0f, osg::Vec3(1.0, 0.0001, 0.000001),
                                  1.0f, 24.0f, true, options);
  EffectGeode* egeode = new EffectGeode;
  egeode->setEffect(effect);

  for (int i = lights.getNumLights() - 1; 0 <= i; --i) {
    egeode->addDrawable(getLightDrawable(lights.getLight(i), useTriangles));
  }
  sequence->addChild(egeode, flashTime);
  sequence->addChild(new osg::Group, 1.9 + 0.1 * sg_random() - flashTime);
  sequence->setInterval(osg::Sequence::LOOP, 0, -1);
  sequence->setDuration(1.0f, -1);
  sequence->setMode(osg::Sequence::START);
  sequence->setSync(true);
  return sequence;
}

osg::Node*
SGLightFactory::getOdal(const SGLightBin& lights, const SGReaderWriterOptions* options)
{
  if (lights.getNumLights() < 2)
    return 0;

  // generate a repeatable random seed
  sg_srandom(unsigned(lights.getLight(0).position[0]));
  float flashTime = 0.065 + 0.003 * sg_random();
  osg::Sequence* sequence = new osg::Sequence;
  sequence->setDefaultTime(flashTime);
  Effect* effect = getLightEffect(20.0f, osg::Vec3(1.0, 0.0001, 0.000001),
                                  1.0f, 20.0f, false, options);
  // centerline lights
  for (int i = lights.getNumLights() - 1; i >= 2; i--) {
    EffectGeode* egeode = new EffectGeode;
    egeode->setEffect(effect);
    egeode->addDrawable(getLightDrawable(lights.getLight(i)));
    sequence->addChild(egeode, flashTime);
  }
  // add extra empty group for a break
  sequence->addChild(new osg::Group, 4 * flashTime);
  // runway end lights
  EffectGeode* egeode = new EffectGeode;
  egeode->setEffect(effect);
  for (unsigned i = 0; i < 2; ++i) {
    egeode->addDrawable(getLightDrawable(lights.getLight(i)));
  }
  sequence->addChild(egeode, flashTime);

  // add an extra empty group for a break
  sequence->addChild(new osg::Group, 1.9 + (0.1 * sg_random()) - ((lights.getNumLights() + 2) * flashTime));
  sequence->setInterval(osg::Sequence::LOOP, 0, -1);
  sequence->setDuration(1.0f, -1);
  sequence->setMode(osg::Sequence::START);
  sequence->setSync(true);

  return sequence;
}

// Blinking hold short line lights
osg::Node*
SGLightFactory::getHoldShort(const SGDirectionalLightBin& lights, const SGReaderWriterOptions* options)
{
  if (lights.getNumLights() < 2)
    return 0;

  static SGSceneFeatures* sceneFeatures = SGSceneFeatures::instance();
  bool useTriangles = sceneFeatures->getEnableTriangleDirectionalLights();

  sg_srandom(unsigned(lights.getLight(0).position[0]));
  float flashTime = 0.9 + 0.2 * sg_random();
  osg::Sequence* sequence = new osg::Sequence;

  // start with lights off
  sequence->addChild(new osg::Group, 0.2);
  // ...and increase the lights in steps
  for (int i = 0; i < 5; i++) {
      Effect* effect = getLightEffect(12.0f + i, osg::Vec3(1, 0.001, 0.0002),
                                      1.0f, 12.0f + i, true, options);
      EffectGeode* egeode = new EffectGeode;
      egeode->setEffect(effect);
      for (unsigned int j = 0; j < lights.getNumLights(); ++j) {
          egeode->addDrawable(getLightDrawable(lights.getLight(j), useTriangles));
      }
      sequence->addChild(egeode, (i==4) ? flashTime : 0.1);
  }
  sequence->setInterval(osg::Sequence::SWING, 0, -1);
  sequence->setDuration(1.0f, -1);
  sequence->setMode(osg::Sequence::START);

  return sequence;
}

// Alternating runway guard lights ("wig-wag")
osg::Node*
SGLightFactory::getGuard(const SGDirectionalLightBin& lights, const SGReaderWriterOptions* options)
{
  if (lights.getNumLights() < 2)
    return 0;

  static SGSceneFeatures* sceneFeatures = SGSceneFeatures::instance();
  bool useTriangles = sceneFeatures->getEnableTriangleDirectionalLights();

  // generate a repeatable random seed
  sg_srandom(unsigned(lights.getLight(0).position[0]));
  float flashTime = 0.9 + 0.2 * sg_random();
  osg::Sequence* sequence = new osg::Sequence;
  sequence->setDefaultTime(flashTime);
  Effect* effect = getLightEffect(16.0f, osg::Vec3(1.0, 0.001, 0.0002),
                                  1.0f, 16.0f, true, options);
  for (unsigned int i = 0; i < lights.getNumLights(); ++i) {
    EffectGeode* egeode = new EffectGeode;
    egeode->setEffect(effect);
    egeode->addDrawable(getLightDrawable(lights.getLight(i), useTriangles));
    sequence->addChild(egeode, flashTime);
  }
  sequence->setInterval(osg::Sequence::LOOP, 0, -1);
  sequence->setDuration(1.0f, -1);
  sequence->setMode(osg::Sequence::START);
  sequence->setSync(true);
  return sequence;
}
