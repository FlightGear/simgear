// Copyright (C) 2008 - 2010  Tim Moore timoore33@gmail.com
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "Effect.hxx"
#include "EffectBuilder.hxx"
#include "EffectGeode.hxx"
#include "Technique.hxx"
#include "Pass.hxx"
#include "TextureBuilder.hxx"
#include "parseBlendFunc.hxx"

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <queue>
#include <utility>
#include <unordered_map>
#include <mutex>

#include <boost/functional/hash.hpp>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/Drawable>
#include <osg/Material>
#include <osg/Math>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/Point>
#include <osg/Program>
#include <osg/Referenced>
#include <osg/RenderInfo>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osg/Stencil>
#include <osg/TexEnv>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureRectangle>
#include <osg/Uniform>
#include <osg/Vec4d>
#include <osgUtil/CullVisitor>
#include <osgDB/FileUtils>
#include <osgDB/Input>
#include <osgDB/ParameterOutput>
#include <osgDB/ReadFile>
#include <osgDB/Registry>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGProgram.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/structure/OSGUtils.hxx>
#include <simgear/structure/SGExpression.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/io/iostreams/sgstream.hxx>

namespace simgear
{
using namespace std;
using namespace osg;
using namespace osgUtil;

using namespace effect;

const char* UniformFactoryImpl::vec3Names[] = {"x", "y", "z"};
const char* UniformFactoryImpl::vec4Names[] = {"x", "y", "z", "w"};

void UniformFactoryImpl::reset()
{
  uniformCache.clear();
}

// work around the fact osg::Shader::loadShaderFromSourceFile does not
// respect UTF8 paths, even when OSG_USE_UTF8_FILENAME is set :(
bool loadShaderFromUTF8File(osg::Shader* shader, const std::string& fileName)
{
    sg_ifstream inStream(SGPath::fromUtf8(fileName), std::ios::in | std::ios::binary);
    if (!inStream.is_open())
        return false;

    shader->setFileName(fileName);
    shader->setShaderSource(inStream.read_all());
    return true;
}

ref_ptr<Uniform> UniformFactoryImpl::getUniform( Effect * effect,
                                 const string & name,
                                 Uniform::Type uniformType,
                                 SGConstPropertyNode_ptr valProp,
                                 const SGReaderWriterOptions* options )
{
	std::lock_guard<std::mutex> scopeLock(_mutex);
	std::string val = "0";

	if (valProp->nChildren() == 0) {
		// Completely static value
		val = valProp->getStringValue();
	} else {
		// Value references <parameters> section of Effect
		const SGPropertyNode* prop = getEffectPropertyNode(effect, valProp);

		if (prop) {
			if (prop->nChildren() == 0) {
				// Static value in parameters section
				val = prop->getStringValue();
			} else {
				// Dynamic property value in parameters section
				val = getGlobalProperty(prop, options);
			}
		} else {
			SG_LOG(SG_GL,SG_DEBUG,"Invalid parameter " << valProp->getName() << " for uniform " << name << " in Effect ");
		}
	}

	UniformCacheKey key = std::make_tuple(name, uniformType, val, effect->getName());
	ref_ptr<Uniform> uniform = uniformCache[key];

    if (uniform.valid()) {
    	// We've got a hit to cache - simply return it
    	return uniform;
    }

    SG_LOG(SG_GL,SG_DEBUG,"new uniform " << name << " value " << uniformCache.size());
    uniformCache[key] = uniform = new Uniform;

    uniform->setName(name);
    uniform->setType(uniformType);
    switch (uniformType) {
    case Uniform::BOOL:
    	initFromParameters(effect, valProp, uniform.get(),
                           static_cast<bool (Uniform::*)(bool)>(&Uniform::set),
                           options);
        break;
    case Uniform::FLOAT:
    	initFromParameters(effect, valProp, uniform.get(),
                           static_cast<bool (Uniform::*)(float)>(&Uniform::set),
                           options);
        break;
    case Uniform::FLOAT_VEC3:
    	initFromParameters(effect, valProp, uniform.get(),
                           static_cast<bool (Uniform::*)(const Vec3&)>(&Uniform::set),
                           vec3Names, options);
        break;
    case Uniform::FLOAT_VEC4:
    	initFromParameters(effect, valProp, uniform.get(),
                           static_cast<bool (Uniform::*)(const Vec4&)>(&Uniform::set),
                           vec4Names, options);
        break;
    case Uniform::INT:
    case Uniform::SAMPLER_1D:
    case Uniform::SAMPLER_2D:
    case Uniform::SAMPLER_3D:
    case Uniform::SAMPLER_1D_SHADOW:
    case Uniform::SAMPLER_2D_SHADOW:
    case Uniform::SAMPLER_CUBE:
    	initFromParameters(effect, valProp, uniform.get(),
                           static_cast<bool (Uniform::*)(int)>(&Uniform::set),
                           options);
        break;
    default: // avoid compiler warning
    	SG_LOG(SG_ALL,SG_ALERT,"UNKNOWN Uniform type '" << uniformType << "'");
        break;
    }

    return uniform;
}

void UniformFactoryImpl::addListener(DeferredPropertyListener* listener)
{
    if (listener != 0) {
    	// Uniform requires a property listener. Add it to the list to be
    	// created when the main thread gets to it.
    	deferredListenerList.push(listener);
    }
}

void UniformFactoryImpl::updateListeners( SGPropertyNode* propRoot )
{
	std::lock_guard<std::mutex> scopeLock(_mutex);

	if (deferredListenerList.empty()) return;

	SG_LOG(SG_GL,SG_DEBUG,"Adding " << deferredListenerList.size() << " listeners for effects.");

	// Instantiate all queued listeners
	while (! deferredListenerList.empty()) {
		DeferredPropertyListener* listener = deferredListenerList.front();
		listener->activate(propRoot);
		deferredListenerList.pop();
	}
}


Effect::Effect()
    : _cache(0), _isRealized(false)
{
}

Effect::Effect(const Effect& rhs, const CopyOp& copyop)
    : osg::Object(rhs, copyop), root(rhs.root), parametersProp(rhs.parametersProp), _cache(0),
      _isRealized(rhs._isRealized),
      _effectFilePath(rhs._effectFilePath)
{
    typedef vector<ref_ptr<Technique> > TechniqueList;
    for (TechniqueList::const_iterator itr = rhs.techniques.begin(),
             end = rhs.techniques.end();
         itr != end;
         ++itr)
        techniques.push_back(static_cast<Technique*>(copyop(itr->get())));

    generator = rhs.generator;
    _name = rhs._name;
    _name += " clone";
}

// Assume that the last technique is always valid.
StateSet* Effect::getDefaultStateSet()
{
    Technique* tniq = techniques.back().get();
    if (!tniq)
        return 0;
    Pass* pass = tniq->passes.front().get();
    return pass;
}

int Effect::getGenerator(Effect::Generator what) const
{
    std::map<Generator,int>::const_iterator it = generator.find(what);
    if(it == generator.end()) return -1;
    else return it->second;
}

// There should always be a valid technique in an effect.

Technique* Effect::chooseTechnique(RenderInfo* info, const std::string &scheme)
{
    for (auto& technique : techniques) {
        if (technique->valid(info) == Technique::VALID &&
            technique->getScheme() == scheme)
            return technique.get();
    }
    return 0;
}

void Effect::resizeGLObjectBuffers(unsigned int maxSize)
{
    for (const auto& technique : techniques) {
        technique->resizeGLObjectBuffers(maxSize);
    }
}

void Effect::releaseGLObjects(osg::State* state) const
{
    for (const auto& technique : techniques) {
        technique->releaseGLObjects(state);
    }
}

Effect::~Effect()
{
    delete _cache;
}

void buildPass(Effect* effect, Technique* tniq, const SGPropertyNode* prop,
               const SGReaderWriterOptions* options)
{
    simgear::ErrorReportContext ec("effect-pass", prop->getPath());

    Pass* pass = new Pass;
    tniq->passes.push_back(pass);
    for (int i = 0; i < prop->nChildren(); ++i) {
        const SGPropertyNode* attrProp = prop->getChild(i);
        PassAttributeBuilder* builder
            = PassAttributeBuilder::find(attrProp->getNameString());
        if (builder)
            builder->buildAttribute(effect, pass, attrProp, options);
        else
            SG_LOG(SG_INPUT, SG_ALERT,
                   "skipping unknown pass attribute " << attrProp->getName());
    }
}

osg::Vec4f getColor(const SGPropertyNode* prop)
{
    if (prop->nChildren() == 0) {
        if (prop->getType() == props::VEC4D) {
            return osg::Vec4f(toOsg(prop->getValue<SGVec4d>()));
        } else if (prop->getType() == props::VEC3D) {
            return osg::Vec4f(toOsg(prop->getValue<SGVec3d>()), 1.0f);
        } else {
            SG_LOG(SG_INPUT, SG_ALERT,
                   "invalid color property " << prop->getName() << " "
                   << prop->getStringValue());
            return osg::Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
        }
    } else {
        osg::Vec4f result;
        static const char* colors[] = {"r", "g", "b"};
        for (int i = 0; i < 3; ++i) {
            const SGPropertyNode* componentProp = prop->getChild(colors[i]);
            result[i] = componentProp ? componentProp->getValue<float>() : 0.0f;
        }
        const SGPropertyNode* alphaProp = prop->getChild("a");
        result[3] = alphaProp ? alphaProp->getValue<float>() : 1.0f;
        return result;
    }
}

struct LightingBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options);
};

void LightingBuilder::buildAttribute(Effect* effect, Pass* pass,
                                     const SGPropertyNode* prop,
                                     const SGReaderWriterOptions* options)
{
    const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
    if (!realProp)
        return;
    pass->setMode(GL_LIGHTING, (realProp->getValue<bool>() ? StateAttribute::ON
                                : StateAttribute::OFF));
}

InstallAttributeBuilder<LightingBuilder> installLighting("lighting");

struct ShadeModelBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;
        StateAttributeFactory *attrFact = StateAttributeFactory::instance();
        string propVal = realProp->getStringValue();
        if (propVal == "flat")
            pass->setAttribute(attrFact->getFlatShadeModel());
        else if (propVal == "smooth")
            pass->setAttribute(attrFact->getSmoothShadeModel());
        else
            SG_LOG(SG_INPUT, SG_ALERT,
                   "invalid shade model property " << propVal);
    }
};

InstallAttributeBuilder<ShadeModelBuilder> installShadeModel("shade-model");

struct CullFaceBuilder : PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp) {
            pass->setMode(GL_CULL_FACE, StateAttribute::OFF);
            return;
        }
        StateAttributeFactory *attrFact = StateAttributeFactory::instance();
        string propVal = realProp->getStringValue();
        if (propVal == "front")
            pass->setAttributeAndModes(attrFact->getCullFaceFront());
        else if (propVal == "back")
            pass->setAttributeAndModes(attrFact->getCullFaceBack());
        else if (propVal == "front-back")
            pass->setAttributeAndModes(new CullFace(CullFace::FRONT_AND_BACK));
        else if (propVal == "off")
            pass->setMode(GL_CULL_FACE, StateAttribute::OFF);
        else
            SG_LOG(SG_INPUT, SG_ALERT,
                   "invalid cull face property " << propVal);
    }
};

InstallAttributeBuilder<CullFaceBuilder> installCullFace("cull-face");

struct ColorMaskBuilder : PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;

        ColorMask *mask = new ColorMask;
        Vec4 m = getColor(realProp);
        mask->setMask(m.r() > 0.0, m.g() > 0.0, m.b() > 0.0, m.a() > 0.0);
        pass->setAttributeAndModes(mask);
    }
};

InstallAttributeBuilder<ColorMaskBuilder> installColorMask("color-mask");

EffectNameValue<StateSet::RenderingHint> renderingHintInit[] =
{
    { "default", StateSet::DEFAULT_BIN },
    { "opaque", StateSet::OPAQUE_BIN },
    { "transparent", StateSet::TRANSPARENT_BIN }
};

EffectPropertyMap<StateSet::RenderingHint> renderingHints(renderingHintInit);

struct HintBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;
        StateSet::RenderingHint renderingHint = StateSet::DEFAULT_BIN;
        findAttr(renderingHints, realProp, renderingHint);
        pass->setRenderingHint(renderingHint);
    }
};

InstallAttributeBuilder<HintBuilder> installHint("rendering-hint");

struct RenderBinBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        if (!isAttributeActive(effect, prop))
            return;
        const SGPropertyNode* binProp = prop->getChild("bin-number");
        binProp = getEffectPropertyNode(effect, binProp);
        const SGPropertyNode* nameProp = prop->getChild("bin-name");
        nameProp = getEffectPropertyNode(effect, nameProp);
        if (binProp && nameProp) {
            pass->setRenderBinDetails(binProp->getIntValue(),
                                      nameProp->getStringValue());
        } else {
            if (!binProp)
                SG_LOG(SG_INPUT, SG_ALERT,
                       "No render bin number specified in render bin section");
            if (!nameProp)
                SG_LOG(SG_INPUT, SG_ALERT,
                       "No render bin name specified in render bin section");
        }
    }
};

InstallAttributeBuilder<RenderBinBuilder> installRenderBin("render-bin");

struct MaterialBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options);
};

EffectNameValue<Material::ColorMode> colorModeInit[] =
{
    { "ambient", Material::AMBIENT },
    { "ambient-and-diffuse", Material::AMBIENT_AND_DIFFUSE },
    { "diffuse", Material::DIFFUSE },
    { "emissive", Material::EMISSION },
    { "specular", Material::SPECULAR },
    { "off", Material::OFF }
};
EffectPropertyMap<Material::ColorMode> colorModes(colorModeInit);

void MaterialBuilder::buildAttribute(Effect* effect, Pass* pass,
                                     const SGPropertyNode* prop,
                                     const SGReaderWriterOptions* options)
{
    if (!isAttributeActive(effect, prop))
        return;
    Material* mat = new Material;
    const SGPropertyNode* color = 0;
    if ((color = getEffectPropertyChild(effect, prop, "ambient")))
        mat->setAmbient(Material::FRONT_AND_BACK, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "ambient-front")))
        mat->setAmbient(Material::FRONT, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "ambient-back")))
        mat->setAmbient(Material::BACK, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "diffuse")))
        mat->setDiffuse(Material::FRONT_AND_BACK, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "diffuse-front")))
        mat->setDiffuse(Material::FRONT, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "diffuse-back")))
        mat->setDiffuse(Material::BACK, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "specular")))
        mat->setSpecular(Material::FRONT_AND_BACK, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "specular-front")))
        mat->setSpecular(Material::FRONT, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "specular-back")))
        mat->setSpecular(Material::BACK, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "emissive")))
        mat->setEmission(Material::FRONT_AND_BACK, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "emissive-front")))
        mat->setEmission(Material::FRONT, getColor(color));
    if ((color = getEffectPropertyChild(effect, prop, "emissive-back")))
        mat->setEmission(Material::BACK, getColor(color));
    const SGPropertyNode* shininess = 0;
    mat->setShininess(Material::FRONT_AND_BACK, 0.0f);
    if ((shininess = getEffectPropertyChild(effect, prop, "shininess")))
        mat->setShininess(Material::FRONT_AND_BACK, shininess->getFloatValue());
    if ((shininess = getEffectPropertyChild(effect, prop, "shininess-front")))
        mat->setShininess(Material::FRONT, shininess->getFloatValue());
    if ((shininess = getEffectPropertyChild(effect, prop, "shininess-back")))
        mat->setShininess(Material::BACK, shininess->getFloatValue());
    Material::ColorMode colorMode = Material::OFF;
    findAttr(colorModes, getEffectPropertyChild(effect, prop, "color-mode"),
             colorMode);
    mat->setColorMode(colorMode);
    pass->setAttribute(mat);
}

InstallAttributeBuilder<MaterialBuilder> installMaterial("material");

EffectNameValue<BlendFunc::BlendFuncMode> blendFuncModesInit[] =
{
    {"dst-alpha", BlendFunc::DST_ALPHA},
    {"dst-color", BlendFunc::DST_COLOR},
    {"one", BlendFunc::ONE},
    {"one-minus-dst-alpha", BlendFunc::ONE_MINUS_DST_ALPHA},
    {"one-minus-dst-color", BlendFunc::ONE_MINUS_DST_COLOR},
    {"one-minus-src-alpha", BlendFunc::ONE_MINUS_SRC_ALPHA},
    {"one-minus-src-color", BlendFunc::ONE_MINUS_SRC_COLOR},
    {"src-alpha", BlendFunc::SRC_ALPHA},
    {"src-alpha-saturate", BlendFunc::SRC_ALPHA_SATURATE},
    {"src-color", BlendFunc::SRC_COLOR},
    {"constant-color", BlendFunc::CONSTANT_COLOR},
    {"one-minus-constant-color", BlendFunc::ONE_MINUS_CONSTANT_COLOR},
    {"constant-alpha", BlendFunc::CONSTANT_ALPHA},
    {"one-minus-constant-alpha", BlendFunc::ONE_MINUS_CONSTANT_ALPHA},
    {"zero", BlendFunc::ZERO}
};
EffectPropertyMap<BlendFunc::BlendFuncMode> blendFuncModes(blendFuncModesInit);

struct BlendBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        if (!isAttributeActive(effect, prop))
            return;
        // XXX Compatibility with early <blend> syntax; should go away
        // before a release
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;
        if (realProp->nChildren() == 0) {
            pass->setMode(GL_BLEND, (realProp->getBoolValue()
                                     ? StateAttribute::ON
                                     : StateAttribute::OFF));
            return;
        }

        const SGPropertyNode* pmode = getEffectPropertyChild(effect, prop,
                                                             "mode");
        // XXX When dynamic parameters are supported, this code should
        // create the blend function even if the mode is off.
        if (pmode && !pmode->getValue<bool>()) {
            pass->setMode(GL_BLEND, StateAttribute::OFF);
            return;
        }

        parseBlendFunc(
          pass,
          getEffectPropertyChild(effect, prop, "source"),
          getEffectPropertyChild(effect, prop, "destination"),
          getEffectPropertyChild(effect, prop, "source-rgb"),
          getEffectPropertyChild(effect, prop, "destination-rgb"),
          getEffectPropertyChild(effect, prop, "source-alpha"),
          getEffectPropertyChild(effect, prop, "destination-alpha")
        );
    }
};

InstallAttributeBuilder<BlendBuilder> installBlend("blend");


EffectNameValue<Stencil::Function> stencilFunctionInit[] =
{
    {"never", Stencil::NEVER },
    {"less", Stencil::LESS},
    {"equal", Stencil::EQUAL},
    {"less-or-equal", Stencil::LEQUAL},
    {"greater", Stencil::GREATER},
    {"not-equal", Stencil::NOTEQUAL},
    {"greater-or-equal", Stencil::GEQUAL},
    {"always", Stencil::ALWAYS}
};

EffectPropertyMap<Stencil::Function> stencilFunction(stencilFunctionInit);

EffectNameValue<Stencil::Operation> stencilOperationInit[] =
{
    {"keep", Stencil::KEEP},
    {"zero", Stencil::ZERO},
    {"replace", Stencil::REPLACE},
    {"increase", Stencil::INCR},
    {"decrease", Stencil::DECR},
    {"invert", Stencil::INVERT},
    {"increase-wrap", Stencil::INCR_WRAP},
    {"decrease-wrap", Stencil::DECR_WRAP}
};

EffectPropertyMap<Stencil::Operation> stencilOperation(stencilOperationInit);

struct StencilBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        if (!isAttributeActive(effect, prop))
            return;

        const SGPropertyNode* pmode = getEffectPropertyChild(effect, prop,
                                                             "mode");
        if (pmode && !pmode->getValue<bool>()) {
            pass->setMode(GL_STENCIL, StateAttribute::OFF);
            return;
        }
        const SGPropertyNode* pfunction
            = getEffectPropertyChild(effect, prop, "function");
        const SGPropertyNode* pvalue
            = getEffectPropertyChild(effect, prop, "value");
        const SGPropertyNode* pmask
            = getEffectPropertyChild(effect, prop, "mask");
        const SGPropertyNode* psfail
            = getEffectPropertyChild(effect, prop, "stencil-fail");
        const SGPropertyNode* pzfail
            = getEffectPropertyChild(effect, prop, "z-fail");
        const SGPropertyNode* ppass
            = getEffectPropertyChild(effect, prop, "pass");

        Stencil::Function func = Stencil::ALWAYS;  // Always pass
        int ref = 0;
        unsigned int mask = ~0u;  // All bits on
        Stencil::Operation sfailop = Stencil::KEEP;  // Keep the old values as default
        Stencil::Operation zfailop = Stencil::KEEP;
        Stencil::Operation passop = Stencil::KEEP;

        ref_ptr<Stencil> stencilFunc = new Stencil;

        if (pfunction)
            findAttr(stencilFunction, pfunction, func);
        if (pvalue)
            ref = pvalue->getIntValue();
        if (pmask)
            mask = pmask->getIntValue();

        if (psfail)
            findAttr(stencilOperation, psfail, sfailop);
        if (pzfail)
            findAttr(stencilOperation, pzfail, zfailop);
        if (ppass)
            findAttr(stencilOperation, ppass, passop);

        // Set the stencil operation
        stencilFunc->setFunction(func, ref, mask);

        // Set the operation, s-fail, s-pass/z-fail, s-pass/z-pass
        stencilFunc->setOperation(sfailop, zfailop, passop);

        // Add the operation to pass
        pass->setAttributeAndModes(stencilFunc.get());
    }
};

InstallAttributeBuilder<StencilBuilder> installStencil("stencil");

struct AlphaToCoverageBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options);
};

#ifndef GL_SAMPLE_ALPHA_TO_COVERAGE_ARB
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB 0x809E
#endif

void AlphaToCoverageBuilder::buildAttribute(Effect* effect, Pass* pass,
                                     const SGPropertyNode* prop,
                                     const SGReaderWriterOptions* options)
{
    const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
    if (!realProp)
        return;
    pass->setMode(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB, (realProp->getValue<bool>() ?
                                    StateAttribute::ON : StateAttribute::OFF));
}

InstallAttributeBuilder<AlphaToCoverageBuilder> installAlphaToCoverage("alpha-to-coverage");

EffectNameValue<AlphaFunc::ComparisonFunction> alphaComparisonInit[] =
{
    {"never", AlphaFunc::NEVER},
    {"less", AlphaFunc::LESS},
    {"equal", AlphaFunc::EQUAL},
    {"lequal", AlphaFunc::LEQUAL},
    {"greater", AlphaFunc::GREATER},
    {"notequal", AlphaFunc::NOTEQUAL},
    {"gequal", AlphaFunc::GEQUAL},
    {"always", AlphaFunc::ALWAYS}
};
EffectPropertyMap<AlphaFunc::ComparisonFunction>
alphaComparison(alphaComparisonInit);

struct AlphaTestBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        if (!isAttributeActive(effect, prop))
            return;
        // XXX Compatibility with early <alpha-test> syntax; should go away
        // before a release
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;
        if (realProp->nChildren() == 0) {
            pass->setMode(GL_ALPHA_TEST, (realProp->getBoolValue()
                                     ? StateAttribute::ON
                                     : StateAttribute::OFF));
            return;
        }

        const SGPropertyNode* pmode = getEffectPropertyChild(effect, prop,
                                                             "mode");
        // XXX When dynamic parameters are supported, this code should
        // create the blend function even if the mode is off.
        if (pmode && !pmode->getValue<bool>()) {
            pass->setMode(GL_ALPHA_TEST, StateAttribute::OFF);
            return;
        }
        const SGPropertyNode* pComp = getEffectPropertyChild(effect, prop,
                                                             "comparison");
        const SGPropertyNode* pRef = getEffectPropertyChild(effect, prop,
                                                             "reference");

        AlphaFunc::ComparisonFunction func = AlphaFunc::ALWAYS;
        float refValue = 1.0f;
        if (pComp)
            findAttr(alphaComparison, pComp, func);
        if (pRef)
            refValue = pRef->getValue<float>();
        if (func == AlphaFunc::GREATER && osg::equivalent(refValue, 1.0f)) {
            pass->setAttributeAndModes(StateAttributeFactory::instance()
                                       ->getStandardAlphaFunc());
        } else {
            AlphaFunc* alphaFunc = new AlphaFunc;
            alphaFunc->setFunction(func);
            alphaFunc->setReferenceValue(refValue);
            pass->setAttributeAndModes(alphaFunc);
        }
    }
};

InstallAttributeBuilder<AlphaTestBuilder> installAlphaTest("alpha-test");

InstallAttributeBuilder<TextureUnitBuilder> textureUnitBuilder("texture-unit");

// Shader key, used both for shaders with relative and absolute names
typedef pair<string, int> ShaderKey;

inline ShaderKey makeShaderKey(SGPropertyNode_ptr& ptr, int shaderType)
{
    return ShaderKey(ptr->getStringValue(), shaderType);
}

struct ProgramKey
{
    typedef pair<string, int> AttribKey;
    osgDB::FilePathList paths;
    vector<ShaderKey> shaders;
    vector<AttribKey> attributes;
    struct EqualTo
    {
        bool operator()(const ProgramKey& lhs, const ProgramKey& rhs) const
        {
            return (lhs.paths.size() == rhs.paths.size()
                    && equal(lhs.paths.begin(), lhs.paths.end(),
                             rhs.paths.begin())
                    && lhs.shaders.size() == rhs.shaders.size()
                    && equal (lhs.shaders.begin(), lhs.shaders.end(),
                              rhs.shaders.begin())
                    && lhs.attributes.size() == rhs.attributes.size()
                    && equal(lhs.attributes.begin(), lhs.attributes.end(),
                             rhs.attributes.begin()));
        }
    };
};

size_t hash_value(const ProgramKey& key)
{
    size_t seed = 0;
    boost::hash_range(seed, key.paths.begin(), key.paths.end());
    boost::hash_range(seed, key.shaders.begin(), key.shaders.end());
    boost::hash_range(seed, key.attributes.begin(), key.attributes.end());
    return seed;
}

// XXX Should these be protected by a mutex? Probably

typedef std::unordered_map<ProgramKey, ref_ptr<Program>,
                           boost::hash<ProgramKey>, ProgramKey::EqualTo>
ProgramMap;
ProgramMap programMap;
ProgramMap resolvedProgramMap;  // map with resolved shader file names

typedef std::unordered_map<ShaderKey, ref_ptr<Shader>, boost::hash<ShaderKey> >
ShaderMap;
ShaderMap shaderMap;

void reload_shaders()
{
    for(ShaderMap::iterator sitr = shaderMap.begin(); sitr != shaderMap.end(); ++sitr)
    {
        Shader *shader = sitr->second.get();
        string fileName = SGModelLib::findDataFile(sitr->first.first);
        if (!fileName.empty()) {
            loadShaderFromUTF8File(shader, fileName);
        } else {
            SG_LOG(SG_INPUT, SG_ALERT, "Could not locate shader: " << fileName);
            simgear::reportFailure(simgear::LoadFailure::NotFound,
                                   simgear::ErrorCode::LoadEffectsShaders,
                                   "Reload: couldn't find shader:" + sitr->first.first);
        }
    }
}

struct ShaderProgramBuilder : PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options);
};


EffectNameValue<GLint> geometryInputTypeInit[] =
{
    {"points", GL_POINTS},
    {"lines", GL_LINES},
    {"lines-adjacency", GL_LINES_ADJACENCY_EXT},
    {"triangles", GL_TRIANGLES},
    {"triangles-adjacency", GL_TRIANGLES_ADJACENCY_EXT},
};
EffectPropertyMap<GLint>
geometryInputType(geometryInputTypeInit);


EffectNameValue<GLint> geometryOutputTypeInit[] =
{
    {"points", GL_POINTS},
    {"line-strip", GL_LINE_STRIP},
    {"triangle-strip", GL_TRIANGLE_STRIP}
};
EffectPropertyMap<GLint>
geometryOutputType(geometryOutputTypeInit);

void ShaderProgramBuilder::buildAttribute(Effect* effect, Pass* pass,
                                          const SGPropertyNode* prop,
                                          const SGReaderWriterOptions*
                                          options)
{
    if (!isAttributeActive(effect, prop))
        return;
    PropertyList pVertShaders = prop->getChildren("vertex-shader");
    PropertyList pGeomShaders = prop->getChildren("geometry-shader");
    PropertyList pFragShaders = prop->getChildren("fragment-shader");
    PropertyList pAttributes = prop->getChildren("attribute");
    ProgramKey prgKey;
    std::back_insert_iterator<vector<ShaderKey> > inserter(prgKey.shaders);
    std::transform(pVertShaders.begin(), pVertShaders.end(), inserter,
                   [] (SGPropertyNode_ptr& ptr) {
                       return makeShaderKey(ptr, Shader::VERTEX); });
    std::transform(pGeomShaders.begin(), pGeomShaders.end(), inserter,
                   [] (SGPropertyNode_ptr& ptr) {
                       return makeShaderKey(ptr, Shader::GEOMETRY); });
    std::transform(pFragShaders.begin(), pFragShaders.end(), inserter,
                   [] (SGPropertyNode_ptr& ptr) {
                       return makeShaderKey(ptr, Shader::FRAGMENT); });
    for (PropertyList::iterator itr = pAttributes.begin(),
             e = pAttributes.end();
         itr != e;
         ++itr) {
        const SGPropertyNode* pName = getEffectPropertyChild(effect, *itr,
                                                             "name");
        const SGPropertyNode* pIndex = getEffectPropertyChild(effect, *itr,
                                                              "index");
        if (!pName || ! pIndex)
            throw BuilderException("malformed attribute property");
        prgKey.attributes
            .push_back(ProgramKey::AttribKey(pName->getStringValue(),
                                             pIndex->getValue<int>()));
    }
    if (options)
        prgKey.paths = options->getDatabasePathList();
    Program* program = 0;
    ProgramMap::iterator pitr = programMap.find(prgKey);
    if (pitr != programMap.end()) {
        program = pitr->second.get();
        pass->setAttributeAndModes(program);
        return;
    }
    // The program wasn't in the map using the load path passed in with
    // the options, but it might have already been loaded using a
    // different load path i.e., its shaders were found in the fg data
    // directory. So, resolve the shaders' file names and look in the
    // resolvedProgramMap for a program using those shaders.
    ProgramKey resolvedKey;
    resolvedKey.attributes = prgKey.attributes;
    for (const auto& shaderKey : prgKey.shaders) {
        // FIXME orig: const string& shaderName = shaderKey.first;
        string shaderName = shaderKey.first;
        Shader::Type stype = (Shader::Type)shaderKey.second;
        const bool compositorEnabled = getPropertyRoot()->getBoolValue("/sim/version/compositor-support", false);
        if (compositorEnabled &&
            shaderName.substr(0, shaderName.find("/")) == "Shaders") {
            shaderName = "Compositor/" + shaderName;
        }
        string fileName = SGModelLib::findDataFile(shaderName, options);
        if (fileName.empty())
        {
            simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::LoadEffectsShaders,
                                   "Couldn't locate shader:" + shaderName, sg_location{shaderName});

            throw BuilderException(string("couldn't find shader ") +
                shaderName);
        }
        resolvedKey.shaders.push_back(ShaderKey(fileName, stype));
    }
    ProgramMap::iterator resitr = resolvedProgramMap.find(resolvedKey);
    if (resitr != resolvedProgramMap.end()) {
        program = resitr->second.get();
        programMap.insert(ProgramMap::value_type(prgKey, program));
        pass->setAttributeAndModes(program);
        return;
    }

    auto sgprogram = new SGProgram;
    program = sgprogram;
    sgprogram->setEffectFilePath(effect->filePath());

    for (const auto& skey : resolvedKey.shaders) {
        const string& fileName = skey.first;
        Shader::Type stype = (Shader::Type)skey.second;
        ShaderMap::iterator sitr = shaderMap.find(skey);
        if (sitr != shaderMap.end()) {
            program->addShader(sitr->second.get());
        } else {
            ref_ptr<Shader> shader = new Shader(stype);
			shader->setName(fileName);
            if (loadShaderFromUTF8File(shader, fileName)) {
                if (!program->addShader(shader.get())) {
                    simgear::reportFailure(simgear::LoadFailure::BadData,
                                           simgear::ErrorCode::LoadEffectsShaders,
                                           "Program::addShader failed",
                                           SGPath::fromUtf8(fileName));
                }

                shaderMap.insert(ShaderMap::value_type(skey, shader));
            } else {
                simgear::reportFailure(simgear::LoadFailure::BadData,
                                       simgear::ErrorCode::LoadEffectsShaders,
                                       "Failed to read shader source code",
                                       SGPath::fromUtf8(fileName));
            }
        }
    }

    if (sgprogram->getNumShaders() == 0) {
        simgear::reportFailure(simgear::LoadFailure::BadData,
                               simgear::ErrorCode::LoadEffectsShaders,
                               "No shader source code defined for effect",
                               effect->filePath());
    }

    for (const auto& key : prgKey.attributes) {
        program->addBindAttribLocation(key.first, key.second);
    }
    const SGPropertyNode* pGeometryVerticesOut
        = getEffectPropertyChild(effect, prop, "geometry-vertices-out");
    if (pGeometryVerticesOut)
        program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT,
                              pGeometryVerticesOut->getIntValue());
    const SGPropertyNode* pGeometryInputType
        = getEffectPropertyChild(effect, prop, "geometry-input-type");
    if (pGeometryInputType) {
        GLint type;
        findAttr(geometryInputType, pGeometryInputType->getStringValue(), type);
        program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, type);
    }
    const SGPropertyNode* pGeometryOutputType
        = getEffectPropertyChild(effect, prop, "geometry-output-type");
    if (pGeometryOutputType) {
        GLint type;
        findAttr(geometryOutputType, pGeometryOutputType->getStringValue(),
                 type);
        program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, type);
    }
    PropertyList pUniformBlockBindings
        = prop->getChildren("uniform-block-binding");
    for (const auto &pUniformBlockBinding : pUniformBlockBindings) {
        program->addBindUniformBlock(
            pUniformBlockBinding->getStringValue("name"),
            pUniformBlockBinding->getIntValue("index"));
    }
    programMap.insert(ProgramMap::value_type(prgKey, program));
    resolvedProgramMap.insert(ProgramMap::value_type(resolvedKey, program));
    pass->setAttributeAndModes(program);
}

InstallAttributeBuilder<ShaderProgramBuilder> installShaderProgram("program");

EffectNameValue<Uniform::Type> uniformTypesInit[] =
{
    {"bool", Uniform::BOOL},
    {"int", Uniform::INT},
    {"float", Uniform::FLOAT},
    {"float-vec3", Uniform::FLOAT_VEC3},
    {"float-vec4", Uniform::FLOAT_VEC4},
    {"sampler-1d", Uniform::SAMPLER_1D},
    {"sampler-1d-shadow", Uniform::SAMPLER_1D_SHADOW},
    {"sampler-2d", Uniform::SAMPLER_2D},
    {"sampler-2d-shadow", Uniform::SAMPLER_2D_SHADOW},
    {"sampler-3d", Uniform::SAMPLER_3D},
    {"sampler-cube", Uniform::SAMPLER_CUBE}
};
EffectPropertyMap<Uniform::Type> uniformTypes(uniformTypesInit);

// Optimization hack for common uniforms.
// XXX protect these with a mutex?

ref_ptr<Uniform> texture0;
ref_ptr<Uniform> colorMode[3];

struct UniformBuilder :public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        if (!texture0.valid()) {
            texture0 = new Uniform(Uniform::SAMPLER_2D, "texture");
            texture0->set(0);
            texture0->setDataVariance(Object::STATIC);
            for (int i = 0; i < 3; ++i) {
                colorMode[i] = new Uniform(Uniform::INT, "colorMode");
                colorMode[i]->set(i);
                colorMode[i]->setDataVariance(Object::STATIC);
            }
        }
        if (!isAttributeActive(effect, prop))
            return;
        SGConstPropertyNode_ptr nameProp = prop->getChild("name");
        SGConstPropertyNode_ptr typeProp = prop->getChild("type");
        SGConstPropertyNode_ptr positionedProp = prop->getChild("positioned");
        SGConstPropertyNode_ptr valProp = prop->getChild("value");
        string name;
        Uniform::Type uniformType = Uniform::FLOAT;
        if (nameProp) {
            name = nameProp->getStringValue();
        } else {
            SG_LOG(SG_INPUT, SG_ALERT, "No name for uniform property ");
            return;
        }
        if (!valProp) {
            SG_LOG(SG_INPUT, SG_ALERT, "No value for uniform property "
                   << name);
            return;
        }
        if (!typeProp) {
            props::Type propType = valProp->getType();
            switch (propType) {
            case props::BOOL:
                uniformType = Uniform::BOOL;
                break;
            case props::INT:
                uniformType = Uniform::INT;
                break;
            case props::FLOAT:
            case props::DOUBLE:
                break;          // default float type;
            case props::VEC3D:
                uniformType = Uniform::FLOAT_VEC3;
                break;
            case props::VEC4D:
                uniformType = Uniform::FLOAT_VEC4;
                break;
            default:
                SG_LOG(SG_INPUT, SG_ALERT, "Can't deduce type of uniform "
                       << name);
                return;
            }
        } else {
            findAttr(uniformTypes, typeProp, uniformType);
        }
        ref_ptr<Uniform> uniform = UniformFactory::instance()->
          getUniform( effect, name, uniformType, valProp, options );

        // optimize common uniforms
        if (uniformType == Uniform::SAMPLER_2D || uniformType == Uniform::INT)
        {
            int val = 0;
            uniform->get(val); // 'val' remains unchanged in case of error (Uniform is a non-scalar)
            if (uniformType == Uniform::SAMPLER_2D && val == 0
                && name == "texture") {
                uniform = texture0;
            } else if (uniformType == Uniform::INT && val >= 0 && val < 3
                       && name == "colorMode") {
                uniform = colorMode[val];
            }
        }
        pass->addUniform(uniform.get());
        if (positionedProp && positionedProp->getBoolValue() && uniformType == Uniform::FLOAT_VEC4) {
            osg::Vec4 offset;
            uniform->get(offset);
            pass->addPositionedUniform( name, offset );
        }
    }
};

InstallAttributeBuilder<UniformBuilder> installUniform("uniform");

// Not sure what to do with "name". At one point I wanted to use it to
// order the passes, but I do support render bin and stuff too...
// Clément de l'Hamaide 10/2014: "name" is now used in the UniformCacheKey

struct NameBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        // name can't use <use>
        string name = prop->getStringValue();
        if (!name.empty())
            pass->setName(name);
    }
};

InstallAttributeBuilder<NameBuilder> installName("name");

EffectNameValue<PolygonMode::Mode> polygonModeModesInit[] =
{
    {"fill", PolygonMode::FILL},
    {"line", PolygonMode::LINE},
    {"point", PolygonMode::POINT}
};
EffectPropertyMap<PolygonMode::Mode> polygonModeModes(polygonModeModesInit);

struct PolygonModeBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        if (!isAttributeActive(effect, prop))
            return;
        const SGPropertyNode* frontProp
            = getEffectPropertyChild(effect, prop, "front");
        const SGPropertyNode* backProp
            = getEffectPropertyChild(effect, prop, "back");
        ref_ptr<PolygonMode> pmode = new PolygonMode;
        PolygonMode::Mode frontMode = PolygonMode::FILL;
        PolygonMode::Mode backMode = PolygonMode::FILL;
        if (frontProp) {
            findAttr(polygonModeModes, frontProp, frontMode);
            pmode->setMode(PolygonMode::FRONT, frontMode);
        }
        if (backProp) {
            findAttr(polygonModeModes, backProp, backMode);
            pmode->setMode(PolygonMode::BACK, backMode);
        }
        pass->setAttribute(pmode.get());
    }
};

InstallAttributeBuilder<PolygonModeBuilder> installPolygonMode("polygon-mode");

struct PolygonOffsetBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        if (!isAttributeActive(effect, prop))
            return;

        const SGPropertyNode* factor
           = getEffectPropertyChild(effect, prop, "factor");
        const SGPropertyNode* units
           = getEffectPropertyChild(effect, prop, "units");

        ref_ptr<PolygonOffset> polyoffset = new PolygonOffset;

        polyoffset->setFactor(factor->getFloatValue());
        polyoffset->setUnits(units->getFloatValue());

        SG_LOG(SG_INPUT, SG_BULK,
                   "Set PolygonOffset to " << polyoffset->getFactor() << polyoffset->getUnits() );

        pass->setAttributeAndModes(polyoffset.get(),
                                   StateAttribute::OVERRIDE|StateAttribute::ON);
    }
};

InstallAttributeBuilder<PolygonOffsetBuilder> installPolygonOffset("polygon-offset");

struct VertexProgramTwoSideBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;
        pass->setMode(GL_VERTEX_PROGRAM_TWO_SIDE,
                      (realProp->getValue<bool>()
                       ? StateAttribute::ON : StateAttribute::OFF));
    }
};

InstallAttributeBuilder<VertexProgramTwoSideBuilder>
installTwoSide("vertex-program-two-side");

struct VertexProgramPointSizeBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;
        pass->setMode(GL_VERTEX_PROGRAM_POINT_SIZE,
                      (realProp->getValue<bool>()
                       ? StateAttribute::ON : StateAttribute::OFF));
    }
};

InstallAttributeBuilder<VertexProgramPointSizeBuilder>
installPointSize("vertex-program-point-size");

struct PointBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        float minsize = 1.0;
        float maxsize = 1.0;
        float size    = 1.0;
        osg::Vec3f attenuation = osg::Vec3f(1.0, 1.0, 1.0);

        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;

        const SGPropertyNode* pminsize
            = getEffectPropertyChild(effect, prop, "min-size");
        const SGPropertyNode* pmaxsize
            = getEffectPropertyChild(effect, prop, "max-size");
        const SGPropertyNode* psize
            = getEffectPropertyChild(effect, prop, "size");
        const SGPropertyNode* pattenuation
            = getEffectPropertyChild(effect, prop, "attenuation");

        if (pminsize)
            minsize = pminsize->getFloatValue();
        if (pmaxsize)
            maxsize = pmaxsize->getFloatValue();
        if (psize)
            size = psize->getFloatValue();
        if (pattenuation)
            attenuation = osg::Vec3f(pattenuation->getChild("x")->getFloatValue(),
                                     pattenuation->getChild("y")->getFloatValue(),
                                     pattenuation->getChild("z")->getFloatValue());

        osg::Point* point = new osg::Point;
        point->setMinSize(minsize);
        point->setMaxSize(maxsize);
        point->setSize(size);
        point->setDistanceAttenuation(attenuation);
        pass->setAttributeAndModes(point);
    }
};

InstallAttributeBuilder<PointBuilder>
installPoint("point");

EffectNameValue<Depth::Function> depthFunctionInit[] =
{
    {"never", Depth::NEVER},
    {"less", Depth::LESS},
    {"equal", Depth::EQUAL},
    {"lequal", Depth::LEQUAL},
    {"greater", Depth::GREATER},
    {"notequal", Depth::NOTEQUAL},
    {"gequal", Depth::GEQUAL},
    {"always", Depth::ALWAYS}
};
EffectPropertyMap<Depth::Function> depthFunction(depthFunctionInit);

struct DepthBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options)
    {
        if (!isAttributeActive(effect, prop))
            return;
        ref_ptr<Depth> depth = new Depth;
        const SGPropertyNode* pfunc
            = getEffectPropertyChild(effect, prop, "function");
        if (pfunc) {
            Depth::Function func = Depth::LESS;
            findAttr(depthFunction, pfunc, func);
            depth->setFunction(func);
        }
        const SGPropertyNode* pnear
            = getEffectPropertyChild(effect, prop, "near");
        if (pnear)
            depth->setZNear(pnear->getValue<double>());
        const SGPropertyNode* pfar
            = getEffectPropertyChild(effect, prop, "far");
        if (pfar)
            depth->setZFar(pfar->getValue<double>());
        const SGPropertyNode* pmask
            = getEffectPropertyChild(effect, prop, "write-mask");
        if (pmask)
            depth->setWriteMask(pmask->getValue<bool>());
        const SGPropertyNode* penabled
            = getEffectPropertyChild(effect, prop, "enabled");
        bool enabled = ( penabled == 0 || penabled->getBoolValue() );
        pass->setAttributeAndModes(depth.get(), enabled ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
    }
};

InstallAttributeBuilder<DepthBuilder> installDepth("depth");

void buildTechnique(Effect* effect, const SGPropertyNode* prop,
                    const SGReaderWriterOptions* options)
{
    simgear::ErrorReportContext ec("effect-technique", prop->getPath());

    Technique* tniq = new Technique;
    effect->techniques.push_back(tniq);
    tniq->setScheme(prop->getStringValue("scheme"));
    const SGPropertyNode* predProp = prop->getChild("predicate");
    if (!predProp) {
        tniq->setAlwaysValid(true);
    } else {
        try {
            TechniquePredParser parser;
            parser.setTechnique(tniq);
            expression::BindingLayout& layout = parser.getBindingLayout();
            /*int contextLoc = */layout.addBinding("__contextId", expression::INT);
            SGExpressionb* validExp
                = dynamic_cast<SGExpressionb*>(parser.read(predProp
                                                           ->getChild(0)));
            if (validExp)
                tniq->setValidExpression(validExp, layout);
            else
                throw expression::ParseError("technique predicate is not a boolean expression");
        }
        catch (expression::ParseError& except)
        {
            SG_LOG(SG_INPUT, SG_ALERT,
                   "parsing technique predicate " << except.getMessage());
            tniq->setAlwaysValid(false);
        }
    }
    PropertyList passProps = prop->getChildren("pass");
    for (PropertyList::iterator itr = passProps.begin(), e = passProps.end();
         itr != e;
         ++itr) {
        buildPass(effect, tniq, itr->ptr(), options);
    }
}

// Specifically for .ac files...
bool makeParametersFromStateSet(SGPropertyNode* effectRoot, const StateSet* ss)
{
    SGPropertyNode* paramRoot = makeChild(effectRoot, "parameters");
    SGPropertyNode* matNode = paramRoot->getChild("material", 0, true);
    Vec4f ambVal, difVal, specVal, emisVal;
    float shininess = 0.0f;
    const Material* mat = getStateAttribute<Material>(ss);
    if (mat) {
        ambVal = mat->getAmbient(Material::FRONT_AND_BACK);
        difVal = mat->getDiffuse(Material::FRONT_AND_BACK);
        specVal = mat->getSpecular(Material::FRONT_AND_BACK);
        emisVal = mat->getEmission(Material::FRONT_AND_BACK);
        shininess = mat->getShininess(Material::FRONT_AND_BACK);
        makeChild(matNode, "active")->setValue(true);
        makeChild(matNode, "ambient")->setValue(toVec4d(toSG(ambVal)));
        makeChild(matNode, "diffuse")->setValue(toVec4d(toSG(difVal)));
        makeChild(matNode, "specular")->setValue(toVec4d(toSG(specVal)));
        makeChild(matNode, "emissive")->setValue(toVec4d(toSG(emisVal)));
        makeChild(matNode, "shininess")->setValue(shininess);
        matNode->getChild("color-mode", 0, true)->setStringValue("diffuse");
    } else {
        makeChild(matNode, "active")->setValue(false);
    }
    const ShadeModel* sm = getStateAttribute<ShadeModel>(ss);
    string shadeModelString("smooth");
    if (sm) {
        ShadeModel::Mode smMode = sm->getMode();
        if (smMode == ShadeModel::FLAT)
            shadeModelString = "flat";
    }
    makeChild(paramRoot, "shade-model")->setStringValue(shadeModelString);
    string cullFaceString("off");
    const CullFace* cullFace = getStateAttribute<CullFace>(ss);
    if (cullFace) {
        switch (cullFace->getMode()) {
        case CullFace::FRONT:
            cullFaceString = "front";
            break;
        case CullFace::BACK:
            cullFaceString = "back";
            break;
        case CullFace::FRONT_AND_BACK:
            cullFaceString = "front-back";
            break;
        default:
            break;
        }
    }
    makeChild(paramRoot, "cull-face")->setStringValue(cullFaceString);
    // Macintosh ATI workaround
    bool vertexTwoSide = cullFaceString == "off";
    makeChild(paramRoot, "vertex-program-two-side")->setValue(vertexTwoSide);
    const BlendFunc* blendFunc = getStateAttribute<BlendFunc>(ss);
    SGPropertyNode* blendNode = makeChild(paramRoot, "blend");
    if (blendFunc) {
        string sourceMode = findName(blendFuncModes, blendFunc->getSource());
        string destMode = findName(blendFuncModes, blendFunc->getDestination());
        makeChild(blendNode, "active")->setValue(true);
        makeChild(blendNode, "source")->setStringValue(sourceMode);
        makeChild(blendNode, "destination")->setStringValue(destMode);
        makeChild(blendNode, "mode")->setValue(true);
    } else {
        makeChild(blendNode, "active")->setValue(false);
    }
    string renderingHint = findName(renderingHints, ss->getRenderingHint());
    makeChild(paramRoot, "rendering-hint")->setStringValue(renderingHint);
    makeTextureParameters(paramRoot, ss);
    return true;
}

SGPropertyNode_ptr schemeList;

void mergeSchemesFallbacks(Effect *effect, const SGReaderWriterOptions *options)
{
    if (!schemeList) {
        schemeList = new SGPropertyNode;
        const string schemes_file("Effects/schemes.xml");
        string absFileName
            = SGModelLib::findDataFile(schemes_file, options);
        if (absFileName.empty()) {
            SG_LOG(SG_INPUT, SG_ALERT, "Could not find '" << schemes_file << "'");
            return;
        }
        try {
            readProperties(absFileName, schemeList, 0, true);
        } catch (sg_io_exception& e) {
            SG_LOG(SG_INPUT, SG_ALERT, "Error reading '" << schemes_file <<
                   "': " << e.getFormattedMessage());
            return;
        }
    }

    PropertyList p_schemes = schemeList->getChildren("scheme");
    for (const auto &p_scheme : p_schemes) {
        string scheme_name   = p_scheme->getStringValue("name");
        string fallback_name = p_scheme->getStringValue("fallback");
        if (scheme_name.empty() || fallback_name.empty())
            continue;
        vector<SGPropertyNode_ptr> techniques = effect->root->getChildren("technique");
        auto it = std::find_if(techniques.begin(), techniques.end(),
                               [&scheme_name](const SGPropertyNode_ptr &tniq) {
                                   return tniq->getStringValue("scheme") == scheme_name;
                               });
        // Only merge the fallback effect if we haven't found a technique
        // implementing the scheme
        if (it == techniques.end()) {
            ref_ptr<Effect> fallback = makeEffect(fallback_name, false, options);
            if (fallback) {
                SGPropertyNode *new_root = new SGPropertyNode;
                mergePropertyTrees(new_root, effect->root, fallback->root);
                effect->root = new_root;
                effect->parametersProp = effect->root->getChild("parameters");
            }
        }
    }
}

// Walk the techniques property tree, building techniques and
// passes.
bool Effect::realizeTechniques(const SGReaderWriterOptions* options)
{
    simgear::ErrorReportContext ec{"effect", getName()};

    if (getPropertyRoot()->getBoolValue("/sim/version/compositor-support", false))
        mergeSchemesFallbacks(this, options);

    if (_isRealized)
        return true;

    PropertyList tniqList = root->getChildren("technique");
    for (const auto& tniq : tniqList) {
        buildTechnique(this, tniq, options);
    }

    _isRealized = true;
    return true;
}

void Effect::addDeferredPropertyListener(DeferredPropertyListener* listener)
{
	UniformFactory::instance()->addListener(listener);
}

void Effect::InitializeCallback::doUpdate(osg::Node* node, osg::NodeVisitor* nv)
{
    EffectGeode* eg = dynamic_cast<EffectGeode*>(node);
    if (!eg)
        return;
    Effect* effect = eg->getEffect();
    if (!effect)
        return;
    SGPropertyNode* root = getPropertyRoot();

    // Initialize all queued listeners
    UniformFactory::instance()->updateListeners(root);
}

bool Effect::Key::EqualTo::operator()(const Effect::Key& lhs,
                                      const Effect::Key& rhs) const
{
    if (lhs.paths.size() != rhs.paths.size()
        || !equal(lhs.paths.begin(), lhs.paths.end(), rhs.paths.begin()))
        return false;
    if (lhs.unmerged.valid() && rhs.unmerged.valid())
        return props::Compare()(lhs.unmerged, rhs.unmerged);
    else
        return lhs.unmerged == rhs.unmerged;
}

size_t hash_value(const Effect::Key& key)
{
    size_t seed = 0;
    if (key.unmerged.valid())
        boost::hash_combine(seed, *key.unmerged);
    boost::hash_range(seed, key.paths.begin(), key.paths.end());
    return seed;
}

bool Effect_writeLocalData(const Object& obj, osgDB::Output& fw)
{
    const Effect& effect = static_cast<const Effect&>(obj);

    fw.indent() << "techniques " << effect.techniques.size() << "\n";
    for (const auto& technique : effect.techniques) {
        fw.writeObject(*technique);
    }
    return true;
}

namespace
{
osgDB::RegisterDotOsgWrapperProxy effectProxy
(
    new Effect,
    "simgear::Effect",
    "Object simgear::Effect",
    0,
    &Effect_writeLocalData
    );
}

// Property expressions for technique predicates
template<typename T>
class PropertyExpression : public SGExpression<T>
{
public:
    PropertyExpression(SGPropertyNode* pnode) : _pnode(pnode), _listener(NULL) {}

    ~PropertyExpression()
    {
        delete _listener;
    }

    void eval(T& value, const expression::Binding*) const
    {
        value = _pnode->getValue<T>();
    }

    void setListener(SGPropertyChangeListener* l)
    {
        _listener = l;
    }
protected:
    SGPropertyNode_ptr _pnode;
    SGPropertyChangeListener* _listener;
};

class EffectPropertyListener : public SGPropertyChangeListener
{
public:
    EffectPropertyListener(Technique* tniq) : _tniq(tniq) {}

    void valueChanged(SGPropertyNode* node)
    {
        if (_tniq.valid())
            _tniq->refreshValidity();
    }

    virtual ~EffectPropertyListener() { }

protected:
    osg::observer_ptr<Technique> _tniq;
};

template<typename T>
Expression* propertyExpressionParser(const SGPropertyNode* exp,
                                     expression::Parser* parser)
{
    SGPropertyNode_ptr pnode = getPropertyRoot()->getNode(exp->getStringValue(),
                                                          true);
    PropertyExpression<T>* pexp = new PropertyExpression<T>(pnode);
    TechniquePredParser* predParser
        = dynamic_cast<TechniquePredParser*>(parser);
    if (predParser) {
        EffectPropertyListener* l = new EffectPropertyListener(predParser
                                                               ->getTechnique());
        pexp->setListener(l);
        pnode->addChangeListener(l);
    }
    return pexp;
}

expression::ExpParserRegistrar propertyRegistrar("property",
                                                 propertyExpressionParser<bool>);

expression::ExpParserRegistrar propvalueRegistrar("float-property",
                                                 propertyExpressionParser<float>);

}

using namespace simgear;

void Effect::setFilePath(const SGPath& path)
{
    _effectFilePath = path;
}

SGPath Effect::filePath() const
{
    return _effectFilePath;
}
