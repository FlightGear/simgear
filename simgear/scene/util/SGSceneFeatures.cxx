/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGSceneFeatures.hxx"

#include <osg/Version>
#include <osg/FragmentProgram>
#include <osg/VertexProgram>
#include <osg/Point>
#include <osg/PointSprite>
#include <osg/Texture>
#include <osg/GLExtensions>

#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>

#include <simgear/structure/SGSharedPtr.hxx>

SGSceneFeatures::SGSceneFeatures() :
  _textureCompression(UseARBCompression),
  _MaxTextureSize(4096),
  _TextureCacheCompressionActive(true),
  _TextureCacheCompressionActiveTransparent(true),
  _TextureCacheActive(false),
  _shaderLights(true),
  _pointSpriteLights(true),
  _triangleDirectionalLights(true),
  _distanceAttenuationLights(true),
  _textureFilter(1),
  _VPBActive(false)
{
}

static OpenThreads::Mutex mutexSGSceneFeatures_instance;
SGSceneFeatures*
SGSceneFeatures::instance()
{
  static SGSharedPtr<SGSceneFeatures> sceneFeatures;
  if (sceneFeatures)
    return sceneFeatures;
  OpenThreads::ScopedLock<OpenThreads::Mutex> lock(mutexSGSceneFeatures_instance);
  if (sceneFeatures)
    return sceneFeatures;
  sceneFeatures = new SGSceneFeatures;
  return sceneFeatures;
}

void
SGSceneFeatures::applyTextureCompression(osg::Texture* texture) const
{
    // if the texture cache is active, always use the file data format
    if (_TextureCacheActive && _TextureCacheCompressionActive) {
        texture->setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);
    } else {
        // keep the older texture compression working, some people need it
        switch (_textureCompression) {
        case UseARBCompression:
          texture->setInternalFormatMode(osg::Texture::USE_ARB_COMPRESSION);
          break;
        case UseDXT1Compression:
          texture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT1_COMPRESSION);
          break;
        case UseDXT3Compression:
          texture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT3_COMPRESSION);
          break;
        case UseDXT5Compression:
          texture->setInternalFormatMode(osg::Texture::USE_S3TC_DXT5_COMPRESSION);
          break;
        default:
          texture->setInternalFormatMode(osg::Texture::USE_IMAGE_DATA_FORMAT);
          break;
        }
    }
}

bool
SGSceneFeatures::getHavePointSprites(unsigned contextId) const
{
  const osg::GLExtensions* ex = osg::GLExtensions::Get(contextId, true);
  return ex && ex->isPointSpriteSupported;
}

bool
SGSceneFeatures::getHaveFragmentPrograms(unsigned contextId) const
{
  const osg::GLExtensions* ex = osg::GLExtensions::Get(contextId, true);
  return ex && ex->isFragmentProgramSupported;
}

bool
SGSceneFeatures::getHaveVertexPrograms(unsigned contextId) const
{
  const osg::GLExtensions* ex = osg::GLExtensions::Get(contextId, true);
  return ex && ex->isVertexProgramSupported;
}

bool
SGSceneFeatures::getHaveShaderPrograms(unsigned contextId) const
{
  if (!getHaveFragmentPrograms(contextId))
    return false;
  return getHaveVertexPrograms(contextId);
}

bool
SGSceneFeatures::getHavePointParameters(unsigned contextId) const
{
  const osg::GLExtensions* ex = osg::GLExtensions::Get(contextId, true);
  return ex && ex->isPointParametersSupported;
}
