// Copyright (C) 2007 Tim Moore
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include <OpenThreads/Mutex>
#include <osg/ref_ptr>
#include <osg/Array>
#include <map>
#include <mutex>

namespace osg
{
class AlphaFunc;
class BlendFunc;
class CullFace;
class Depth;
class ShadeModel;
class Texture2D;
class Texture3D;
class TexEnv;
}

#include <simgear/scene/util/OsgSingleton.hxx>

// Return read-only instances of common OSG state attributes.
namespace simgear
{
class StateAttributeFactory : public ReferencedSingleton<StateAttributeFactory> {
public:
    virtual ~StateAttributeFactory();

    // alpha source, 1 - alpha destination
    osg::BlendFunc* getStandardBlendFunc() { return _standardBlendFunc.get(); }
    // White color
    osg::Vec4Array* getWhiteColor() { return _white.get(); }
    // White, repeating texture
    osg::Texture2D* getWhiteTexture() { return _whiteTexture.get(); }
    // A white, completely transparent texture
    osg::Texture2D* getTransparentTexture() { return _transparentTexture.get(); }
    // Null normalmap texture vec3(0.5, 0.5, 1.0)
    osg::Texture2D* getNullNormalmapTexture() { return _nullNormalmapTexture.get(); }
    // cull front and back facing polygons
    osg::CullFace* getCullFaceFront() { return _cullFaceFront.get(); }
    osg::CullFace* getCullFaceBack() { return _cullFaceBack.get(); }
    // Standard depth
    osg::Depth* getStandardDepth() { return _standardDepth.get(); }
    // Standard depth with writes disabled
    osg::Depth* getStandardDepthWritesDisabled() { return _standardDepthWritesDisabled.get(); }
    osg::Texture3D* getNoiseTexture(int size);

    StateAttributeFactory();    
protected:
    osg::ref_ptr<osg::BlendFunc> _standardBlendFunc;
    osg::ref_ptr<osg::Vec4Array> _white;
    osg::ref_ptr<osg::Texture2D> _whiteTexture;
    osg::ref_ptr<osg::Texture2D> _transparentTexture;
    osg::ref_ptr<osg::Texture2D> _nullNormalmapTexture;
    osg::ref_ptr<osg::CullFace> _cullFaceFront;
    osg::ref_ptr<osg::CullFace> _cullFaceBack;
    osg::ref_ptr<osg::Depth> _standardDepth;
    osg::ref_ptr<osg::Depth> _standardDepthWritesDisabled;

    typedef std::map<int, osg::ref_ptr<osg::Texture3D>> NoiseMap;
    NoiseMap _noises;
    inline static std::mutex _noise_mutex; // Protects the NoiseMap _noises for mult-threaded access
};

} // namespace simgear
