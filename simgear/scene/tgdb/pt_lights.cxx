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
#include <boost/tuple/tuple_comparison.hpp>

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
#include <osg/PolygonMode>
#include <osg/Fog>
#include <osg/FragmentProgram>
#include <osg/VertexProgram>
#include <osg/Point>
#include <osg/PointSprite>
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

static void
setPointSpriteImage(unsigned char* data, unsigned log2resolution,
                    unsigned charsPerPixel)
{
  int env_tex_res = (1 << log2resolution);
  for (int i = 0; i < env_tex_res; ++i) {
    for (int j = 0; j < env_tex_res; ++j) {
      int xi = 2*i + 1 - env_tex_res;
      int yi = 2*j + 1 - env_tex_res;
      if (xi < 0)
        xi = -xi;
      if (yi < 0)
        yi = -yi;
      
      xi -= 1;
      yi -= 1;
      
      if (xi < 0)
        xi = 0;
      if (yi < 0)
        yi = 0;
      
      float x = 1.5*xi/(float)(env_tex_res);
      float y = 1.5*yi/(float)(env_tex_res);
      //       float x = 2*xi/(float)(env_tex_res);
      //       float y = 2*yi/(float)(env_tex_res);
      float dist = sqrt(x*x + y*y);
      float bright = SGMiscf::clip(255*(1-dist), 0, 255);
      for (unsigned l = 0; l < charsPerPixel; ++l)
        data[charsPerPixel*(i*env_tex_res + j) + l] = (unsigned char)bright;
    }
  }
}

static osg::Image*
getPointSpriteImage(int logResolution)
{
  osg::Image* image = new osg::Image;
  
  osg::Image::MipmapDataType mipmapOffsets;
  unsigned off = 0;
  for (int i = logResolution; 0 <= i; --i) {
    unsigned res = 1 << i;
    off += res*res;
    mipmapOffsets.push_back(off);
  }
  
  int env_tex_res = (1 << logResolution);
  
  unsigned char* imageData = new unsigned char[off];
  image->setImage(env_tex_res, env_tex_res, 1,
                  GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, imageData,
                  osg::Image::USE_NEW_DELETE);
  image->setMipmapLevels(mipmapOffsets);
  
  for (int k = logResolution; 0 <= k; --k) {
    setPointSpriteImage(image->getMipmapData(logResolution - k), k, 1);
  }
  
  return image;
}

static Mutex lightMutex;

static osg::Texture2D*
gen_standard_light_sprite(void)
{
  // Always called from when the lightMutex is already taken
  static osg::ref_ptr<osg::Texture2D> texture;
  if (texture.valid())
    return texture.get();
  
  texture = new osg::Texture2D;
  texture->setImage(getPointSpriteImage(6));
  texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
  texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);
  
  return texture.get();
}

namespace
{
typedef boost::tuple<float, osg::Vec3, float, float, bool> PointParams;
typedef std::map<PointParams, ref_ptr<Effect> > EffectMap;

EffectMap effectMap;

ref_ptr<PolygonMode> polyMode = new PolygonMode(PolygonMode::FRONT,
                                                PolygonMode::POINT);
ref_ptr<PointSprite> pointSprite = new PointSprite;
}

Effect* getLightEffect(float size, const Vec3& attenuation,
                       float minSize, float maxSize, bool directional)
{
    PointParams pointParams(size, attenuation, minSize, maxSize, directional);
    ScopedLock<Mutex> lock(lightMutex);
    EffectMap::iterator eitr = effectMap.find(pointParams);
    if (eitr != effectMap.end())
        return eitr->second.get();
    // Basic stuff; no sprite or attenuation support
    Pass *basicPass = new Pass;
    basicPass->setRenderBinDetails(POINT_LIGHTS_BIN, "RenderBin");
    basicPass->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    StateAttributeFactory *attrFact = StateAttributeFactory::instance();
    basicPass->setAttributeAndModes(attrFact->getStandardBlendFunc());
    basicPass->setAttributeAndModes(attrFact->getStandardAlphaFunc());
    if (directional) {
        basicPass->setAttributeAndModes(attrFact->getCullFaceBack());
        basicPass->setAttribute(polyMode.get());
    }
    Pass *attenuationPass = clone(basicPass, CopyOp::SHALLOW_COPY);
    osg::Point* point = new osg::Point;
    point->setMinSize(minSize);
    point->setMaxSize(maxSize);
    point->setSize(size);
    point->setDistanceAttenuation(attenuation);
    attenuationPass->setAttributeAndModes(point);
    Pass *spritePass = clone(basicPass, CopyOp::SHALLOW_COPY);
    spritePass->setTextureAttributeAndModes(0, pointSprite.get(),
                                            osg::StateAttribute::ON);
    Texture2D* texture = gen_standard_light_sprite();
    spritePass->setTextureAttribute(0, texture);
    spritePass->setTextureMode(0, GL_TEXTURE_2D,
                               osg::StateAttribute::ON);
    spritePass->setTextureAttribute(0, attrFact->getStandardTexEnv());
    Pass *combinedPass = clone(spritePass, CopyOp::SHALLOW_COPY);
    combinedPass->setAttributeAndModes(point);
    Effect* effect = new Effect;
    std::vector<std::string> parameterExtensions;

    if (SGSceneFeatures::instance()->getEnablePointSpriteLights())
    {
        std::vector<std::string> combinedExtensions;
        combinedExtensions.push_back("GL_ARB_point_sprite");
        combinedExtensions.push_back("GL_ARB_point_parameters");
        Technique* combinedTniq = new Technique;
        combinedTniq->passes.push_back(combinedPass);
        combinedTniq->setGLExtensionsPred(2.0, combinedExtensions);
        effect->techniques.push_back(combinedTniq);
        std::vector<std::string> spriteExtensions;
        spriteExtensions.push_back(combinedExtensions.front());
        Technique* spriteTniq = new Technique;
        spriteTniq->passes.push_back(spritePass);
        spriteTniq->setGLExtensionsPred(2.0, spriteExtensions);
        effect->techniques.push_back(spriteTniq);
        parameterExtensions.push_back(combinedExtensions.back());
    }

    Technique* parameterTniq = new Technique;
    parameterTniq->passes.push_back(attenuationPass);
    parameterTniq->setGLExtensionsPred(1.4, parameterExtensions);
    effect->techniques.push_back(parameterTniq);
    Technique* basicTniq = new Technique(true);
    basicTniq->passes.push_back(basicPass);
    effect->techniques.push_back(basicTniq);
    effectMap.insert(std::make_pair(pointParams, effect));
    return effect;
}


osg::Drawable*
SGLightFactory::getLightDrawable(const SGLightBin::Light& light)
{
  osg::Vec3Array* vertices = new osg::Vec3Array;
  osg::Vec4Array* colors = new osg::Vec4Array;

  vertices->push_back(toOsg(light.position));
  colors->push_back(toOsg(light.color));
  
  osg::Geometry* geometry = new osg::Geometry;
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
SGLightFactory::getLightDrawable(const SGDirectionalLightBin::Light& light)
{
  osg::Vec3Array* vertices = new osg::Vec3Array;
  osg::Vec4Array* colors = new osg::Vec4Array;

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
  
  geometry->setVertexArray(vertices);
  geometry->setNormalBinding(osg::Geometry::BIND_OFF);
  geometry->setColorArray(colors);
  geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
  
  osg::DrawArrays* drawArrays;
  drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS,
                                   0, vertices->size());
  geometry->addPrimitiveSet(drawArrays);

  {
    ScopedLock<Mutex> lock(lightMutex);
    if (!simpleLightSS.valid()) {
      StateAttributeFactory *attrFact = StateAttributeFactory::instance();
      simpleLightSS = new StateSet;
      simpleLightSS->setRenderBinDetails(POINT_LIGHTS_BIN, "RenderBin");
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
  
  geometry->setVertexArray(vertices);
  geometry->setNormalBinding(osg::Geometry::BIND_OFF);
  geometry->setColorArray(colors);
  geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
  
  osg::DrawArrays* drawArrays;
  drawArrays = new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES,
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
  stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "RenderBin");
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
SGLightFactory::getSequenced(const SGDirectionalLightBin& lights)
{
  if (lights.getNumLights() <= 0)
    return 0;

  // generate a repeatable random seed
  sg_srandom(unsigned(lights.getLight(0).position[0]));
  float flashTime = 2e-2 + 5e-3*sg_random();
  osg::Sequence* sequence = new osg::Sequence;
  sequence->setDefaultTime(flashTime);
  Effect* effect = getLightEffect(40.0f, osg::Vec3(1.0, 0.0001, 0.00000001),
                                  10.0f, 40.0f, true);
  for (int i = lights.getNumLights() - 1; 0 <= i; --i) {
    EffectGeode* egeode = new EffectGeode;
    egeode->setEffect(effect);
    egeode->addDrawable(getLightDrawable(lights.getLight(i)));
    sequence->addChild(egeode, flashTime);
  }
  sequence->addChild(new osg::Group, 1 + 1e-1*sg_random());
  sequence->setInterval(osg::Sequence::LOOP, 0, -1);
  sequence->setDuration(1.0f, -1);
  sequence->setMode(osg::Sequence::START);
  sequence->setSync(true);
  return sequence;
}

osg::Node*
SGLightFactory::getOdal(const SGLightBin& lights)
{
  if (lights.getNumLights() < 2)
    return 0;

  // generate a repeatable random seed
  sg_srandom(unsigned(lights.getLight(0).position[0]));
  float flashTime = 2e-2 + 5e-3*sg_random();
  osg::Sequence* sequence = new osg::Sequence;
  sequence->setDefaultTime(flashTime);
  Effect* effect = getLightEffect(40.0f, osg::Vec3(1.0, 0.0001, 0.00000001),
                                  10.0, 40.0, false);
  // centerline lights
  for (int i = lights.getNumLights() - 1; 2 <= i; --i) {
    EffectGeode* egeode = new EffectGeode;
    egeode->setEffect(effect);
    egeode->addDrawable(getLightDrawable(lights.getLight(i)));
    sequence->addChild(egeode, flashTime);
  }
  // runway end lights
  osg::Group* group = new osg::Group;
  for (unsigned i = 0; i < 2; ++i) {
    EffectGeode* egeode = new EffectGeode;
    egeode->setEffect(effect);
    egeode->addDrawable(getLightDrawable(lights.getLight(i)));
    group->addChild(egeode);
  }
  sequence->addChild(group, flashTime);

  // add an extra empty group for a break
  sequence->addChild(new osg::Group, 9 + 1e-1*sg_random());
  sequence->setInterval(osg::Sequence::LOOP, 0, -1);
  sequence->setDuration(1.0f, -1);
  sequence->setMode(osg::Sequence::START);
  sequence->setSync(true);

  return sequence;
}
