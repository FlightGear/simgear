/* -*-c++-*-
 *
 * Copyright (C) 2007 Tim Moore
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

#ifndef SIMGEAR_STATEATTRIBUTEFACTORY_HXX
#define SIMGEAR_STATEATTRIBUTEFACTORY_HXX 1

#include <OpenThreads/Mutex>
#include <osg/ref_ptr>
#include <osg/Array>
#include <map>

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
class StateAttributeFactory :
        public ReferencedSingleton<StateAttributeFactory> {
public:
    ~StateAttributeFactory();
          
    // Alpha test > .01
    osg::AlphaFunc* getStandardAlphaFunc() { return _standardAlphaFunc.get(); }
    // alpha source, 1 - alpha destination
    osg::BlendFunc* getStandardBlendFunc() { return _standardBlendFunc.get(); }
    // modulate
    osg::TexEnv* getStandardTexEnv() { return _standardTexEnv.get(); }
    osg::ShadeModel* getSmoothShadeModel() { return _smooth.get(); }
    osg::ShadeModel* getFlatShadeModel() { return _flat.get(); }
    // White, repeating texture
    osg::Texture2D* getWhiteTexture() { return _whiteTexture.get(); }
    // White color
    osg::Vec4Array* getWhiteColor() {return _white.get(); }
    // A white, completely transparent texture
    osg::Texture2D* getTransparentTexture()
    {
        return _transparentTexture.get();
    }
    // cull front and back facing polygons
    osg::CullFace* getCullFaceFront() { return _cullFaceFront.get(); }
    osg::CullFace* getCullFaceBack() { return _cullFaceBack.get(); }
    // Standard depth with writes disabled.
    osg::Depth* getDepthWritesDisabled() { return _depthWritesDisabled.get(); }
    osg::Texture3D* getNoiseTexture(int size);
    StateAttributeFactory();    
protected:
    osg::ref_ptr<osg::AlphaFunc> _standardAlphaFunc;
    osg::ref_ptr<osg::ShadeModel> _smooth;
    osg::ref_ptr<osg::ShadeModel> _flat;
    osg::ref_ptr<osg::BlendFunc> _standardBlendFunc;
    osg::ref_ptr<osg::TexEnv> _standardTexEnv;
    osg::ref_ptr<osg::Texture2D> _whiteTexture;
    osg::ref_ptr<osg::Texture2D> _transparentTexture;
    osg::ref_ptr<osg::Vec4Array> _white;
    osg::ref_ptr<osg::CullFace> _cullFaceFront;
    osg::ref_ptr<osg::CullFace> _cullFaceBack;
    osg::ref_ptr<osg::Depth> _depthWritesDisabled;
    typedef std::map<int, osg::ref_ptr<osg::Texture3D> > NoiseMap;
    NoiseMap _noises;
};
}
#endif
