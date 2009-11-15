// Copyright (C) 2008 - 2009  Tim Moore timoore@redhat.com
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
#include "Technique.hxx"
#include "Pass.hxx"
#include "TextureBuilder.hxx"

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <utility>

#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Drawable>
#include <osg/Material>
#include <osg/Math>
#include <osg/PolygonMode>
#include <osg/Program>
#include <osg/Referenced>
#include <osg/RenderInfo>
#include <osg/ShadeModel>
#include <osg/StateSet>
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

#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/structure/OSGUtils.hxx>
#include <simgear/structure/SGExpression.hxx>



namespace simgear
{
using namespace std;
using namespace osg;
using namespace osgUtil;

using namespace effect;

Effect::Effect()
{
}

Effect::Effect(const Effect& rhs, const CopyOp& copyop)
    : root(rhs.root), parametersProp(rhs.parametersProp)
{
    typedef vector<ref_ptr<Technique> > TechniqueList;
    for (TechniqueList::const_iterator itr = rhs.techniques.begin(),
             end = rhs.techniques.end();
         itr != end;
         ++itr)
        techniques.push_back(static_cast<Technique*>(copyop(itr->get())));
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

// There should always be a valid technique in an effect.

Technique* Effect::chooseTechnique(RenderInfo* info)
{
    BOOST_FOREACH(ref_ptr<Technique>& technique, techniques)
    {
        if (technique->valid(info) == Technique::VALID)
            return technique.get();
    }
    return 0;
}

void Effect::resizeGLObjectBuffers(unsigned int maxSize)
{
    BOOST_FOREACH(const ref_ptr<Technique>& technique, techniques)
    {
        technique->resizeGLObjectBuffers(maxSize);
    }
}

void Effect::releaseGLObjects(osg::State* state) const
{
    BOOST_FOREACH(const ref_ptr<Technique>& technique, techniques)
    {
        technique->releaseGLObjects(state);
    }
}

Effect::~Effect()
{
}

void buildPass(Effect* effect, Technique* tniq, const SGPropertyNode* prop,
               const osgDB::ReaderWriter::Options* options)
{
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
                        const osgDB::ReaderWriter::Options* options);
};

void LightingBuilder::buildAttribute(Effect* effect, Pass* pass,
                                     const SGPropertyNode* prop,
                                     const osgDB::ReaderWriter::Options* options)
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
                        const osgDB::ReaderWriter::Options* options)
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
                        const osgDB::ReaderWriter::Options* options)
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
                        const osgDB::ReaderWriter::Options* options)
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
                        const osgDB::ReaderWriter::Options* options)
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
                        const osgDB::ReaderWriter::Options* options);
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
                                     const osgDB::ReaderWriter::Options* options)
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
                        const osgDB::ReaderWriter::Options* options)
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
        const SGPropertyNode* psource
            = getEffectPropertyChild(effect, prop, "source");
        const SGPropertyNode* pdestination
            = getEffectPropertyChild(effect, prop, "destination");
        const SGPropertyNode* psourceRGB
            = getEffectPropertyChild(effect, prop, "source-rgb");
        const SGPropertyNode* psourceAlpha
            = getEffectPropertyChild(effect, prop, "source-alpha");
        const SGPropertyNode* pdestRGB
            = getEffectPropertyChild(effect, prop, "destination-rgb");
        const SGPropertyNode* pdestAlpha
            = getEffectPropertyChild(effect, prop, "destination-alpha");
        BlendFunc::BlendFuncMode sourceMode = BlendFunc::ONE;
        BlendFunc::BlendFuncMode destMode = BlendFunc::ZERO;
        if (psource)
            findAttr(blendFuncModes, psource, sourceMode);
        if (pdestination)
            findAttr(blendFuncModes, pdestination, destMode);
        if (psource && pdestination
            && !(psourceRGB || psourceAlpha || pdestRGB || pdestAlpha)
            && sourceMode == BlendFunc::SRC_ALPHA
            && destMode == BlendFunc::ONE_MINUS_SRC_ALPHA) {
            pass->setAttributeAndModes(StateAttributeFactory::instance()
                                       ->getStandardBlendFunc());
            return;
        }
        BlendFunc* blendFunc = new BlendFunc;
        if (psource)
            blendFunc->setSource(sourceMode);
        if (pdestination)
            blendFunc->setDestination(destMode);
        if (psourceRGB) {
            BlendFunc::BlendFuncMode sourceRGBMode;
            findAttr(blendFuncModes, psourceRGB, sourceRGBMode);
            blendFunc->setSourceRGB(sourceRGBMode);
        }
        if (pdestRGB) {
            BlendFunc::BlendFuncMode destRGBMode;
            findAttr(blendFuncModes, pdestRGB, destRGBMode);
            blendFunc->setDestinationRGB(destRGBMode);
        }
        if (psourceAlpha) {
            BlendFunc::BlendFuncMode sourceAlphaMode;
            findAttr(blendFuncModes, psourceAlpha, sourceAlphaMode);
            blendFunc->setSourceAlpha(sourceAlphaMode);
        }
        if (pdestAlpha) {
            BlendFunc::BlendFuncMode destAlphaMode;
            findAttr(blendFuncModes, pdestAlpha, destAlphaMode);
            blendFunc->setDestinationAlpha(destAlphaMode);
        }
        pass->setAttributeAndModes(blendFunc);
    }
};

InstallAttributeBuilder<BlendBuilder> installBlend("blend");

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
                        const osgDB::ReaderWriter::Options* options)
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

typedef map<string, ref_ptr<Program> > ProgramMap;
ProgramMap programMap;

typedef map<string, ref_ptr<Shader> > ShaderMap;
ShaderMap shaderMap;

void reload_shaders()
{
    for(ShaderMap::iterator sitr = shaderMap.begin(); sitr != shaderMap.end(); ++sitr)
    {
	Shader *shader = sitr->second.get();
        string fileName = osgDB::findDataFile(sitr->first);
        if (!fileName.empty()) {
	    shader->loadShaderSourceFromFile(fileName);
        }
    }
}

struct ShaderProgramBuilder : PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options);
};

void ShaderProgramBuilder::buildAttribute(Effect* effect, Pass* pass,
                                          const SGPropertyNode* prop,
                                          const osgDB::ReaderWriter::Options*
                                          options)
{
    if (!isAttributeActive(effect, prop))
        return;
    PropertyList pVertShaders = prop->getChildren("vertex-shader");
    PropertyList pFragShaders = prop->getChildren("fragment-shader");
    string programKey;
    for (PropertyList::iterator itr = pVertShaders.begin(),
             e = pVertShaders.end();
         itr != e;
         ++itr)
    {
        programKey += (*itr)->getStringValue();
        programKey += ";";
    }
    for (PropertyList::iterator itr = pFragShaders.begin(),
             e = pFragShaders.end();
         itr != e;
         ++itr)
    {
        programKey += (*itr)->getStringValue();
        programKey += ";";
    }
    Program* program = 0;
    ProgramMap::iterator pitr = programMap.find(programKey);
    if (pitr != programMap.end()) {
        program = pitr->second.get();
    } else {
        program = new Program;
        program->setName(programKey);
        // Add vertex shaders, then fragment shaders
        PropertyList& pvec = pVertShaders;
        Shader::Type stype = Shader::VERTEX;
        for (int i = 0; i < 2; ++i) {
            for (PropertyList::iterator nameItr = pvec.begin(), e = pvec.end();
                 nameItr != e;
                 ++nameItr) {
                string shaderName = (*nameItr)->getStringValue();
                ShaderMap::iterator sitr = shaderMap.find(shaderName);
                if (sitr != shaderMap.end()) {
                    program->addShader(sitr->second.get());
                } else {
                    string fileName = osgDB::findDataFile(shaderName, options);
                    if (!fileName.empty()) {
                        ref_ptr<Shader> shader = new Shader(stype);
                        if (shader->loadShaderSourceFromFile(fileName)) {
                            program->addShader(shader.get());
                            shaderMap.insert(make_pair(shaderName, shader));
                        }
                    }
                }
            }
            pvec = pFragShaders;
            stype = Shader::FRAGMENT;
        }
        programMap.insert(make_pair(programKey, program));
    }
    pass->setAttributeAndModes(program);
}

InstallAttributeBuilder<ShaderProgramBuilder> installShaderProgram("program");

EffectNameValue<Uniform::Type> uniformTypesInit[] =
{
    {"float", Uniform::FLOAT},
    {"float-vec3", Uniform::FLOAT_VEC3},
    {"float-vec4", Uniform::FLOAT_VEC4},
    {"sampler-1d", Uniform::SAMPLER_1D},
    {"sampler-2d", Uniform::SAMPLER_2D},
    {"sampler-3d", Uniform::SAMPLER_3D}
};
EffectPropertyMap<Uniform::Type> uniformTypes(uniformTypesInit);

struct UniformBuilder :public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options)
    {
        if (!isAttributeActive(effect, prop))
            return;
        const SGPropertyNode* nameProp = prop->getChild("name");
        const SGPropertyNode* typeProp = prop->getChild("type");
        const SGPropertyNode* valProp
            = getEffectPropertyChild(effect, prop, "value");
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
        ref_ptr<Uniform> uniform = new Uniform;
        uniform->setName(name);
        uniform->setType(uniformType);
        switch (uniformType) {
        case Uniform::FLOAT:
            uniform->set(valProp->getValue<float>());
            break;
        case Uniform::FLOAT_VEC3:
            uniform->set(toOsg(valProp->getValue<SGVec3d>()));
            break;
        case Uniform::FLOAT_VEC4:
            uniform->set(toOsg(valProp->getValue<SGVec4d>()));
            break;
        case Uniform::SAMPLER_1D:
        case Uniform::SAMPLER_2D:
        case Uniform::SAMPLER_3D:
            uniform->set(valProp->getValue<int>());
            break;
        default: // avoid compiler warning
            break;
        }
        pass->addUniform(uniform.get());
    }
};

InstallAttributeBuilder<UniformBuilder> installUniform("uniform");

// Not sure what to do with "name". At one point I wanted to use it to
// order the passes, but I do support render bin and stuff too...

struct NameBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options)
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
                        const osgDB::ReaderWriter::Options* options)
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
void buildTechnique(Effect* effect, const SGPropertyNode* prop,
                    const osgDB::ReaderWriter::Options* options)
{
    Technique* tniq = new Technique;
    effect->techniques.push_back(tniq);
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
    makeTextureParameters(paramRoot, ss);
    return true;
}

// Walk the techniques property tree, building techniques and
// passes.
bool Effect::realizeTechniques(const osgDB::ReaderWriter::Options* options)
{
    PropertyList tniqList = root->getChildren("technique");
    for (PropertyList::iterator itr = tniqList.begin(), e = tniqList.end();
         itr != e;
         ++itr)
        buildTechnique(this, *itr, options);
    return true;
}

bool Effect_writeLocalData(const Object& obj, osgDB::Output& fw)
{
    const Effect& effect = static_cast<const Effect&>(obj);

    fw.indent() << "techniques " << effect.techniques.size() << "\n";
    BOOST_FOREACH(const ref_ptr<Technique>& technique, effect.techniques) {
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
class PropertyExpression : public SGExpression<bool>
{
public:
    PropertyExpression(SGPropertyNode* pnode) : _pnode(pnode) {}
    
    void eval(bool& value, const expression::Binding*) const
    {
        value = _pnode->getValue<bool>();
    }
protected:
    SGPropertyNode_ptr _pnode;
};

class EffectPropertyListener : public SGPropertyChangeListener
{
public:
    EffectPropertyListener(Technique* tniq) : _tniq(tniq) {}
    
    void valueChanged(SGPropertyNode* node)
    {
        _tniq->refreshValidity();
    }
protected:
    osg::ref_ptr<Technique> _tniq;
};

Expression* propertyExpressionParser(const SGPropertyNode* exp,
                                     expression::Parser* parser)
{
    SGPropertyNode_ptr pnode = getPropertyRoot()->getNode(exp->getStringValue(),
                                                          true);
    PropertyExpression* pexp = new PropertyExpression(pnode);
    TechniquePredParser* predParser
        = dynamic_cast<TechniquePredParser*>(parser);
    if (predParser)
        pnode->addChangeListener(new EffectPropertyListener(predParser
                                                            ->getTechnique()));
    return pexp;
}

expression::ExpParserRegistrar propertyRegistrar("property",
                                                 propertyExpressionParser);

}
