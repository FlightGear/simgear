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

#include <osg/TexEnv>
#include <osg/TexEnvCombine>
#include <osg/TexGen>
#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureRectangle>
#include <osg/TextureCubeMap>
#include <osgDB/FileUtils>

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include <simgear/scene/model/SGReaderWriterXMLOptions.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/structure/OSGUtils.hxx>

#include "Noise.hxx"

namespace simgear
{
using namespace std;
using namespace osg;

using namespace effect;

TexEnvCombine* buildTexEnvCombine(Effect* effect,
                                  const SGPropertyNode* envProp,
                                  const SGReaderWriterXMLOptions* options);
TexGen* buildTexGen(Effect* Effect, const SGPropertyNode* tgenProp);

// Hack to force inclusion of TextureBuilder.cxx in library
osg::Texture* TextureBuilder::buildFromType(Effect* effect, const string& type,
                                            const SGPropertyNode*props,
                                            const SGReaderWriterXMLOptions*
                                            options)
{
    return EffectBuilder<Texture>::buildFromType(effect, type, props, options);
}

typedef boost::tuple<string, Texture::FilterMode, Texture::FilterMode,
                     Texture::WrapMode, Texture::WrapMode, Texture::WrapMode,
                     string, MipMapTuple> TexTuple;

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
                                        const SGReaderWriterXMLOptions* options)
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
                unit = boost::lexical_cast<int>(pName->getStringValue());
            } catch (boost::bad_lexical_cast& lex) {
                SG_LOG(SG_INPUT, SG_ALERT, "can't decode name as texture unit "
                       << lex.what());
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
        texture = TextureBuilder::buildFromType(effect, type, prop,
                                                options);
    }
    catch (BuilderException& ) {
        SG_LOG(SG_INPUT, SG_ALERT, "No image file, "
            << "maybe the reader did not set the filename attribute, "
            << "using white on " << pass->getName());
        texture = StateAttributeFactory::instance()->getWhiteTexture();
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
                      const SGReaderWriterXMLOptions* options,
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
    if (pImage)
        imageName = pImage->getStringValue();
    string absFileName = osgDB::findDataFile(imageName, options);

    const SGPropertyNode* pMipmapControl
        = getEffectPropertyChild(effect, props, "mipmap-control");
    MipMapTuple mipmapFunctions( AUTOMATIC, AUTOMATIC, AUTOMATIC, AUTOMATIC ); 
    if ( pMipmapControl )
        mipmapFunctions = makeMipMapTuple(effect, pMipmapControl, options);

    return TexTuple(absFileName, minFilter, magFilter, sWrap, tWrap, rWrap,
                    texType, mipmapFunctions);
}

void setAttrs(const TexTuple& attrs, Texture* tex,
              const SGReaderWriterXMLOptions* options)
{
    const string& imageName = attrs.get<0>();
    if (imageName.empty()) {
        throw BuilderException("no image file");
    } else {
        osgDB::ReaderWriter::ReadResult result
            = osgDB::Registry::instance()->readImage(imageName, options);
        if (result.success()) {
            osg::ref_ptr<osg::Image> image = result.getImage();
            image = computeMipmap( image, attrs.get<7>() );
            tex->setImage(GL_FRONT_AND_BACK, image);
            int s = image->s();
            int t = image->t();
            if (s <= t && 32 <= s) {
                SGSceneFeatures::instance()->setTextureCompression(tex);
            } else if (t < s && 32 <= t) {
                SGSceneFeatures::instance()->setTextureCompression(tex);
            }
            tex->setMaxAnisotropy(SGSceneFeatures::instance()
                                  ->getTextureFilter());
        } else {
            SG_LOG(SG_INPUT, SG_ALERT, "failed to load effect texture file "
                   << imageName);
        }
    }
    // texture->setDataVariance(osg::Object::STATIC);
    tex->setFilter(Texture::MIN_FILTER, attrs.get<1>());
    tex->setFilter(Texture::MAG_FILTER, attrs.get<2>());
    tex->setWrap(Texture::WRAP_S, attrs.get<3>());
    tex->setWrap(Texture::WRAP_T, attrs.get<4>());
    tex->setWrap(Texture::WRAP_R, attrs.get<5>());
}
}

template<typename T>
class TexBuilder : public TextureBuilder
{
public:
    TexBuilder(const string& texType) : _type(texType) {}
    Texture* build(Effect* effect, const SGPropertyNode*,
                   const SGReaderWriterXMLOptions* options);
protected:
    typedef map<TexTuple, ref_ptr<T> > TexMap;
    TexMap texMap;
    const string _type;
};

template<typename T>
Texture* TexBuilder<T>::build(Effect* effect, const SGPropertyNode* props,
                              const SGReaderWriterXMLOptions* options)
{
    TexTuple attrs = makeTexTuple(effect, props, options, _type);
    typename TexMap::iterator itr = texMap.find(attrs);
    if (itr != texMap.end())
        return itr->second.get();
    T* tex = new T;
    setAttrs(attrs, tex, options);
    texMap.insert(make_pair(attrs, tex));
    return tex;
}


namespace
{
TextureBuilder::Registrar install1D("1d", new TexBuilder<Texture1D>("1d"));
TextureBuilder::Registrar install2D("2d", new TexBuilder<Texture2D>("2d"));
TextureBuilder::Registrar install3D("3d", new TexBuilder<Texture3D>("3d"));
}

class WhiteTextureBuilder : public TextureBuilder
{
public:
    Texture* build(Effect* effect, const SGPropertyNode*,
                   const SGReaderWriterXMLOptions* options);
};

Texture* WhiteTextureBuilder::build(Effect* effect, const SGPropertyNode*,
                                    const SGReaderWriterXMLOptions* options)
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
    Texture* build(Effect* effect, const SGPropertyNode*,
                   const SGReaderWriterXMLOptions* options);
};

Texture* TransparentTextureBuilder::build(Effect* effect, const SGPropertyNode*,
                                    const SGReaderWriterXMLOptions* options)
{
    return StateAttributeFactory::instance()->getTransparentTexture();
}

namespace
{
TextureBuilder::Registrar installTransparent("transparent",
                                             new TransparentTextureBuilder);
}

osg::Image* make3DNoiseImage(int texSize)
{
    osg::Image* image = new osg::Image;
    image->setImage(texSize, texSize, texSize,
                    4, GL_RGBA, GL_UNSIGNED_BYTE,
                    new unsigned char[4 * texSize * texSize * texSize],
                    osg::Image::USE_NEW_DELETE);

    const int startFrequency = 4;
    const int numOctaves = 4;

    int f, i, j, k, inc;
    double ni[3];
    double inci, incj, inck;
    int frequency = startFrequency;
    GLubyte *ptr;
    double amp = 0.5;

    osg::notify(osg::WARN) << "creating 3D noise texture... ";

    for (f = 0, inc = 0; f < numOctaves; ++f, frequency *= 2, ++inc, amp *= 0.5)
    {
        SetNoiseFrequency(frequency);
        ptr = image->data();
        ni[0] = ni[1] = ni[2] = 0;

        inci = 1.0 / (texSize / frequency);
        for (i = 0; i < texSize; ++i, ni[0] += inci)
        {
            incj = 1.0 / (texSize / frequency);
            for (j = 0; j < texSize; ++j, ni[1] += incj)
            {
                inck = 1.0 / (texSize / frequency);
                for (k = 0; k < texSize; ++k, ni[2] += inck, ptr += 4)
                {
                    *(ptr+inc) = (GLubyte) (((noise3(ni) + 1.0) * amp) * 128.0);
                }
            }
        }
    }

    osg::notify(osg::WARN) << "DONE" << std::endl;
    return image;
}

class NoiseBuilder : public TextureBuilder
{
public:
    Texture* build(Effect* effect, const SGPropertyNode*,
                   const SGReaderWriterXMLOptions* options);
protected:
    typedef map<int, ref_ptr<Texture3D> > NoiseMap;
    NoiseMap _noises;
};

Texture* NoiseBuilder::build(Effect* effect, const SGPropertyNode* props,
                             const SGReaderWriterXMLOptions* options)
{
    int texSize = 64;
    const SGPropertyNode* sizeProp = getEffectPropertyChild(effect, props,
                                                            "size");
    if (sizeProp)
        texSize = sizeProp->getValue<int>();
    NoiseMap::iterator itr = _noises.find(texSize);
    if (itr != _noises.end())
        return itr->second.get();
    Texture3D* noiseTexture = new osg::Texture3D;
    noiseTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::REPEAT);
    noiseTexture->setImage( make3DNoiseImage(texSize) );
    _noises.insert(make_pair(texSize, noiseTexture));
    return noiseTexture;
}

namespace
{
TextureBuilder::Registrar installNoise("noise", new NoiseBuilder);
}



// Image names for all sides
typedef boost::tuple<string, string, string, string, string, string> CubeMapTuple;

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
    Texture* build(Effect* effect, const SGPropertyNode*,
                   const SGReaderWriterXMLOptions* options);
protected:
    typedef map<CubeMapTuple, ref_ptr<TextureCubeMap> > CubeMap;
    typedef map<string, ref_ptr<TextureCubeMap> > CrossCubeMap;
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


Texture* CubeMapBuilder::build(Effect* effect, const SGPropertyNode* props,
                               const SGReaderWriterXMLOptions* options)
{
    // First check that there is a <images> tag
    const SGPropertyNode* texturesProp = getEffectPropertyChild(effect, props, "images");
    const SGPropertyNode* crossProp = getEffectPropertyChild(effect, props, "image");
    if (!texturesProp && !crossProp) {
        throw BuilderException("no images defined for cube map");
        return NULL; // This is redundant
    }

    // Using 6 separate images
    if(texturesProp) {

        SG_LOG(SG_INPUT, SG_DEBUG, "try 6 images ");

        CubeMapTuple _tuple = makeCubeMapTuple(effect, texturesProp);

        CubeMap::iterator itr = _cubemaps.find(_tuple);
        if (itr != _cubemaps.end())
            return itr->second.get();

        TextureCubeMap* cubeTexture = new osg::TextureCubeMap;

        // TODO: Read these from effect file? Maybe these are sane for all cuebmaps?
        cubeTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
        cubeTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture::LINEAR);
        cubeTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        cubeTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        cubeTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture::CLAMP_TO_EDGE);

        osgDB::ReaderWriter::ReadResult result =
            osgDB::Registry::instance()->readImage(_tuple.get<0>(), options);
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::POSITIVE_X, image);
        }
        result = osgDB::Registry::instance()->readImage(_tuple.get<1>(), options);
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::NEGATIVE_X, image);
        }
        result = osgDB::Registry::instance()->readImage(_tuple.get<2>(), options);
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::POSITIVE_Y, image);
        }
        result = osgDB::Registry::instance()->readImage(_tuple.get<3>(), options);
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::NEGATIVE_Y, image);
        }
        result = osgDB::Registry::instance()->readImage(_tuple.get<4>(), options);
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::POSITIVE_Z, image);
        }
        result = osgDB::Registry::instance()->readImage(_tuple.get<5>(), options);
        if(result.success()) {
            osg::Image* image = result.getImage();
            cubeTexture->setImage(TextureCubeMap::NEGATIVE_Z, image);
        }

        _cubemaps[_tuple] = cubeTexture;

        return cubeTexture;
    }


    // Using 1 cross image
    else if(crossProp) {
        SG_LOG(SG_INPUT, SG_DEBUG, "try cross map ");

        std::string texname = crossProp->getStringValue();

        // Try to find existing cube map
        CrossCubeMap::iterator itr = _crossmaps.find(texname);
        if (itr != _crossmaps.end())
            return itr->second.get();

        osgDB::ReaderWriter::ReadResult result =
            osgDB::Registry::instance()->readImage(texname, options);
        if(result.success()) {
            osg::Image* image = result.getImage();
            image->flipVertical();   // Seems like the image coordinates are somewhat funny, flip to get better ones

            //cubeTexture->setResizeNonPowerOfTwoHint(false);

            // Size of a single image, 4 rows and 3 columns
            int width = image->s() / 3;
            int height = image->t() / 4;
            int depth = image->r();

            TextureCubeMap* cubeTexture = new osg::TextureCubeMap;

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

            _crossmaps[texname] = cubeTexture;

            return cubeTexture;

        } else {
            throw BuilderException("Could not load cube cross");
        }
    }

    return NULL;
}

namespace {
TextureBuilder::Registrar installCubeMap("cubemap", new CubeMapBuilder);
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
                                  const SGReaderWriterXMLOptions* options)
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
    if (colorNode)
        initFromParameters(effect, colorNode, result,
                           &TexEnvCombine::setConstantColor, colorFields,
                           options);
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

}
