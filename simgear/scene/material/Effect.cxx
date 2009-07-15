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

#include "Effect.hxx"
#include "Technique.hxx"
#include "Pass.hxx"

#include <algorithm>
#include <functional>
#include <iterator>
#include <map>
#include <utility>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include <osg/CullFace>
#include <osg/Drawable>
#include <osg/Material>
#include <osg/PolygonMode>
#include <osg/Program>
#include <osg/Referenced>
#include <osg/RenderInfo>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osg/TexEnv>
#include <osg/Texture2D>
#include <osg/Uniform>
#include <osg/Vec4d>
#include <osgUtil/CullVisitor>
#include <osgDB/FileUtils>
#include <osgDB/Input>
#include <osgDB/ParameterOutput>
#include <osgDB/ReadFile>
#include <osgDB/Registry>

#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/structure/OSGUtils.hxx>
#include <simgear/structure/SGExpression.hxx>



namespace simgear
{
using namespace std;
using namespace osg;
using namespace osgUtil;

Effect::Effect()
{
}

Effect::Effect(const Effect& rhs, const CopyOp& copyop)
    : root(rhs.root), parametersProp(rhs.parametersProp)
{
    using namespace boost;
    transform(rhs.techniques.begin(), rhs.techniques.end(),
              backRefInsertIterator(techniques),
              bind(simgear::clone_ref<Technique>, _1, copyop));
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

class PassAttributeBuilder : public Referenced
{
public:
    virtual void buildAttribute(Effect* effect, Pass* pass,
                                const SGPropertyNode* prop,
                                const osgDB::ReaderWriter::Options* options)
    = 0;
};

typedef map<const string, ref_ptr<PassAttributeBuilder> > PassAttrMap;
PassAttrMap passAttrMap;

template<typename T>
struct InstallAttributeBuilder
{
    InstallAttributeBuilder(const string& name)
    {
        passAttrMap.insert(make_pair(name, new T));
    }
};
// Simple tables of strings and OSG constants. The table intialization
// *must* be in alphabetical order.
template <typename T>
struct EffectNameValue
{
    // Don't use std::pair because we want to use aggregate intialization.

    const char* first;
    T second;
    class Compare
    {
    private:
        static bool compare(const char* lhs, const char* rhs)
        {
            return strcmp(lhs, rhs) < 0;
        }
    public:
        bool operator()(const EffectNameValue& lhs,
                        const EffectNameValue& rhs) const
        {
            return compare(lhs.first, rhs.first);
        }
        bool operator()(const char* lhs, const EffectNameValue& rhs) const
        {
            return compare(lhs, rhs.first);
        }
        bool operator()(const EffectNameValue& lhs, const char* rhs) const
        {
            return compare (lhs.first, rhs);
        }
    };
};

template<typename ENV, typename T, int N>
bool findAttr(const ENV (&attrs)[N], const SGPropertyNode* prop, T& result)
{
    if (!prop)
        return false;
    const char* name = prop->getStringValue();
    if (!name)
        return false;
    std::pair<const ENV*, const ENV*> itrs
        = std::equal_range(&attrs[0], &attrs[N], name, typename ENV::Compare());
    if (itrs.first == itrs.second) {
        return false;
    } else {
        result = itrs.first->second;
        return true;
    }
}

void buildPass(Effect* effect, Technique* tniq, const SGPropertyNode* prop,
               const osgDB::ReaderWriter::Options* options)
{
    Pass* pass = new Pass;
    tniq->passes.push_back(pass);
    for (int i = 0; i < prop->nChildren(); ++i) {
        const SGPropertyNode* attrProp = prop->getChild(i);
        PassAttrMap::iterator itr = passAttrMap.find(attrProp->getName());
        if (itr != passAttrMap.end())
            itr->second->buildAttribute(effect, pass, attrProp, options);
        else
            SG_LOG(SG_INPUT, SG_ALERT,
                   "skipping unknown pass attribute " << attrProp->getName());
    }
}

osg::Vec4f getColor(const SGPropertyNode* prop)
{
    if (prop->nChildren() == 0) {
        if (prop->getType() == props::VEC4D) {
            return osg::Vec4f(prop->getValue<SGVec4d>().osg());
        } else if (prop->getType() == props::VEC3D) {
            return osg::Vec4f(prop->getValue<SGVec3d>().osg(), 1.0f);
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

// Given a property node from a pass, get its value either from it or
// from the effect parameters.
const SGPropertyNode* getEffectPropertyNode(Effect* effect,
                                            const SGPropertyNode* prop)
{
    if (!prop)
        return 0;
    if (prop->nChildren() > 0) {
        const SGPropertyNode* useProp = prop->getChild("use");
        if (!useProp || !effect->parametersProp)
            return prop;
        return effect->parametersProp->getNode(useProp->getStringValue());
    }
    return prop;
}

// Get a named child property from pass parameters or effect
// parameters.
const SGPropertyNode* getEffectPropertyChild(Effect* effect,
                                             const SGPropertyNode* prop,
                                             const char* name)
{
    const SGPropertyNode* child = prop->getChild(name);
    if (!child)
        return 0;
    else
        return getEffectPropertyNode(effect, child);
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
        if (!realProp)
            return;
        StateAttributeFactory *attrFact = StateAttributeFactory::instance();
        string propVal = realProp->getStringValue();
        if (propVal == "front")
            pass->setAttributeAndModes(attrFact->getCullFaceFront());
        else if (propVal == "back")
            pass->setAttributeAndModes(attrFact->getCullFaceBack());
        else if (propVal == "front-back")
            pass->setAttributeAndModes(new CullFace(CullFace::FRONT_AND_BACK));
        else
            SG_LOG(SG_INPUT, SG_ALERT,
                   "invalid cull face property " << propVal);            
    }    
};

InstallAttributeBuilder<CullFaceBuilder> installCullFace("cull-face");

struct HintBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options)
    {
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;
        string propVal = realProp->getStringValue();
        if (propVal == "opaque")
            pass->setRenderingHint(StateSet::OPAQUE_BIN);
        else if (propVal == "transparent")
            pass->setRenderingHint(StateSet::TRANSPARENT_BIN);
    }    
};

InstallAttributeBuilder<HintBuilder> installHint("rendering-hint");

struct RenderBinBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options)
    {
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

EffectNameValue<Material::ColorMode> colorModes[] =
{
    { "ambient", Material::AMBIENT },
    { "ambient-and-diffuse", Material::AMBIENT_AND_DIFFUSE },
    { "diffuse", Material::DIFFUSE },
    { "emissive", Material::EMISSION },
    { "specular", Material::SPECULAR },
    { "off", Material::OFF }
};

void MaterialBuilder::buildAttribute(Effect* effect, Pass* pass,
                                     const SGPropertyNode* prop,
                                     const osgDB::ReaderWriter::Options* options)
{
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

struct BlendBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options)
    {
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;
        pass->setMode(GL_BLEND, (realProp->getBoolValue()
                                 ? StateAttribute::ON
                                 : StateAttribute::OFF));
    }
};

InstallAttributeBuilder<BlendBuilder> installBlend("blend");

struct AlphaTestBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options)
    {
        const SGPropertyNode* realProp = getEffectPropertyNode(effect, prop);
        if (!realProp)
            return;
        pass->setMode(GL_ALPHA_TEST, (realProp->getBoolValue()
                                      ? StateAttribute::ON
                                      : StateAttribute::OFF));
    }
};

InstallAttributeBuilder<AlphaTestBuilder> installAlphaTest("alpha-test");

EffectNameValue<Texture::FilterMode> filterModes[] =
{
    { "linear", Texture::LINEAR },
    { "linear-mipmap-linear", Texture::LINEAR_MIPMAP_LINEAR},
    { "linear-mipmap-nearest", Texture::LINEAR_MIPMAP_NEAREST},
    { "nearest", Texture::NEAREST},
    { "nearest-mipmap-linear", Texture::NEAREST_MIPMAP_LINEAR},
    { "nearest-mipmap-nearest", Texture::NEAREST_MIPMAP_NEAREST}
};

EffectNameValue<Texture::WrapMode> wrapModes[] =
{
    {"clamp", Texture::CLAMP},
    {"clamp-to-border", Texture::CLAMP_TO_BORDER},
    {"clamp-to-edge", Texture::CLAMP_TO_EDGE},
    {"mirror", Texture::MIRROR},
    {"repeat", Texture::REPEAT}
};

EffectNameValue<TexEnv::Mode> texEnvModes[] =
{
    {"add", TexEnv::ADD},
    {"blend", TexEnv::BLEND},
    {"decal", TexEnv::DECAL},
    {"modulate", TexEnv::MODULATE},
    {"replace", TexEnv::REPLACE}
};

TexEnv* buildTexEnv(Effect* effect, const SGPropertyNode* prop)
{
    const SGPropertyNode* modeProp = getEffectPropertyChild(effect, prop,
                                                            "mode");
    const SGPropertyNode* colorProp = getEffectPropertyChild(effect, prop,
                                                             "color");
    if (!modeProp)
        return 0;
    TexEnv::Mode mode = TexEnv::MODULATE;
    findAttr(texEnvModes, modeProp, mode);
    if (mode == TexEnv::MODULATE) {
        return StateAttributeFactory::instance()->getStandardTexEnv();
    }
    TexEnv* env = new TexEnv(mode);
    if (colorProp)
        env->setColor(colorProp->getValue<SGVec4d>().osg());
    return env;
 }

typedef boost::tuple<string, Texture::FilterMode, Texture::FilterMode,
                     Texture::WrapMode, Texture::WrapMode,
                     Texture::WrapMode> TexTuple;

typedef map<TexTuple, ref_ptr<Texture2D> > TexMap;

TexMap texMap;

struct TextureUnitBuilder : PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options);
};

void TextureUnitBuilder::buildAttribute(Effect* effect, Pass* pass,
                                        const SGPropertyNode* prop,
                                        const osgDB::ReaderWriter::Options* options)
{
    // First, all the texture properties
    const SGPropertyNode* pTexture2d = prop->getChild("texture2d");
    if (!pTexture2d)
        return;
    const SGPropertyNode* pImage
        = getEffectPropertyChild(effect, pTexture2d, "image");
    if (!pImage)
        return;
    const char* imageName = pImage->getStringValue();
    Texture::FilterMode minFilter = Texture::LINEAR_MIPMAP_LINEAR;
    findAttr(filterModes, getEffectPropertyChild(effect, pTexture2d, "filter"),
             minFilter);
    Texture::FilterMode magFilter = Texture::LINEAR;
    findAttr(filterModes, getEffectPropertyChild(effect, pTexture2d,
                                                 "mag-filter"),
             magFilter);
    const SGPropertyNode* pWrapS
        = getEffectPropertyChild(effect, pTexture2d, "wrap-s");
    Texture::WrapMode sWrap = Texture::CLAMP;
    findAttr(wrapModes, pWrapS, sWrap);
    const SGPropertyNode* pWrapT
        = getEffectPropertyChild(effect, pTexture2d, "wrap-t");
    Texture::WrapMode tWrap = Texture::CLAMP;
    findAttr(wrapModes, pWrapT, tWrap);
    const SGPropertyNode* pWrapR
        = getEffectPropertyChild(effect, pTexture2d, "wrap-r");
    Texture::WrapMode rWrap = Texture::CLAMP;
    findAttr(wrapModes, pWrapR, rWrap);
    TexTuple tuple(imageName, minFilter, magFilter, sWrap, tWrap, rWrap);
    TexMap::iterator texIter = texMap.find(tuple);
    Texture2D* texture = 0;
    if (texIter != texMap.end()) {
        texture = texIter->second.get();
    } else {
        texture = new Texture2D;
        osgDB::ReaderWriter::ReadResult result
            = osgDB::Registry::instance()->readImage(imageName, options);
        if (result.success()) {
            osg::Image* image = result.getImage();
            texture->setImage(image);
            int s = image->s();
            int t = image->t();

            if (s <= t && 32 <= s) {
                SGSceneFeatures::instance()->setTextureCompression(texture);
            } else if (t < s && 32 <= t) {
                SGSceneFeatures::instance()->setTextureCompression(texture);
            }
            texture->setMaxAnisotropy(SGSceneFeatures::instance()
                                      ->getTextureFilter());
        } else {
            SG_LOG(SG_INPUT, SG_ALERT, "failed to load effect texture file "
                   << imageName);
        }
        // texture->setDataVariance(osg::Object::STATIC);
        texture->setFilter(Texture::MIN_FILTER, minFilter);
        texture->setFilter(Texture::MAG_FILTER, magFilter);
        texture->setWrap(Texture::WRAP_S, sWrap);
        texture->setWrap(Texture::WRAP_T, tWrap);
        texture->setWrap(Texture::WRAP_R, rWrap);
        texMap.insert(make_pair(tuple, texture));
    }
    // Decode the texture unit
    int unit = 0;
    const SGPropertyNode* pUnit = prop->getChild("unit");
    if (pUnit) {
        unit = pUnit->getValue<int>();
    } else {
        const SGPropertyNode* pName = prop->getChild("name");
        if (pName)
            try {
                unit = boost::lexical_cast<int>(pName->getStringValue());
            } catch (boost::bad_lexical_cast& lex) {
                SG_LOG(SG_INPUT, SG_ALERT, "can't decode name as texture unit "
                       << lex.what());
            }
    }
    pass->setTextureAttributeAndModes(unit, texture);
    const SGPropertyNode* envProp = prop->getChild("environment");
    if (envProp) {
        TexEnv* env = buildTexEnv(effect, envProp);
        if (env)
            pass->setTextureAttributeAndModes(unit, env);
    }
}

InstallAttributeBuilder<TextureUnitBuilder> textureUnitBuilder("texture-unit");

typedef map<string, ref_ptr<Program> > ProgramMap;
ProgramMap programMap;

typedef map<string, ref_ptr<Shader> > ShaderMap;
ShaderMap shaderMap;

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
                    string fileName = osgDB::Registry::instance()
                        ->findDataFile(shaderName, options,
                                       osgDB::CASE_SENSITIVE);
                    if (!fileName.empty()) {
                        ref_ptr<Shader> shader = new Shader(stype);
                        if (shader->loadShaderSourceFromFile(fileName)) {
                            shaderMap.insert(make_pair(shaderName, shader));
                            program->addShader(shader.get());
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

EffectNameValue<Uniform::Type> uniformTypes[] =
{
    {"float", Uniform::FLOAT},
    {"float-vec3", Uniform::FLOAT_VEC3},
    {"float-vec4", Uniform::FLOAT_VEC4},
    {"sampler-1d", Uniform::SAMPLER_1D},
    {"sampler-2d", Uniform::SAMPLER_2D},
    {"sampler-3d", Uniform::SAMPLER_3D}
};

struct UniformBuilder :public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options)
    {
        using namespace simgear::props;
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
            case FLOAT:
            case DOUBLE:
                break;          // default float type;
            case VEC3D:
                uniformType = Uniform::FLOAT_VEC3;
                break;
            case VEC4D:
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
            uniform->set(Vec3f(valProp->getValue<SGVec3d>().osg()));
            break;
        case Uniform::FLOAT_VEC4:
            uniform->set(Vec4f(valProp->getValue<SGVec4d>().osg()));
            break;
        case Uniform::SAMPLER_1D:
        case Uniform::SAMPLER_2D:
        case Uniform::SAMPLER_3D:
            uniform->set(valProp->getValue<int>());
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

EffectNameValue<PolygonMode::Mode> polygonModeModes[] =
{
    {"fill", PolygonMode::FILL},
    {"line", PolygonMode::LINE},
    {"point", PolygonMode::POINT}
};

struct PolygonModeBuilder : public PassAttributeBuilder
{
    void buildAttribute(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                        const osgDB::ReaderWriter::Options* options)
    {
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
            int contextLoc = layout.addBinding("__contextId", expression::INT);
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

// Walk the techniques property tree, building techniques and
// passes.
bool Effect::realizeTechniques(const osgDB::ReaderWriter::Options* options)
{
    PropertyList tniqList = root->getChildren("technique");
    for (PropertyList::iterator itr = tniqList.begin(), e = tniqList.end();
         itr != e;
         ++itr)
        buildTechnique(this, *itr, options);
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
}

