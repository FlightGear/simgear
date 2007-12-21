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

#include <simgear/math/sg_random.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/threads/SGGuard.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGEnlargeBoundingBox.hxx>

#include "SGVasiDrawable.hxx"

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

static osg::Texture2D*
gen_standard_light_sprite(void)
{
  // double checked locking ...
  static osg::ref_ptr<osg::Texture2D> texture;
  if (texture.valid())
    return texture.get();
  
  static SGMutex mutex;
  SGGuard<SGMutex> guard(mutex);
  if (texture.valid())
    return texture.get();
  
  texture = new osg::Texture2D;
  texture->setImage(getPointSpriteImage(6));
  texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
  texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);
  
  return texture.get();
}

SGPointSpriteLightCullCallback::SGPointSpriteLightCullCallback(const osg::Vec3& da,
                                                               float sz) :
  _pointSpriteStateSet(new osg::StateSet),
  _distanceAttenuationStateSet(new osg::StateSet)
{
  osg::PointSprite* pointSprite = new osg::PointSprite;
  _pointSpriteStateSet->setTextureAttributeAndModes(0, pointSprite,
                                                    osg::StateAttribute::ON);
  osg::Texture2D* texture = gen_standard_light_sprite();
  _pointSpriteStateSet->setTextureAttribute(0, texture);
  _pointSpriteStateSet->setTextureMode(0, GL_TEXTURE_2D,
                                       osg::StateAttribute::ON);
  osg::TexEnv* texEnv = new osg::TexEnv;
  texEnv->setMode(osg::TexEnv::MODULATE);
  _pointSpriteStateSet->setTextureAttribute(0, texEnv);
  
  osg::Point* point = new osg::Point;
  point->setFadeThresholdSize(1);
  point->setMinSize(1);
  point->setMaxSize(sz);
  point->setSize(sz);
  point->setDistanceAttenuation(da);
  _distanceAttenuationStateSet->setAttributeAndModes(point);
}

// FIXME make state sets static
SGPointSpriteLightCullCallback::SGPointSpriteLightCullCallback(osg::Point* point) :
  _pointSpriteStateSet(new osg::StateSet),
  _distanceAttenuationStateSet(new osg::StateSet)
{
  osg::PointSprite* pointSprite = new osg::PointSprite;
  _pointSpriteStateSet->setTextureAttributeAndModes(0, pointSprite,
                                                    osg::StateAttribute::ON);
  osg::Texture2D* texture = gen_standard_light_sprite();
  _pointSpriteStateSet->setTextureAttribute(0, texture);
  _pointSpriteStateSet->setTextureMode(0, GL_TEXTURE_2D,
                                       osg::StateAttribute::ON);
  osg::TexEnv* texEnv = new osg::TexEnv;
  texEnv->setMode(osg::TexEnv::MODULATE);
  _pointSpriteStateSet->setTextureAttribute(0, texEnv);
  
  _distanceAttenuationStateSet->setAttributeAndModes(point);
}

void
SGPointSpriteLightCullCallback::operator()(osg::Node* node,
                                           osg::NodeVisitor* nv)
{
  assert(dynamic_cast<osgUtil::CullVisitor*>(nv));
  osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
  
  // Test for point sprites and point parameters availibility
  unsigned contextId = cv->getRenderInfo().getContextID();
  SGSceneFeatures* features = SGSceneFeatures::instance();
  bool usePointSprite = features->getEnablePointSpriteLights(contextId);
  bool usePointParameters = features->getEnableDistanceAttenuationLights(contextId);
  
  if (usePointSprite)
    cv->pushStateSet(_pointSpriteStateSet.get());
  
  if (usePointParameters)
    cv->pushStateSet(_distanceAttenuationStateSet.get());
  
  traverse(node, nv);
  
  if (usePointParameters)
    cv->popStateSet();
  
  if (usePointSprite)
    cv->popStateSet();
}

osg::Node*
SGLightFactory::getLight(const SGLightBin::Light& light)
{
  osg::Vec3Array* vertices = new osg::Vec3Array;
  osg::Vec4Array* colors = new osg::Vec4Array;

  vertices->push_back(light.position.osg());
  colors->push_back(light.color.osg());
  
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
  
  osg::StateSet* stateSet = geometry->getOrCreateStateSet();
  stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");
  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  osg::BlendFunc* blendFunc = new osg::BlendFunc;
  stateSet->setAttribute(blendFunc);
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  
  osg::AlphaFunc* alphaFunc;
  alphaFunc = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01);
  stateSet->setAttribute(alphaFunc);
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
  
  osg::Geode* geode = new osg::Geode;
  geode->addDrawable(geometry);

  return geode;
}

osg::Node*
SGLightFactory::getLight(const SGDirectionalLightBin::Light& light)
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
  vertices->push_back(position.osg());
  vertices->push_back((position + perp1).osg());
  vertices->push_back((position + perp2).osg());
  colors->push_back(visibleColor.osg());
  colors->push_back(invisibleColor.osg());
  colors->push_back(invisibleColor.osg());
  
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
  
  osg::StateSet* stateSet = geometry->getOrCreateStateSet();
  stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");

  osg::Material* material = new osg::Material;
  material->setColorMode(osg::Material::OFF);
  stateSet->setAttribute(material);

  osg::CullFace* cullFace = new osg::CullFace;
  cullFace->setMode(osg::CullFace::BACK);
  stateSet->setAttribute(cullFace, osg::StateAttribute::ON);
  stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
  
  osg::PolygonMode* polygonMode = new osg::PolygonMode;
  polygonMode->setMode(osg::PolygonMode::FRONT, osg::PolygonMode::POINT);
  stateSet->setAttribute(polygonMode);

  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  osg::BlendFunc* blendFunc = new osg::BlendFunc;
  stateSet->setAttribute(blendFunc);
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  
  osg::AlphaFunc* alphaFunc;
  alphaFunc = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01);
  stateSet->setAttribute(alphaFunc);
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
  
  osg::Geode* geode = new osg::Geode;
  geode->addDrawable(geometry);

  return geode;
}

osg::Drawable*
SGLightFactory::getLights(const SGLightBin& lights, unsigned inc, float alphaOff)
{
  if (lights.getNumLights() <= 0)
    return 0;
  
  osg::Vec3Array* vertices = new osg::Vec3Array;
  osg::Vec4Array* colors = new osg::Vec4Array;

  for (unsigned i = 0; i < lights.getNumLights(); i += inc) {
    vertices->push_back(lights.getLight(i).position.osg());
    SGVec4f color = lights.getLight(i).color;
    color[3] = SGMiscf::max(0, SGMiscf::min(1, color[3] + alphaOff));
    colors->push_back(color.osg());
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
  
  osg::StateSet* stateSet = geometry->getOrCreateStateSet();
  stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");

  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  osg::BlendFunc* blendFunc = new osg::BlendFunc;
  stateSet->setAttribute(blendFunc);
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  
  osg::AlphaFunc* alphaFunc;
  alphaFunc = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01);
  stateSet->setAttribute(alphaFunc);
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
  
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
    vertices->push_back(position.osg());
    vertices->push_back((position + perp1).osg());
    vertices->push_back((position + perp2).osg());
    colors->push_back(visibleColor.osg());
    colors->push_back(invisibleColor.osg());
    colors->push_back(invisibleColor.osg());
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
  
  osg::StateSet* stateSet = geometry->getOrCreateStateSet();
  stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");

  osg::Material* material = new osg::Material;
  material->setColorMode(osg::Material::OFF);
  stateSet->setAttribute(material);

  osg::CullFace* cullFace = new osg::CullFace;
  cullFace->setMode(osg::CullFace::BACK);
  stateSet->setAttribute(cullFace, osg::StateAttribute::ON);
  stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
  
  osg::PolygonMode* polygonMode = new osg::PolygonMode;
  polygonMode->setMode(osg::PolygonMode::FRONT, osg::PolygonMode::POINT);
  stateSet->setAttribute(polygonMode);

  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  
  osg::BlendFunc* blendFunc = new osg::BlendFunc;
  stateSet->setAttribute(blendFunc);
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  
  osg::AlphaFunc* alphaFunc;
  alphaFunc = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01);
  stateSet->setAttribute(alphaFunc);
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
  
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
  }
  else if (count == 12) {
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
SGLightFactory::getSequenced(const SGDirectionalLightBin& lights)
{
  if (lights.getNumLights() <= 0)
    return 0;

  // generate a repeatable random seed
  sg_srandom(unsigned(lights.getLight(0).position[0]));
  float flashTime = 2e-2 + 5e-3*sg_random();
  osg::Sequence* sequence = new osg::Sequence;
  sequence->setDefaultTime(flashTime);

  for (int i = lights.getNumLights() - 1; 0 <= i; --i)
    sequence->addChild(getLight(lights.getLight(i)), flashTime);
  sequence->addChild(new osg::Group, 1 + 1e-1*sg_random());
  sequence->setInterval(osg::Sequence::LOOP, 0, -1);
  sequence->setDuration(1.0f, -1);
  sequence->setMode(osg::Sequence::START);
  sequence->setSync(true);

  osg::StateSet* stateSet = sequence->getOrCreateStateSet();
  stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");
  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

  osg::BlendFunc* blendFunc = new osg::BlendFunc;
  stateSet->setAttribute(blendFunc);
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  
  osg::AlphaFunc* alphaFunc;
  alphaFunc = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01);
  stateSet->setAttribute(alphaFunc);
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);

  osg::Point* point = new osg::Point;
  point->setMinSize(6);
  point->setMaxSize(10);
  point->setSize(10);
  point->setDistanceAttenuation(osg::Vec3(1.0, 0.0001, 0.00000001));
  sequence->setCullCallback(new SGPointSpriteLightCullCallback(point));

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

  // centerline lights
  for (int i = lights.getNumLights() - 1; 2 <= i; --i)
    sequence->addChild(getLight(lights.getLight(i)), flashTime);

  // runway end lights
  osg::Group* group = new osg::Group;
  for (unsigned i = 0; i < 2; ++i)
    group->addChild(getLight(lights.getLight(i)));
  sequence->addChild(group, flashTime);

  // add an extra empty group for a break
  sequence->addChild(new osg::Group, 9 + 1e-1*sg_random());
  sequence->setInterval(osg::Sequence::LOOP, 0, -1);
  sequence->setDuration(1.0f, -1);
  sequence->setMode(osg::Sequence::START);
  sequence->setSync(true);

  osg::StateSet* stateSet = sequence->getOrCreateStateSet();
  stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");
  stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

  osg::BlendFunc* blendFunc = new osg::BlendFunc;
  stateSet->setAttribute(blendFunc);
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  
  osg::AlphaFunc* alphaFunc;
  alphaFunc = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01);
  stateSet->setAttribute(alphaFunc);
  stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);

  osg::Point* point = new osg::Point;
  point->setMinSize(6);
  point->setMaxSize(10);
  point->setSize(10);
  point->setDistanceAttenuation(osg::Vec3(1.0, 0.0001, 0.00000001));
  sequence->setCullCallback(new SGPointSpriteLightCullCallback(point));

  return sequence;
}
