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

#ifndef SG_SCENE_FEATURES_HXX
#define SG_SCENE_FEATURES_HXX

#include <simgear/structure/SGReferenced.hxx>

namespace osg { class Texture; }

class SGSceneFeatures : public SGReferenced {
public:
  static SGSceneFeatures* instance();

  enum TextureCompression {
    DoNotUseCompression,
    UseARBCompression,
    UseDXT1Compression,
    UseDXT3Compression,
    UseDXT5Compression
  };

  void setTextureCompression(TextureCompression textureCompression)
  { _textureCompression = textureCompression; }
  TextureCompression getTextureCompression() const
  { return _textureCompression; }
  void setTextureCompression(osg::Texture* texture) const;

  void setEnablePointSpriteLights(bool enable)
  { _pointSpriteLights = enable; }
  bool getEnablePointSpriteLights() const
  {
      return _pointSpriteLights;
  }
  bool getEnablePointSpriteLights(unsigned contextId) const
  {
    if (!_pointSpriteLights)
      return false;
    return getHavePointSprites(contextId);
  }

  void setEnableDistanceAttenuationLights(bool enable)
  { _distanceAttenuationLights = enable; }
  bool getEnableDistanceAttenuationLights(unsigned contextId) const
  {
    if (!_distanceAttenuationLights)
      return false;
    return getHavePointParameters(contextId);
  }

  void setEnableShaderLights(bool enable)
  { _shaderLights = enable; }
  bool getEnableShaderLights(unsigned contextId) const
  {
    if (!_shaderLights)
      return false;
    return getHaveShaderPrograms(contextId);
  }
  
  void setTextureFilter(int max) 
  { _textureFilter = max; }
  int getTextureFilter() const
  { return _textureFilter; }

protected:
  bool getHavePointSprites(unsigned contextId) const;
  bool getHaveFragmentPrograms(unsigned contextId) const;
  bool getHaveVertexPrograms(unsigned contextId) const;
  bool getHaveShaderPrograms(unsigned contextId) const;
  bool getHavePointParameters(unsigned contextId) const;

private:
  SGSceneFeatures();
  SGSceneFeatures(const SGSceneFeatures&);
  SGSceneFeatures& operator=(const SGSceneFeatures&);

  TextureCompression _textureCompression;
  bool _shaderLights;
  bool _pointSpriteLights;
  bool _distanceAttenuationLights;
  int  _textureFilter;
};

#endif
