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
#include <simgear/scene/util/load_shader.hxx>
#include <simgear/scene/util/OsgUtils.hxx>
#include <simgear/structure/SGExpression.hxx>

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
			SG_LOG(SG_GL,SG_DEBUG,"Invalid parameter " << valProp->getNameString() << " for uniform " << name << " in Effect ");
		}
	}

	UniformCacheKey key = std::make_tuple(name, uniformType, val, effect->getName());
	ref_ptr<Uniform> uniform = uniformCache[key];

    if (uniform.valid()) {
    	// We've got a hit to cache - simply return it
    	return uniform;
    }

    SG_LOG(SG_GL,SG_DEBUG,"new uniform " << name << " value " << uniformCache.size());
    // REVIEW: Memory Leak - 13,800 bytes in 69 blocks are indirectly lost
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
    case Uniform::IMAGE_1D:
    case Uniform::IMAGE_2D:
    case Uniform::IMAGE_3D:
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

bool EffectSchemeSingleton::is_valid_scheme(const std::string& name,
                                            const SGReaderWriterOptions* options)
{
    if (!_schemes_xml_read) {
        read_schemes_xml(options);
    }
    if (name.empty()) {
        // Empty Effect scheme means the default scheme, which is a valid scheme
        return true;
    }
    auto it = std::find_if(_schemes.begin(), _schemes.end(),
                           [&name](const EffectScheme& scheme) {
                               return name == scheme.name;
                           });
    if (it != _schemes.end()) {
        return true;
    }
    return false;
}

void EffectSchemeSingleton::maybe_merge_fallbacks(Effect* effect,
                                                  const SGReaderWriterOptions* options)
{
    if (!_schemes_xml_read) {
        read_schemes_xml(options);
    }
    for (const auto& scheme : _schemes) {
        if (!scheme.fallback) {
            // The scheme does not have a fallback effect, skip
            continue;
        }
        const std::string& scheme_name = scheme.name;
        std::vector<SGPropertyNode_ptr> techniques = effect->root->getChildren("technique");
        auto it = std::find_if(techniques.begin(), techniques.end(),
                               [&scheme_name](const SGPropertyNode_ptr& tniq) {
                                   return tniq->getStringValue("scheme") == scheme_name;
                               });
        // Only merge the fallback effect if we haven't found a technique
        // implementing the scheme.
        if (it == techniques.end()) {
            SGPropertyNode_ptr new_root = new SGPropertyNode;
            mergePropertyTrees(new_root, effect->root, scheme.fallback->root);
            effect->root = new_root;
            effect->parametersProp = new_root->getChild("parameters");
            // Copy the generator only if it doesn't exist yet
            if (effect->generator.empty()) {
                effect->generator = scheme.fallback->generator;
            }
        }
    }
}

void EffectSchemeSingleton::read_schemes_xml(const SGReaderWriterOptions* options)
{
    SGPropertyNode_ptr scheme_list = new SGPropertyNode;
    const std::string schemes_file{"Effects/schemes.xml"};
    std::string abs_file_name = SGModelLib::findDataFile(schemes_file, options);
    if (abs_file_name.empty()) {
        SG_LOG(SG_INPUT, SG_ALERT, "Could not find Effect schemes file \"" << schemes_file << "\"");
        return;
    }
    try {
        readProperties(abs_file_name, scheme_list, 0, true);
    } catch (sg_io_exception& e) {
        SG_LOG(SG_INPUT, SG_ALERT, "Error reading Effect schemes file \""
               << schemes_file << "\": " << e.getFormattedMessage());
        return;
    }

    PropertyList p_schemes = scheme_list->getChildren("scheme");
    for (const auto& p_scheme : p_schemes) {
        EffectScheme scheme;
        scheme.name = p_scheme->getStringValue("name");
        if (scheme.name.empty()) {
            SG_LOG(SG_INPUT, SG_ALERT, "Scheme with index " << p_scheme->getIndex()
                   << " does not have a name. Skipping...");
            continue;
        }
        std::string fallback_name = p_scheme->getStringValue("fallback");
        if (!fallback_name.empty()) {
            // Read the fallback Effect
            scheme.fallback = makeEffect(fallback_name, false, options);
            if (!scheme.fallback.valid()) {
                SG_LOG(SG_INPUT, SG_ALERT, "Scheme fallback was provided (" << fallback_name
                       << ") for scheme \"" << scheme.name << "\", but it could not be built. Skipping...");
                continue;
            }
        }
        scheme.description = p_scheme->getStringValue("description");
        _schemes.push_back(scheme);
    }
    _schemes_xml_read = true;
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

// Try to use the state set of the last technique without a scheme.
StateSet* Effect::getDefaultStateSet()
{
    if (techniques.empty())
        return 0;
    auto it = std::find_if(techniques.rbegin(), techniques.rend(),
                           [](const osg::ref_ptr<Technique> &t) {
                               return t && t->getScheme().empty();
                           });
    if (it == techniques.rend())
        return 0;
    Technique* tniq = (*it);
    if (tniq->passes.empty())
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
        if ((technique->valid(info) == Technique::VALID) &&
            (technique->getScheme() == scheme))
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
    // REVIEW: Memory Leak - 35,712 bytes in 72 blocks are still reachable
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
                   "skipping unknown pass attribute " << attrProp->getNameString());
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
                   "invalid color property " << prop->getNameString() << " "
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
inline static std::mutex _programMap_mutex; // Protects the programMap and resolvedProgramMap for multi-threaded access

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
            simgear::loadShaderFromUTF8File(shader, fileName);
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
    PropertyList pCompShaders = prop->getChildren("compute-shader");
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
    std::transform(pCompShaders.begin(), pCompShaders.end(), inserter,
                   [] (SGPropertyNode_ptr& ptr) {
                       return makeShaderKey(ptr, Shader::COMPUTE); });
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

    std::lock_guard<std::mutex> lock(_programMap_mutex); // Lock the programMap and resolvedProgramMap for this scope
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
        string fileName = SGModelLib::findDataFile(shaderName, options);
        if (fileName.empty())
        {
            SG_LOG(SG_INPUT, SG_ALERT, "Could not locate shader" << shaderName);
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
            if (simgear::loadShaderFromUTF8File(shader, fileName)) {
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

    // disabling this check because some effects have disabled technqiues
    // with no shaders defined, intentionally.
#if 0
    if (sgprogram->getNumShaders() == 0) {
        simgear::reportFailure(simgear::LoadFailure::BadData,
                               simgear::ErrorCode::LoadEffectsShaders,
                               "No shader source code defined for effect",
                               effect->filePath());
    }
#endif

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
    {"sampler-cube", Uniform::SAMPLER_CUBE},
    {"image-1d", Uniform::IMAGE_1D},
    {"image-2d", Uniform::IMAGE_2D},
    {"image-3d", Uniform::IMAGE_3D},
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

        // REVIEW: Memory Leak - 144,400 bytes in 38 blocks are indirectly lost
        // Leak occurs within OSG, likely caused by passing a raw pointer
        pass->addUniform(uniform.get());
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

struct DefineBuilder : public PassAttributeBuilder {
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const SGReaderWriterOptions* options) {
        const SGPropertyNode* pName = getEffectPropertyChild(effect, prop, "name");
        if (!pName) {
            return;
        }
        const SGPropertyNode *pValue = getEffectPropertyChild(effect, prop, "value");
        if (pValue) {
            pass->setDefine(pName->getStringValue(), pValue->getStringValue());
        } else {
            pass->setDefine(pName->getStringValue());
        }
    }
};

InstallAttributeBuilder<DefineBuilder> installDefine("define");

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

        Depth::Function func = Depth::LESS;
        const SGPropertyNode* pfunc = getEffectPropertyChild(effect, prop, "function");
        if (pfunc)
            findAttr(depthFunction, pfunc, func);

        double near = 0.0;
        const SGPropertyNode* pnear = getEffectPropertyChild(effect, prop, "near");
        if (pnear)
            near = pnear->getValue<double>();

        double far = 1.0;
        const SGPropertyNode* pfar = getEffectPropertyChild(effect, prop, "far");
        if (pfar)
            far = pfar->getValue<double>();

        bool mask = true;
        const SGPropertyNode* pmask = getEffectPropertyChild(effect, prop, "write-mask");
        if (pmask)
            mask = pmask->getValue<bool>();

        ref_ptr<Depth> depth;
        // Try to get a cached depth state attribute
        if (func == Depth::LESS && equivalent(near, 0.0) && equivalent(far, 1.0))  {
            if (mask) {
                depth = StateAttributeFactory::instance()->getStandardDepth();
            } else {
                depth = StateAttributeFactory::instance()->getStandardDepthWritesDisabled();
            }
        } else {
            depth = new Depth;
            depth->setFunction(func);
            depth->setZNear(near);
            depth->setZFar(far);
            depth->setWriteMask(mask);
        }

        const SGPropertyNode* penabled = getEffectPropertyChild(effect, prop, "enabled");
        bool enabled = (penabled == 0 || penabled->getBoolValue());

        pass->setAttributeAndModes(depth.get(), enabled ? osg::StateAttribute::ON : osg::StateAttribute::OFF);
    }
};

InstallAttributeBuilder<DepthBuilder> installDepth("depth");

void buildTechnique(Effect* effect, const SGPropertyNode* prop,
                    const SGReaderWriterOptions* options)
{
    simgear::ErrorReportContext ec("effect-technique", prop->getPath());

    // REVIEW: Memory Leak - 13,248 bytes in 72 blocks are indirectly lost
    Technique* tniq = new Technique;
    effect->techniques.push_back(tniq);
    std::string scheme = prop->getStringValue("scheme");
    tniq->setScheme(scheme);
    if (!EffectSchemeSingleton::instance()->is_valid_scheme(scheme, options)) {
        SG_LOG(SG_INPUT, SG_ALERT, "technique scheme \"" << scheme << "\" is undefined");
        tniq->setAlwaysValid(false);
    }
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

// Walk the techniques property tree, building techniques and
// passes.
bool Effect::realizeTechniques(const SGReaderWriterOptions* options)
{
    if (_isRealized)
        return true;

    simgear::ErrorReportContext ec{"effect", getName()};

    EffectSchemeSingleton::instance()->maybe_merge_fallbacks(this, options);

    PropertyList tniqList = root->getChildren("technique");
    for (PropertyList::iterator itr = tniqList.begin(), e = tniqList.end();
         itr != e;
         ++itr)
        buildTechnique(this, *itr, options);
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
