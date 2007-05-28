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

#include <osg/FragmentProgram>
#include <osg/VertexProgram>
#include <osg/Point>
#include <osg/PointSprite>

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/threads/SGThread.hxx>
#include <simgear/threads/SGGuard.hxx>

SGSceneFeatures::SGSceneFeatures() :
  _shaderLights(true),
  _pointSpriteLights(true),
  _distanceAttenuationLights(true)
{
}

SGSceneFeatures*
SGSceneFeatures::instance()
{
  static SGSharedPtr<SGSceneFeatures> sceneFeatures;
  if (sceneFeatures)
    return sceneFeatures;
  static SGMutex mutex;
  SGGuard<SGMutex> guard(mutex);
  if (sceneFeatures)
    return sceneFeatures;
  sceneFeatures = new SGSceneFeatures;
  return sceneFeatures;
}

bool
SGSceneFeatures::getHavePointSprites(unsigned contextId) const
{
  return osg::PointSprite::isPointSpriteSupported(contextId);
}

bool
SGSceneFeatures::getHaveFragmentPrograms(unsigned contextId) const
{
  const osg::FragmentProgram::Extensions* fpe;
  fpe = osg::FragmentProgram::getExtensions(contextId, true);
  if (!fpe)
    return false;
  if (!fpe->isFragmentProgramSupported())
    return false;
  
  return true;
}

bool
SGSceneFeatures::getHaveVertexPrograms(unsigned contextId) const
{
  const osg::VertexProgram::Extensions* vpe;
  vpe = osg::VertexProgram::getExtensions(contextId, true);
  if (!vpe)
    return false;
  if (!vpe->isVertexProgramSupported())
    return false;
  
  return true;
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
  const osg::Point::Extensions* pe;
  pe = osg::Point::getExtensions(contextId, true);
  if (!pe)
    return false;
  if (!pe->isPointParametersSupported())
    return false;
  return true;
}

