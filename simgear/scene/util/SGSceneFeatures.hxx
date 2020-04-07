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
#include <string>
#include <simgear/misc/sg_path.hxx>

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
    int getMaxTextureSize() const { return _MaxTextureSize; }
    void setMaxTextureSize(const int maxTextureSize) { _MaxTextureSize = maxTextureSize; }

    SGPath getTextureCompressionPath() const { return _TextureCompressionPath; }
    void setTextureCompressionPath(const SGPath path) { _TextureCompressionPath = path; }

    bool getTextureCacheActive() const { return _TextureCacheActive; }
    void setTextureCacheActive(const bool val) { _TextureCacheActive = val; }

    bool getTextureCacheCompressionActive() const { return _TextureCacheCompressionActive; }
    void setTextureCacheCompressionActive(const bool val) { _TextureCacheCompressionActive = val; }

    bool getTextureCacheCompressionActiveTransparent() const { return _TextureCacheCompressionActiveTransparent; }
    void setTextureCacheCompressionActiveTransparent(const bool val) { _TextureCacheCompressionActiveTransparent = val; }

    void setTextureCompression(TextureCompression textureCompression) { _textureCompression = textureCompression; }
    TextureCompression getTextureCompression() const { return _textureCompression; }

    // modify the texture compression on the texture parameter
    void applyTextureCompression(osg::Texture* texture) const;

    void setEnablePointSpriteLights(bool enable)
    {
        _pointSpriteLights = enable;
    }
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

    void setEnableTriangleDirectionalLights(bool enable)
    {
        _triangleDirectionalLights = enable;
    }
    bool getEnableTriangleDirectionalLights() const
    {
        return _triangleDirectionalLights;
    }
    bool getEnableTriangleDirectionalLights(unsigned contextId) const
    {
        if (!_triangleDirectionalLights)
            return false;
        return getHaveTriangleDirectionals(contextId);
    }

    void setEnableDistanceAttenuationLights(bool enable)
    {
        _distanceAttenuationLights = enable;
    }
    bool getEnableDistanceAttenuationLights(unsigned contextId) const
    {
        if (!_distanceAttenuationLights)
            return false;
        return getHavePointParameters(contextId);
    }

    void setEnableShaderLights(bool enable)
    {
        _shaderLights = enable;
    }
    bool getEnableShaderLights(unsigned contextId) const
    {
        if (!_shaderLights)
            return false;
        return getHaveShaderPrograms(contextId);
    }

    void setTextureFilter(int max)
    {
        _textureFilter = max;
    }
    int getTextureFilter() const
    {
        return _textureFilter;
    }

protected:
    bool getHavePointSprites(unsigned contextId) const;
    bool getHaveTriangleDirectionals(unsigned contextId) const;
    bool getHaveFragmentPrograms(unsigned contextId) const;
    bool getHaveVertexPrograms(unsigned contextId) const;
    bool getHaveShaderPrograms(unsigned contextId) const;
    bool getHavePointParameters(unsigned contextId) const;

private:
    SGSceneFeatures();
    SGSceneFeatures(const SGSceneFeatures&);
    SGSceneFeatures& operator=(const SGSceneFeatures&);

    TextureCompression _textureCompression;
    int _MaxTextureSize;
    SGPath _TextureCompressionPath;
    bool _TextureCacheCompressionActive;
    bool _TextureCacheCompressionActiveTransparent;
    bool _TextureCacheActive;
    bool _shaderLights;
    bool _pointSpriteLights;
    bool _triangleDirectionalLights;
    bool _distanceAttenuationLights;
    int  _textureFilter;
};

#endif
