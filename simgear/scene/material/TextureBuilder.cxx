// Copyright (C) 2009  Tim Moore timoore@redhat.com
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

#include "TextureBuilder.hxx"
#include "mipmap.hxx"

#include "Pass.hxx"

#include <osg/Version>
#include <osg/PointSprite>
#include <osg/TexEnv>
#include <osg/TexEnvCombine>
#include <osg/TexGen>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/structure/OSGUtils.hxx>

namespace simgear
{

using OpenThreads::Mutex;
using OpenThreads::ScopedLock;

using namespace std;
using namespace osg;

using namespace effect;

TexEnvCombine* buildTexEnvCombine(Effect* effect,
                                  const SGPropertyNode* envProp,
                                  const SGReaderWriterOptions* options);
TexGen* buildTexGen(Effect* Effect, const SGPropertyNode* tgenProp);

// Hack to force inclusion of TextureBuilder.cxx in library
osg::Texture* TextureBuilder::buildFromType(Effect* effect, Pass* pass, const string& type,
                                            const SGPropertyNode*props,
                                            const SGReaderWriterOptions*
                                            options)
{
    return EffectBuilder<Texture>::buildFromType(effect, pass, type, props, options);
}

typedef std::tuple<string, Texture::FilterMode, Texture::FilterMode,
                   Texture::WrapMode, Texture::WrapMode, Texture::WrapMode,
                   string, MipMapTuple, ImageInternalFormat> TexTuple;

EffectNameValue<TexEnv::Mode> texEnvModesInit[] =
{
    {"add", TexEnv::ADD},
    {"blend", TexEnv::BLEND},
    {"decal", TexEnv::DECAL},
    {"modulate", TexEnv::MODULATE},
    {"replace", TexEnv::REPLACE}
};
EffectPropertyMap<TexEnv::Mode> texEnvModes(texEnvModesInit);

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
        env->setColor(toOsg(colorProp->getValue<SGVec4d>()));
    return env;
}


void TextureUnitBuilder::buildAttribute(Effect* effect, Pass* pass,
                                        const SGPropertyNode* prop,
                                        const SGReaderWriterOptions* options)
{
    if (!isAttributeActive(effect, prop))
        return;
    // Decode the texture unit
    int unit = 0;
    const SGPropertyNode* pUnit = prop->getChild("unit");
    if (pUnit) {
        unit = pUnit->getValue<int>();
    } else {
        const SGPropertyNode* pName = prop->getChild("name");
        if (pName)
            try {
                unit = std::stoi(pName->getStringValue());
            }
            catch (const std::invalid_argument& ia) {
                SG_LOG(SG_INPUT, SG_ALERT, "can't decode name as texture unit "
                       << ia.what());
            }
    }
    const SGPropertyNode* pType = getEffectPropertyChild(effect, prop, "type");
    string type;
    if (!pType)
        type = "2d";
    else
        type = pType->getStringValue();
    Texture* texture = 0;
    try {
        texture = TextureBuilder::buildFromType(effect, pass, type, prop,
                                                options);
    }
    catch (BuilderException& e) {
        SG_LOG(SG_INPUT, SG_DEBUG, e.getFormattedMessage() << ", "
            << "maybe the reader did not set the filename attribute, "
            << "using white for type '" << type << "' on '" << pass->getName() << "', in " << prop->getPath() );
        texture = StateAttributeFactory::instance()->getWhiteTexture();
    }

    const SGPropertyNode* pPoint = getEffectPropertyChild(effect, prop, "point-sprite");
    if (pPoint && pPoint->getBoolValue()) {
        ref_ptr<PointSprite> pointSprite = new PointSprite;
        pass->setTextureAttributeAndModes(unit, pointSprite.get(), osg::StateAttribute::ON);
    }

    pass->setTextureAttributeAndModes(unit, texture);

    const SGPropertyNode* envProp = prop->getChild("environment");
    if (envProp) {
        TexEnv* env = buildTexEnv(effect, envProp);
        if (env)
            pass->setTextureAttributeAndModes(unit, env);
    }
    const SGPropertyNode* combineProp = prop->getChild("texenv-combine");
    TexEnvCombine* combiner = 0;
    if (combineProp && ((combiner = buildTexEnvCombine(effect, combineProp,
                                                       options))))
        pass->setTextureAttributeAndModes(unit, combiner);
    const SGPropertyNode* tgenProp = prop->getChild("texgen");
    TexGen* tgen = 0;
    if (tgenProp && (tgen = buildTexGen(effect, tgenProp)))
        pass->setTextureAttributeAndModes(unit, tgen);
}

// InstallAttributeBuilder call is in Effect.cxx to force this file to
// be linked in.

namespace
{
EffectNameValue<Texture::FilterMode> filterModesInit[] =
{
    { "linear", Texture::LINEAR },
    { "linear-mipmap-linear", Texture::LINEAR_MIPMAP_LINEAR},
    { "linear-mipmap-nearest", Texture::LINEAR_MIPMAP_NEAREST},
    { "nearest", Texture::NEAREST},
    { "nearest-mipmap-linear", Texture::NEAREST_MIPMAP_LINEAR},
    { "nearest-mipmap-nearest", Texture::NEAREST_MIPMAP_NEAREST}
};
EffectPropertyMap<Texture::FilterMode> filterModes(filterModesInit);

EffectNameValue<Texture::WrapMode> wrapModesInit[] =
{
    {"clamp", Texture::CLAMP},
    {"clamp-to-border", Texture::CLAMP_TO_BORDER},
    {"clamp-to-edge", Texture::CLAMP_TO_EDGE},
    {"mirror", Texture::MIRROR},
    {"repeat", Texture::REPEAT}
};
EffectPropertyMap<Texture::WrapMode> wrapModes(wrapModesInit);

TexTuple makeTexTuple(Effect* effect, const SGPropertyNode* props,
                      const SGReaderWriterOptions* options,
                      const string& texType)
{
    Texture::FilterMode minFilter = Texture::LINEAR_MIPMAP_LINEAR;
    const SGPropertyNode* ep = 0;
    if ((ep = getEffectPropertyChild(effect, props, "filter")))
        findAttr(filterModes, ep, minFilter);
    Texture::FilterMode magFilter = Texture::LINEAR;
    if ((ep = getEffectPropertyChild(effect, props, "mag-filter")))
        findAttr(filterModes, ep, magFilter);
    const SGPropertyNode* pWrapS
        = getEffectPropertyChild(effect, props, "wrap-s");
    Texture::WrapMode sWrap = Texture::CLAMP;
    if (pWrapS)
        findAttr(wrapModes, pWrapS, sWrap);
    const SGPropertyNode* pWrapT
        = getEffectPropertyChild(effect, props, "wrap-t");
    Texture::WrapMode tWrap = Texture::CLAMP;
    if (pWrapT)
        findAttr(wrapModes, pWrapT, tWrap);
    const SGPropertyNode* pWrapR
        = getEffectPropertyChild(effect, props, "wrap-r");
    Texture::WrapMode rWrap = Texture::CLAMP;
    if (pWrapR)
        findAttr(wrapModes, pWrapR, rWrap);
    const SGPropertyNode* pImage
        = getEffectPropertyChild(effect, props, "image");
    string imageName;
    string absFileName;
    if (pImage)
    {
        imageName = pImage->getStringValue();
        absFileName = SGModelLib::findDataFile(imageName, options);
        if (absFileName.empty())
        {
            SG_LOG(SG_INPUT, SG_ALERT, "Texture file not found: '"
                   << imageName << "'");
        }
    }

    const SGPropertyNode* pInternalFormat = getEffectPropertyChild(effect, props, "internal-format");
    pInternalFormat = props->getChild("internal-format");
    ImageInternalFormat iformat = ImageInternalFormat::Unspecified;
    if (pInternalFormat) {
        std::string internalFormat = pInternalFormat->getStringValue();
        if (internalFormat == "normalized") {
            iformat = ImageInternalFormat::Normalized;
            SG_LOG(SG_INPUT, SG_ALERT, "internal-format normalized '" << imageName << "'");
        }
    }

    const SGPropertyNode* pMipmapControl
        = getEffectPropertyChild(effect, props, "mipmap-control");
    MipMapTuple mipmapFunctions( AUTOMATIC, AUTOMATIC, AUTOMATIC, AUTOMATIC );
    if ( pMipmapControl )
        mipmapFunctions = makeMipMapTuple(effect, pMipmapControl, options);

    return TexTuple(absFileName, minFilter, magFilter, sWrap, tWrap, rWrap,
                    texType, mipmapFunctions, iformat);
}

bool setAttrs(const TexTuple& attrs, Texture* tex,
              const SGReaderWriterOptions* options)
{
    const string& imageName = std::get<0>(attrs);
    if (imageName.empty())
        return false;

    osgDB::ReaderWriter::ReadResult result;

    // load texture for effect
    SGReaderWriterOptions::LoadOriginHint origLOH = options->getLoadOriginHint();
    if (std::get<8>(attrs) == ImageInternalFormat::Normalized)
        options->setLoadOriginHint(SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS_NORMALIZED);
    else
        options->setLoadOriginHint(SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS);

    try {
    #if OSG_VERSION_LESS_THAN(3,4,2)
        result = osgDB::readImageFile(imageName, options);
    #else
        result = osgDB::readRefImageFile(imageName, options);
    #endif
    } catch (std::exception& e) {
        simgear::reportFailure(simgear::LoadFailure::OutOfMemory, simgear::ErrorCode::LoadingTexture,
                               string{"osgDB::readRefImageFile failed:"} + e.what(),
                               SGPath::fromUtf8(imageName));
        return false;
    }

    options->setLoadOriginHint(origLOH);
    osg::ref_ptr<osg::Image> image;
    if (result.success())
        image = result.getImage();
    if (image.valid())
    {
        image = computeMipmap( image.get(), std::get<7>(attrs) );
        tex->setImage(GL_FRONT_AND_BACK, image.get());
        int s = image->s();
        int t = image->t();
        if (s <= t && 32 <= s) {
            SGSceneFeatures::instance()->applyTextureCompression(tex);
        } else if (t < s && 32 <= t) {
            SGSceneFeatures::instance()->applyTextureCompression(tex);
        }
        tex->setMaxAnisotropy(SGSceneFeatures::instance()->getTextureFilter());
    } else {
        SG_LOG(SG_INPUT, SG_ALERT, "failed to load effect texture file " << imageName);
        simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::LoadingTexture,
                               "osgDB::readRefImageFile failed:" + result.message(),
                               SGPath::fromUtf8(imageName));
        return false;
    }

    // texture->setDataVariance(osg::Object::STATIC);
    tex->setFilter(Texture::MIN_FILTER, std::get<1>(attrs));
    tex->setFilter(Texture::MAG_FILTER, std::get<2>(attrs));
    tex->setWrap(Texture::WRAP_S, std::get<3>(attrs));
    tex->setWrap(Texture::WRAP_T, std::get<4>(attrs));
    tex->setWrap(Texture::WRAP_R, std::get<5>(attrs));
    return true;
}

} // of anonymous namespace

template<typename T>
class TexBuilder : public TextureBuilder
{
public:
    TexBuilder(const string& texType) : _type(texType) {}
    Texture* build(Effect* effect, Pass* pass, const SGPropertyNode*,
                   const SGReaderWriterOptions* options);
protected:
    typedef map<TexTuple, observer_ptr<T> > TexMap;
    TexMap texMap;
    const string _type;
};

template<typename T>
Texture* TexBuilder<T>::build(Effect* effect, Pass* pass, const SGPropertyNode* props,
                              const SGReaderWriterOptions* options)
{
    TexTuple attrs = makeTexTuple(effect, props, options, _type);
    typename TexMap::iterator itr = texMap.find(attrs);

    ref_ptr<T> tex;
    if ((itr != texMap.end())&&
        (itr->second.lock(tex)))
    {
        return tex.release();
    }

    tex = new T;
    if (!setAttrs(attrs, tex, options))
        return NULL;

    if (itr == texMap.end())
        texMap.insert(make_pair(attrs, tex));
    else
        itr->second = tex; // update existing, but empty observer
    return tex.release();
}


namespace
{
TextureBuilder::Registrar install1D("1d", new TexBuilder<Texture1D>("1d"));
TextureBuilder::Registrar install2D("2d", new TexBuilder<Texture2D>("2d"));
//TextureBuilder::Registrar install3D("3d", new TexBuilder<Texture3D>("3d"));
}

class WhiteTextureBuilder : public TextureBuilder
{
public:
    Texture* build(Effect* effect, Pass* pass, const SGPropertyNode*,
                   const SGReaderWriterOptions* options);
};

Texture* WhiteTextureBuilder::build(Effect* effect, Pass* pass, const SGPropertyNode*,
                                    const SGReaderWriterOptions* options)
{
    return StateAttributeFactory::instance()->getWhiteTexture();
}

namespace
{
TextureBuilder::Registrar installWhite("white", new WhiteTextureBuilder);
}

class TransparentTextureBuilder : public TextureBuilder
{
public:
    Texture* build(Effect* effect, Pass* pass, const SGPropertyNode*,
                   const SGReaderWriterOptions* options);
};

Texture* TransparentTextureBuilder::build(Effect* effect, Pass* pass, const SGPropertyNode*,
                                    const SGReaderWriterOptions* options)
{
    return StateAttributeFactory::instance()->getTransparentTexture();
}

namespace
{
TextureBuilder::Registrar installTransparent("transparent",
                                             new TransparentTextureBuilder);
}

class NoiseBuilder : public TextureBuilder
{
public:
    Texture* build(Effect* effect, Pass* pass, const SGPropertyNode*,
                   const SGReaderWriterOptions* options);
protected:
    typedef map<int, ref_ptr<Texture3D> > NoiseMap;
    NoiseMap _noises;
};

Texture* NoiseBuilder::build(Effect* effect, Pass* pass, const SGPropertyNode* props,
                             const SGReaderWriterOptions* options)
{
    int texSize = 64;
    const SGPropertyNode* sizeProp = getEffectPropertyChild(effect, props,
                                                            "size");
    if (sizeProp)
        texSize = sizeProp->getValue<int>();

    return StateAttributeFactory::instance()->getNoiseTexture(texSize);
}

namespace
{
TextureBuilder::Registrar installNoise("noise", new NoiseBuilder);
}

class LightSpriteBuilder : public TextureBuilder
{
public:
    Texture* build(Effect* effect, Pass* pass, const SGPropertyNode*,
                   const SGReaderWriterOptions* options);
protected:
    Mutex lightMutex;
    void setPointSpriteImage(unsigned char* data, unsigned log2resolution,
                    unsigned charsPerPixel);
    osg::Image* getPointSpriteImage(int logResolution);
};

Texture* LightSpriteBuilder::build(Effect* effect, Pass* pass, const SGPropertyNode* props,
                             const SGReaderWriterOptions* options)
{
    ScopedLock<Mutex> lock(lightMutex);

    // Always called from when the lightMutex is already taken
    static osg::ref_ptr<osg::Texture2D> texture;

    if (texture.valid())
      return texture.get();

    texture = new osg::Texture2D;
    texture->setImage(getPointSpriteImage(6));
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);

    return texture.get();
}

void
LightSpriteBuilder::setPointSpriteImage(unsigned char* data, unsigned log2resolution,
                    unsigned charsPerPixel)
{
  int env_tex_res = (1 << log2resolution);
  for (int i = 0; i < env_tex_res; ++i) {
    for (int j = 0; j < env_tex_res; ++j) {
      int xi = 2*i + 1 - env_tex_res;
      int yi = 2*j + 1 - env_tex_res;
      if (xi < 0)
        xi = -xi;
      if (yi < 0)
        yi = -yi;

      xi -= 1;
      yi -= 1;

      if (xi < 0)
        xi = 0;
      if (yi < 0)
        yi = 0;

      float x = 1.5*xi/(float)(env_tex_res);
      float y = 1.5*yi/(float)(env_tex_res);
      //       float x = 2*xi/(float)(env_tex_res);
      //       float y = 2*yi/(float)(env_tex_res);
      float dist = sqrt(x*x + y*y);
      float bright = SGMiscf::clip(255*(1-dist), 0, 255);
      for (unsigned l = 0; l < charsPerPixel; ++l)
        data[charsPerPixel*(i*env_tex_res + j) + l] = (unsigned char)bright;
    }
  }
}

osg::Image*
LightSpriteBuilder::getPointSpriteImage(int logResolution)
{
  osg::Image* image = new osg::Image;

  osg::Image::MipmapDataType mipmapOffsets;
  unsigned off = 0;
  for (int i = logResolution; 0 <= i; --i) {
    unsigned res = 1 << i;
    off += res*res;
    mipmapOffsets.push_back(off);
  }

  int env_tex_res = (1 << logResolution);

  unsigned char* imageData = new unsigned char[off];
  image->setImage(env_tex_res, env_tex_res, 1,
                  GL_ALPHA, GL_ALPHA, GL_UNSIGNED_BYTE, imageData,
                  osg::Image::USE_NEW_DELETE);
  image->setMipmapLevels(mipmapOffsets);

  for (int k = logResolution; 0 <= k; --k) {
    setPointSpriteImage(image->getMipmapData(logResolution - k), k, 1);
  }

  return image;
}

namespace
{
TextureBuilder::Registrar installLightSprite("light-sprite", new LightSpriteBuilder);
}

// Image names for all sides
typedef std::tuple<string, string, string, string, string, string> CubeMapTuple;

CubeMapTuple makeCubeMapTuple(Effect* effect, const SGPropertyNode* props)
{
    const SGPropertyNode* ep = 0;

    string positive_x;
    if ((ep = getEffectPropertyChild(effect, props, "positive-x")))
        positive_x = ep->getStringValue();
    string negative_x;
    if ((ep = getEffectPropertyChild(effect, props, "negative-x")))
        negative_x = ep->getStringValue();
    string positive_y;
    if ((ep = getEffectPropertyChild(effect, props, "positive-y")))
        positive_y = ep->getStringValue();
    string negative_y;
    if ((ep = getEffectPropertyChild(effect, props, "negative-y")))
        negative_y = ep->getStringValue();
    string positive_z;
    if ((ep = getEffectPropertyChild(effect, props, "positive-z")))
        positive_z = ep->getStringValue();
    string negative_z;
    if ((ep = getEffectPropertyChild(effect, props, "negative-z")))
        negative_z = ep->getStringValue();
    return CubeMapTuple(positive_x, negative_x, positive_y, negative_y, positive_z, negative_z);
}


class CubeMapBuilder : public TextureBuilder
{
public:
    Texture* build(Effect* effect, Pass* pass, const SGPropertyNode*,
                   const SGReaderWriterOptions* options);
protected:
    typedef map<CubeMapTuple, observer_ptr<TextureCubeMap> > CubeMap;
    typedef map<string, observer_ptr<TextureCubeMap> > CrossCubeMap;
    CubeMap _cubemaps;
    CrossCubeMap _crossmaps;
};

// I use this until osg::CopyImage is fixed
// This one assumes images are the same format and sizes are correct
void copySubImage(const osg::Image* srcImage, int src_s, int src_t, int width, int height,
                 osg::Image* destImage, int dest_s, int dest_t)
{
    for(int row = 0; row<height; ++row)
    {
        const unsigned char* srcData = srcImage->data(src_s, src_t+row, 0);
        unsigned char* destData = destImage->data(dest_s, dest_t+row, 0);
        memcpy(destData, srcData, (width*destImage->getPixelSizeInBits())/8);
    }
}


Texture* CubeMapBuilder::build(Effect* effect, Pass* pass, const SGPropertyNode* props,
                               const SGReaderWriterOptions* options)
{
    // First check that there is a <images> tag
    const SGPropertyNode* texturesProp = getEffectPropertyChild(effect, props, "images");
    const SGPropertyNode* crossProp = getEffectPropertyChild(effect, props, "image");
    if (!texturesProp && !crossProp) {
        simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::LoadingTexture, "No images defined for cube map");
        throw BuilderException("no images defined for cube map");
    }

    // Using 6 separate images
    if(texturesProp) {
        CubeMapTuple _tuple = makeCubeMapTuple(effect, texturesProp);

        CubeMap::iterator itr = _cubemaps.find(_tuple);
        ref_ptr<TextureCubeMap> cubeTexture;
        
        if (itr != _cubemaps.end() && itr->second.lock(cubeTexture))
            return cubeTexture.release();

        cubeTexture = new osg::TextureCubeMap;

        // TODO: Read these from effect file? Maybe these are sane for all cuebmaps?
        cubeTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
        cubeTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture::LINEAR);
        cubeTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        cubeTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        cubeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture::CLAMP_TO_EDGE);

        osgDB::ReaderWriter::ReadResult result;
        SGReaderWriterOptions* wOpts = (SGReaderWriterOptions*)options;
        SGReaderWriterOptions::LoadOriginHint origLOH = wOpts->getLoadOriginHint();
        wOpts->setLoadOriginHint(SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS);
#if OSG_VERSION_LESS_THAN(3,4,1)
        result = osgDB::readImageFile(std::get<0>(_tuple), options);
#else
        result = osgDB::readRefImageFile(std::get<0>(_tuple), options);
#endif
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::POSITIVE_X, image);
        }
#if OSG_VERSION_LESS_THAN(3,4,1)
        result = osgDB::readImageFile(std::get<1>(_tuple), options);
#else
        result = osgDB::readRefImageFile(std::get<1>(_tuple), options);
#endif
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::NEGATIVE_X, image);
        }
#if OSG_VERSION_LESS_THAN(3,4,1)
        result = osgDB::readImageFile(std::get<2>(_tuple), options);
#else
        result = osgDB::readRefImageFile(std::get<2>(_tuple), options);
#endif
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::POSITIVE_Y, image);
        }
#if OSG_VERSION_LESS_THAN(3,4,1)
        result = osgDB::readImageFile(std::get<3>(_tuple), options);
#else
        result = osgDB::readRefImageFile(std::get<3>(_tuple), options);
#endif
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::NEGATIVE_Y, image);
        }
#if OSG_VERSION_LESS_THAN(3,4,1)
        result = osgDB::readImageFile(std::get<4>(_tuple), options);
#else
        result = osgDB::readRefImageFile(std::get<4>(_tuple), options);
#endif
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::POSITIVE_Z, image);
        }
#if OSG_VERSION_LESS_THAN(3,4,1)
        result = osgDB::readImageFile(std::get<5>(_tuple), options);
#else
        result = osgDB::readRefImageFile(std::get<5>(_tuple), options);
#endif
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::NEGATIVE_Z, image);
        }
        wOpts->setLoadOriginHint(origLOH);

        if (itr == _cubemaps.end())
            _cubemaps[_tuple] = cubeTexture;
        else
            itr->second = cubeTexture; // update existing
        return cubeTexture.release();
    }

    // Using 1 cross image
    else if(crossProp) {
        std::string texname = crossProp->getStringValue();

        // Try to find existing cube map
        ref_ptr<TextureCubeMap> cubeTexture;
        CrossCubeMap::iterator itr = _crossmaps.find(texname);
        if ((itr != _crossmaps.end()) && itr->second.lock(cubeTexture))
            return cubeTexture.release();

        osgDB::ReaderWriter::ReadResult result;
#if OSG_VERSION_LESS_THAN(3,4,1)
        result = osgDB::readImageFile(texname, options);
#else
        result = osgDB::readRefImageFile(texname, options);
#endif
        if(result.success()) {
            osg::Image* image = result.getImage();
            image->flipVertical();   // Seems like the image coordinates are somewhat funny, flip to get better ones

            //cubeTexture->setResizeNonPowerOfTwoHint(false);

            // Size of a single image, 4 rows and 3 columns
            int width = image->s() / 3;
            int height = image->t() / 4;
            int depth = image->r();

            cubeTexture = new osg::TextureCubeMap;

            // Copy the 6 sub-images and push them
            for(int n=0; n<6; n++) {

                SG_LOG(SG_INPUT, SG_DEBUG, "Copying the " << n << "th sub-images and pushing it" );

                osg::ref_ptr<osg::Image> subimg = new osg::Image();
                subimg->allocateImage(width, height, depth, image->getPixelFormat(), image->getDataType());  // Copy attributes

                // Choose correct image
                switch(n) {
                case 0:  // Front
                    copySubImage(image, width, 0, width, height, subimg.get(), 0, 0);
                    cubeTexture->setImage(TextureCubeMap::POSITIVE_Y, subimg.get());
                    cubeTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
                    break;
                case 1:  // Left
                    copySubImage(image, 0, height, width, height, subimg.get(), 0, 0);
                    cubeTexture->setImage(TextureCubeMap::NEGATIVE_X, subimg.get());
                    cubeTexture->setWrap(osg::Texture2D::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
                    break;
                case 2:  // Top
                    copySubImage(image, width, height, width, height, subimg.get(), 0, 0);
                    cubeTexture->setImage(TextureCubeMap::POSITIVE_Z, subimg.get());
                    cubeTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
                    break;
                case 3:  // Right
                    copySubImage(image, width*2, height, width, height, subimg.get(), 0, 0);
                    cubeTexture->setImage(TextureCubeMap::POSITIVE_X, subimg.get());
                    cubeTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
                    break;
                case 4:  // Back
                    copySubImage(image, width, height*2, width, height, subimg.get(), 0, 0);
                    cubeTexture->setImage(TextureCubeMap::NEGATIVE_Y, subimg.get());
                    cubeTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
                    break;
                case 5:  // Bottom
                    copySubImage(image, width, height*3, width, height, subimg.get(), 0, 0);
                    cubeTexture->setImage(TextureCubeMap::NEGATIVE_Z, subimg.get());
                    cubeTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
                    cubeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
                    break;
                };

            }

            if (itr == _crossmaps.end())
                _crossmaps[texname] = cubeTexture;
            else
                itr->second = cubeTexture; // update existing
            return cubeTexture.release();

        } else {
            simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::LoadingTexture,
                                   "Could not load cube-map image:" + result.message(),
                                   sg_location{texname});
            throw BuilderException("Could not load cube cross");
        }
    }

    return NULL;
}

namespace {
TextureBuilder::Registrar installCubeMap("cubemap", new CubeMapBuilder);
}


class Texture3DBuilder : public TextureBuilder
{
public:
    Texture* build(Effect* effect, Pass* pass, const SGPropertyNode*,
                   const SGReaderWriterOptions* options);
protected:
    typedef map<TexTuple, observer_ptr<Texture3D> > TexMap;
    TexMap texMap;
};

Texture* Texture3DBuilder::build(Effect* effect, Pass* pass,
                                 const SGPropertyNode* props,
                                 const SGReaderWriterOptions* options)
{
    TexTuple attrs = makeTexTuple(effect, props, options, "3d");
    typename TexMap::iterator itr = texMap.find(attrs);

    ref_ptr<Texture3D> tex;
    if ((itr != texMap.end())&&
        (itr->second.lock(tex)))
    {
        return tex.release();
    }

    tex = new Texture3D;

    const string& imageName = std::get<0>(attrs);
    if (imageName.empty())
        return NULL;

    osgDB::ReaderWriter::ReadResult result;

    // load texture for effect
    SGReaderWriterOptions::LoadOriginHint origLOH = options->getLoadOriginHint();
    if (std::get<8>(attrs) == ImageInternalFormat::Normalized)
        options->setLoadOriginHint(SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS_NORMALIZED);
    else
        options->setLoadOriginHint(SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS);
#if OSG_VERSION_LESS_THAN(3,4,2)
    result = osgDB::readImageFile(imageName, options);
#else
    result = osgDB::readRefImageFile(imageName, options);
#endif
    options->setLoadOriginHint(origLOH);
    osg::ref_ptr<osg::Image> image;
    if (result.success())
        image = result.getImage();
    if (image.valid())
    {
        osg::ref_ptr<osg::Image> image3d = new osg::Image;
        int size = image->t();
        int depth = image->s() / image->t();
        image3d->allocateImage(size, size, depth,
                               image->getPixelFormat(), image->getDataType());

        for (int i = 0; i < depth; ++i) {
            osg::ref_ptr<osg::Image> subimage = new osg::Image;
            subimage->allocateImage(size, size, 1,
                                    image->getPixelFormat(), image->getDataType());
            copySubImage(image, size * i, 0, size, size, subimage.get(), 0, 0);
            image3d->copySubImage(0, 0, i, subimage.get());
        }

        image3d->setInternalTextureFormat(image->getInternalTextureFormat());
        image3d = computeMipmap(image3d.get(), std::get<7>(attrs));
        tex->setImage(image3d.get());
    } else {
        SG_LOG(SG_INPUT, SG_ALERT, "failed to load effect texture file " << imageName);
        simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::LoadingTexture,
                               "osgDB::readRefImageFile failed:" + result.message(),
                               SGPath::fromUtf8(imageName));
        return NULL;
    }

    tex->setFilter(Texture::MIN_FILTER, std::get<1>(attrs));
    tex->setFilter(Texture::MAG_FILTER, std::get<2>(attrs));
    tex->setWrap(Texture::WRAP_S, std::get<3>(attrs));
    tex->setWrap(Texture::WRAP_T, std::get<4>(attrs));
    tex->setWrap(Texture::WRAP_R, std::get<5>(attrs));

    if (itr == texMap.end())
        texMap.insert(make_pair(attrs, tex));
    else
        itr->second = tex; // update existing, but empty observer
    return tex.release();
}

namespace {
TextureBuilder::Registrar install3D("3d", new Texture3DBuilder);
}


EffectNameValue<TexEnvCombine::CombineParam> combineParamInit[] =
{
    {"replace", TexEnvCombine::REPLACE},
    {"modulate", TexEnvCombine::MODULATE},
    {"add", TexEnvCombine::ADD},
    {"add-signed", TexEnvCombine::ADD_SIGNED},
    {"interpolate", TexEnvCombine::INTERPOLATE},
    {"subtract", TexEnvCombine::SUBTRACT},
    {"dot3-rgb", TexEnvCombine::DOT3_RGB},
    {"dot3-rgba", TexEnvCombine::DOT3_RGBA}
};

EffectPropertyMap<TexEnvCombine::CombineParam> combineParams(combineParamInit);

EffectNameValue<TexEnvCombine::SourceParam> sourceParamInit[] =
{
    {"constant", TexEnvCombine::CONSTANT},
    {"primary_color", TexEnvCombine::PRIMARY_COLOR},
    {"previous", TexEnvCombine::PREVIOUS},
    {"texture", TexEnvCombine::TEXTURE},
    {"texture0", TexEnvCombine::TEXTURE0},
    {"texture1", TexEnvCombine::TEXTURE1},
    {"texture2", TexEnvCombine::TEXTURE2},
    {"texture3", TexEnvCombine::TEXTURE3},
    {"texture4", TexEnvCombine::TEXTURE4},
    {"texture5", TexEnvCombine::TEXTURE5},
    {"texture6", TexEnvCombine::TEXTURE6},
    {"texture7", TexEnvCombine::TEXTURE7}
};

EffectPropertyMap<TexEnvCombine::SourceParam> sourceParams(sourceParamInit);

EffectNameValue<TexEnvCombine::OperandParam> opParamInit[] =
{
    {"src-color", TexEnvCombine::SRC_COLOR},
    {"one-minus-src-color", TexEnvCombine::ONE_MINUS_SRC_COLOR},
    {"src-alpha", TexEnvCombine::SRC_ALPHA},
    {"one-minus-src-alpha", TexEnvCombine::ONE_MINUS_SRC_ALPHA}
};

EffectPropertyMap<TexEnvCombine::OperandParam> operandParams(opParamInit);

TexEnvCombine* buildTexEnvCombine(Effect* effect, const SGPropertyNode* envProp,
                                  const SGReaderWriterOptions* options)
{
    if (!isAttributeActive(effect, envProp))
        return 0;
    TexEnvCombine* result = new TexEnvCombine;
    const SGPropertyNode* p = 0;
    if ((p = getEffectPropertyChild(effect, envProp, "combine-rgb"))) {
        TexEnvCombine::CombineParam crgb = TexEnvCombine::MODULATE;
        findAttr(combineParams, p, crgb);
        result->setCombine_RGB(crgb);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "combine-alpha"))) {
        TexEnvCombine::CombineParam calpha = TexEnvCombine::MODULATE;
        findAttr(combineParams, p, calpha);
        result->setCombine_Alpha(calpha);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "source0-rgb"))) {
        TexEnvCombine::SourceParam source = TexEnvCombine::TEXTURE;
        findAttr(sourceParams, p, source);
        result->setSource0_RGB(source);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "source1-rgb"))) {
        TexEnvCombine::SourceParam source = TexEnvCombine::PREVIOUS;
        findAttr(sourceParams, p, source);
        result->setSource1_RGB(source);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "source2-rgb"))) {
        TexEnvCombine::SourceParam source = TexEnvCombine::CONSTANT;
        findAttr(sourceParams, p, source);
        result->setSource2_RGB(source);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "source0-alpha"))) {
        TexEnvCombine::SourceParam source = TexEnvCombine::TEXTURE;
        findAttr(sourceParams, p, source);
        result->setSource0_Alpha(source);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "source1-alpha"))) {
        TexEnvCombine::SourceParam source = TexEnvCombine::PREVIOUS;
        findAttr(sourceParams, p, source);
        result->setSource1_Alpha(source);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "source2-alpha"))) {
        TexEnvCombine::SourceParam source = TexEnvCombine::CONSTANT;
        findAttr(sourceParams, p, source);
        result->setSource2_Alpha(source);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "operand0-rgb"))) {
        TexEnvCombine::OperandParam op = TexEnvCombine::SRC_COLOR;
        findAttr(operandParams, p, op);
        result->setOperand0_RGB(op);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "operand1-rgb"))) {
        TexEnvCombine::OperandParam op = TexEnvCombine::SRC_COLOR;
        findAttr(operandParams, p, op);
        result->setOperand1_RGB(op);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "operand2-rgb"))) {
        TexEnvCombine::OperandParam op = TexEnvCombine::SRC_ALPHA;
        findAttr(operandParams, p, op);
        result->setOperand2_RGB(op);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "operand0-alpha"))) {
        TexEnvCombine::OperandParam op = TexEnvCombine::SRC_ALPHA;
        findAttr(operandParams, p, op);
        result->setOperand0_Alpha(op);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "operand1-alpha"))) {
        TexEnvCombine::OperandParam op = TexEnvCombine::SRC_ALPHA;
        findAttr(operandParams, p, op);
        result->setOperand1_Alpha(op);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "operand2-alpha"))) {
        TexEnvCombine::OperandParam op = TexEnvCombine::SRC_ALPHA;
        findAttr(operandParams, p, op);
        result->setOperand2_Alpha(op);
    }
    if ((p = getEffectPropertyChild(effect, envProp, "scale-rgb"))) {
        result->setScale_RGB(p->getValue<float>());
    }
    if ((p = getEffectPropertyChild(effect, envProp, "scale-alpha"))) {
        result->setScale_Alpha(p->getValue<float>());
    }
#if 0
    if ((p = getEffectPropertyChild(effect, envProp, "constant-color"))) {
        SGVec4d color = p->getValue<SGVec4d>();
        result->setConstantColor(toOsg(color));
    } else if ((p = getEffectPropertyChild(effect, envProp,
                                           "light-direction"))) {
        SGVec3d direction = p->getValue<SGVec3d>();
        result->setConstantColorAsLightDirection(toOsg(direction));
    }
#endif
    const SGPropertyNode* colorNode = envProp->getChild("constant-color");
    if (colorNode) {
    	initFromParameters(effect, colorNode, result,
                           &TexEnvCombine::setConstantColor, colorFields,
                           options);
    }
    return result;
}

EffectNameValue<TexGen::Mode> tgenModeInit[] =
{
    { "object-linear", TexGen::OBJECT_LINEAR},
    { "eye-linear", TexGen::EYE_LINEAR},
    { "sphere-map", TexGen::SPHERE_MAP},
    { "normal-map", TexGen::NORMAL_MAP},
    { "reflection-map", TexGen::REFLECTION_MAP}
};

EffectPropertyMap<TexGen::Mode> tgenModes(tgenModeInit);

EffectNameValue<TexGen::Coord> tgenCoordInit[] =
{
    {"s", TexGen::S},
    {"t", TexGen::T},
    {"r", TexGen::R},
    {"q", TexGen::Q}
};

EffectPropertyMap<TexGen::Coord> tgenCoords(tgenCoordInit);

TexGen* buildTexGen(Effect* effect, const SGPropertyNode* tgenProp)
{
    if (!isAttributeActive(effect, tgenProp))
        return 0;
    TexGen* result = new TexGen;
    TexGen::Mode mode = TexGen::OBJECT_LINEAR;
    findAttr(tgenModes, getEffectPropertyChild(effect, tgenProp, "mode"), mode);
    result->setMode(mode);
    const SGPropertyNode* planesNode = tgenProp->getChild("planes");
    if (planesNode) {
        for (int i = 0; i < planesNode->nChildren(); ++i) {
            const SGPropertyNode* planeNode = planesNode->getChild(i);
            TexGen::Coord coord;
            findAttr(tgenCoords, planeNode->getName(), coord);
            const SGPropertyNode* realNode
                = getEffectPropertyNode(effect, planeNode);
            SGVec4d plane = realNode->getValue<SGVec4d>();
            result->setPlane(coord, toOsg(plane));
        }
    }
    return result;
}

bool makeTextureParameters(SGPropertyNode* paramRoot, const StateSet* ss)
{
    SGPropertyNode* texUnit = makeChild(paramRoot, "texture");
    const Texture* tex = getStateAttribute<Texture>(0, ss);
    const Texture2D* texture = dynamic_cast<const Texture2D*>(tex);
    makeChild(texUnit, "unit")->setValue(0);
    if (!tex) {
        // The default shader-based technique ignores active
        makeChild(texUnit, "active")->setValue(false);
        return false;
    }
    const Image* image = texture->getImage();
    string imageName;
    if (image) {
        imageName = image->getFileName();
    } else {
        makeChild(texUnit, "active")->setValue(false);
        makeChild(texUnit, "type")->setValue("white");
        return false;
    }
    makeChild(texUnit, "active")->setValue(true);
    makeChild(texUnit, "type")->setValue("2d");
    string filter = findName(filterModes,
                             texture->getFilter(Texture::MIN_FILTER));
    string magFilter = findName(filterModes,
                             texture->getFilter(Texture::MAG_FILTER));
    string wrapS = findName(wrapModes, texture->getWrap(Texture::WRAP_S));
    string wrapT = findName(wrapModes, texture->getWrap(Texture::WRAP_T));
    string wrapR = findName(wrapModes, texture->getWrap(Texture::WRAP_R));
    makeChild(texUnit, "image")->setStringValue(imageName);
    makeChild(texUnit, "filter")->setStringValue(filter);
    makeChild(texUnit, "mag-filter")->setStringValue(magFilter);
    makeChild(texUnit, "wrap-s")->setStringValue(wrapS);
    makeChild(texUnit, "wrap-t")->setStringValue(wrapT);
    makeChild(texUnit, "wrap-r")->setStringValue(wrapR);
    return true;
}

class GBufferBuilder : public TextureBuilder
{
public:
    GBufferBuilder() {}
    Texture* build(Effect* effect, Pass* pass, const SGPropertyNode*,
                   const SGReaderWriterOptions* options);
private:
    string buffer;
};

class BufferNameChangeListener : public SGPropertyChangeListener,
      public DeferredPropertyListener {
public:
    BufferNameChangeListener(Pass* p, int u, const std::string& pn) : pass(p), unit(u)
    {
        propName = new std::string(pn);
    }
    ~BufferNameChangeListener()
    {
        delete propName;
        propName = 0;
    }
    void valueChanged(SGPropertyNode* node)
    {
        const char* buffer = node->getStringValue();
        pass->setBufferUnit(unit, buffer);
    }
    void activate(SGPropertyNode* propRoot)
    {
        SGPropertyNode* listenProp = makeNode(propRoot, *propName);
        delete propName;
        propName = 0;
        if (listenProp)
            listenProp->addChangeListener(this, true);
    }

private:
    ref_ptr<Pass> pass;
    int unit;
    std::string* propName;
};

Texture* GBufferBuilder::build(Effect* effect, Pass* pass, const SGPropertyNode* prop,
                                    const SGReaderWriterOptions* options)
{
    int unit = 0;
    const SGPropertyNode* pUnit = prop->getChild("unit");
    if (pUnit) {
        unit = pUnit->getValue<int>();
    } else {
        SG_LOG(SG_INPUT, SG_ALERT, "no texture unit");
    }
    const SGPropertyNode* nameProp = getEffectPropertyChild(effect, prop,
                                                            "name");
    if (!nameProp)
        return 0;

    if (nameProp->nChildren() == 0) {
        buffer = nameProp->getStringValue();
        pass->setBufferUnit( unit, buffer );
    } else {
        std::string propName = getGlobalProperty(nameProp, options);
        BufferNameChangeListener* listener = new BufferNameChangeListener(pass, unit, propName);
        effect->addDeferredPropertyListener(listener);
    }

    // Return white for now. Would be overridden in Technique::ProcessDrawable
    return StateAttributeFactory::instance()->getWhiteTexture();
}

namespace
{
    TextureBuilder::Registrar installBuffer("buffer", new GBufferBuilder);
}

}
