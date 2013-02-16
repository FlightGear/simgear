// OSGUtils.hxx - Useful templates for interfacing to Open Scene Graph
//
// Copyright (C) 2008  Tim Moore timoore@redhat.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA  02110-1301, USA.

#ifndef SIMGEAR_OSGUTILS_HXX
#define SIMGEAR_OSGUTILS_HXX 1

#include <osg/CopyOp>
#include <osg/StateAttribute>
#include <osg/StateSet>

namespace simgear
{
// RefPtrAdapter also appears in OpenSceneGraph's
// osgDB/DatabasePager.cpp. I wrote that code too. -- Tim Moore

// Convert function objects that take pointer args into functions that a
// reference to an osg::ref_ptr. This is quite useful for doing STL
// operations on lists of ref_ptr. This code assumes that a function
// with an argument const Foo* should be composed into a function of
// argument type ref_ptr<Foo>&, not ref_ptr<const Foo>&. Some support
// for that should be added to make this more general.
template <typename U>
struct PointerTraits
{
    typedef class NullType {} PointeeType;
};

template <typename U>
struct PointerTraits<U*>
{
    typedef U PointeeType;
};

template <typename U>
struct PointerTraits<const U*>
{
    typedef U PointeeType;
};

template <typename FuncObj>
class RefPtrAdapter
    : public std::unary_function<const osg::ref_ptr<typename PointerTraits<typename FuncObj::argument_type>::PointeeType>,
                                 typename FuncObj::result_type>
{
public:
    typedef typename PointerTraits<typename FuncObj::argument_type>::PointeeType PointeeType;
    typedef osg::ref_ptr<PointeeType> RefPtrType;
    explicit RefPtrAdapter(const FuncObj& funcObj) : _func(funcObj) {}
    typename FuncObj::result_type operator()(const RefPtrType& refPtr) const
    {
        return _func(refPtr.get());
    }
protected:
        FuncObj _func;
};

template <typename FuncObj>
RefPtrAdapter<FuncObj> refPtrAdapt(const FuncObj& func)
{
    return RefPtrAdapter<FuncObj>(func);
}
}
/** Typesafe wrapper around OSG's object clone function. Something
 * very similar is in current OSG sources.
 */
namespace osg
{
template <typename T> class ref_ptr;
class CopyOp;
}

namespace simgear
{
template <typename T>
T* clone(const T* object, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
{
    return static_cast<T*>(object->clone(copyop));
}

template<typename T>
T* clone_ref(const osg::ref_ptr<T>& object,
             const osg::CopyOp& copyop  = osg::CopyOp::SHALLOW_COPY)
{
    return static_cast<T*>(object->clone(copyop));
}

}

namespace osg
{
class AlphaFunc;
class BlendColor;
class BlendEquation;
class BlendFunc;
class ClampColor;
class ColorMask;
class ColorMatrix;
class CullFace;
class Depth;
class Fog;
class FragmentProgram;
class FrontFace;
class LightModel;
class LineStipple;
class LineWidth;
class LogicOp;
class Material;
class Multisample;
class Point;
class PointSprite;
class PolygonMode;
class PolygonOffset;
class PolygonStipple;
class Program;
class Scissor;
class ShadeModel;
class Stencil;
class StencilTwoSided;
class TexEnv;
class TexEnvCombine;
class TexEnvFilter;
class TexGen;
class TexMat;
class Texture1D;
class Texture2D;
class Texture2DArray;
class Texture3D;
class TextureCubeMap;
class TextureRectangle;
class VertexProgram;
class Viewport;
}

namespace simgear
{
namespace osgutils
{
using namespace osg;

template<StateAttribute::Type T>
struct TypeHolder
{
    static const StateAttribute::Type type = T;
};

template<typename AT> struct AttributeType;
template<typename AT> struct TexAttributeType;

template<>
struct AttributeType<AlphaFunc>
    : public TypeHolder<StateAttribute::ALPHAFUNC>
{};

template<>
struct AttributeType<BlendColor>
    : public TypeHolder<StateAttribute::BLENDCOLOR>
{};

template<>
struct AttributeType<BlendEquation>
    : public TypeHolder<StateAttribute::BLENDEQUATION>
{};

template<>
struct AttributeType<BlendFunc>
    : public TypeHolder<StateAttribute::BLENDFUNC>
{};

template<>
struct AttributeType<ClampColor>
    : public TypeHolder<StateAttribute::CLAMPCOLOR>
{};

template<>
struct AttributeType<ColorMask>
    : public TypeHolder<StateAttribute::COLORMASK>
{};

template<>
struct AttributeType<ColorMatrix>
    : public TypeHolder<StateAttribute::COLORMATRIX>
{};

template<>
struct AttributeType<CullFace>
    : public TypeHolder<StateAttribute::CULLFACE>
{};


template<>
struct AttributeType<osg::Depth> // Conflicts with Xlib
    : public TypeHolder<StateAttribute::DEPTH>
{};

template<>
struct AttributeType<Fog>
    : public TypeHolder<StateAttribute::FOG>
{};

template<>
struct AttributeType<FragmentProgram>
    : public TypeHolder<StateAttribute::FRAGMENTPROGRAM>
{};

template<>
struct AttributeType<FrontFace>
    : public TypeHolder<StateAttribute::FRONTFACE>
{};

template<>
struct AttributeType<LightModel>
    : public TypeHolder<StateAttribute::LIGHTMODEL>
{};

template<>
struct AttributeType<LineStipple>
    : public TypeHolder<StateAttribute::LINESTIPPLE>
{};

template<>
struct AttributeType<LineWidth>
    : public TypeHolder<StateAttribute::LINEWIDTH>
{};

template<>
struct AttributeType<LogicOp>
    : public TypeHolder<StateAttribute::LOGICOP>
{};

template<>
struct AttributeType<Material>
    : public TypeHolder<StateAttribute::MATERIAL>
{};

template<>
struct AttributeType<Multisample>
    : public TypeHolder<StateAttribute::MULTISAMPLE>
{};

template<>
struct AttributeType<Point>
    : public TypeHolder<StateAttribute::POINT>
{};

template<>
struct TexAttributeType<PointSprite>
    : public TypeHolder<StateAttribute::POINTSPRITE>
{};

template<>
struct AttributeType<PolygonMode>
    : public TypeHolder<StateAttribute::POLYGONMODE>
{};

template<>
struct AttributeType<PolygonOffset>
    : public TypeHolder<StateAttribute::POLYGONOFFSET>
{};

template<>
struct AttributeType<PolygonStipple>
    : public TypeHolder<StateAttribute::POLYGONSTIPPLE>
{};

template<>
struct AttributeType<Program>
    : public TypeHolder<StateAttribute::PROGRAM>
{};

template<>
struct AttributeType<Scissor>
    : public TypeHolder<StateAttribute::SCISSOR>
{};

template<>
struct AttributeType<ShadeModel>
    : public TypeHolder<StateAttribute::SHADEMODEL>
{};

template<>
struct AttributeType<Stencil>
    : public TypeHolder<StateAttribute::STENCIL>
{};

template<>
struct AttributeType<StencilTwoSided>
    : public TypeHolder<StateAttribute::STENCIL>
{};

// TexEnvCombine is not a subclass of TexEnv, so we can't do a
// typesafe access of the attribute.
#if 0
template<>
struct TexAttributeType<TexEnv>
    : public TypeHolder<StateAttribute::TEXENV>
{};

template<>
struct TexAttributeType<TexEnvCombine>
    : public TypeHolder<StateAttribute::TEXENV>
{};
#endif

template<>
struct TexAttributeType<TexEnvFilter>
    : public TypeHolder<StateAttribute::TEXENVFILTER>
{};

template<>
struct TexAttributeType<TexGen>
    : public TypeHolder<StateAttribute::TEXGEN>
{};

template<>
struct TexAttributeType<TexMat>
    : public TypeHolder<StateAttribute::TEXMAT>
{};

template<>
struct TexAttributeType<Texture>
    : public TypeHolder<StateAttribute::TEXTURE>
{};

template<>
struct AttributeType<VertexProgram>
    : public TypeHolder<StateAttribute::VERTEXPROGRAM>
{};

template<>
struct AttributeType<Viewport>
    : public TypeHolder<StateAttribute::VIEWPORT>
{};
} // namespace osgutils

template<typename AT>
inline AT* getStateAttribute(osg::StateSet* ss)
{
    return static_cast<AT*>(ss->getAttribute(osgutils::AttributeType<AT>::type));
}

template<typename AT>
inline const AT* getStateAttribute(const osg::StateSet* ss)
{
    return static_cast<const AT*>(ss->getAttribute(osgutils::AttributeType<AT>::type));
}

template<typename AT>
inline AT* getStateAttribute(unsigned int unit, osg::StateSet* ss)
{
    return static_cast<AT*>(ss->getTextureAttribute(unit, osgutils::TexAttributeType<AT>
                                                    ::type));
}

template<typename AT>
inline const AT* getStateAttribute(unsigned int unit, const osg::StateSet* ss)
{
    return static_cast<const AT*>(ss->getTextureAttribute(unit,
                                                          osgutils::TexAttributeType<AT>
                                                          ::type));
}
} // namespace simgear

#endif
