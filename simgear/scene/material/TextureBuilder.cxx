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

#include <osg/Texture1D>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureRectangle>
#include <osgDB/FileUtils>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>

#include <simgear/math/SGMath.hxx>

#include "Noise.hxx"

namespace simgear
{
using namespace std;
using namespace osg;

// Hack to force inclusion of TextureBuilder.cxx in library
osg::Texture* TextureBuilder::buildFromType(Effect* effect, const string& type,
                                            const SGPropertyNode*props,
                                            const osgDB::ReaderWriter::Options*
                                            options)
{
    return EffectBuilder<Texture>::buildFromType(effect, type, props, options);
}

typedef boost::tuple<string, Texture::FilterMode, Texture::FilterMode,
                     Texture::WrapMode, Texture::WrapMode, Texture::WrapMode,
                     string> TexTuple;

namespace
{
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



TexTuple makeTexTuple(Effect* effect, const SGPropertyNode* props,
                      const osgDB::ReaderWriter::Options* options,
                      const string& texType)
{
    Texture::FilterMode minFilter = Texture::LINEAR_MIPMAP_LINEAR;
    findAttr(filterModes, getEffectPropertyChild(effect, props, "filter"),
             minFilter);
    Texture::FilterMode magFilter = Texture::LINEAR;
    findAttr(filterModes, getEffectPropertyChild(effect, props,
                                                 "mag-filter"),
             magFilter);
    const SGPropertyNode* pWrapS
        = getEffectPropertyChild(effect, props, "wrap-s");
    Texture::WrapMode sWrap = Texture::CLAMP;
    findAttr(wrapModes, pWrapS, sWrap);
    const SGPropertyNode* pWrapT
        = getEffectPropertyChild(effect, props, "wrap-t");
    Texture::WrapMode tWrap = Texture::CLAMP;
    findAttr(wrapModes, pWrapT, tWrap);
    const SGPropertyNode* pWrapR
        = getEffectPropertyChild(effect, props, "wrap-r");
    Texture::WrapMode rWrap = Texture::CLAMP;
    findAttr(wrapModes, pWrapR, rWrap);
    const SGPropertyNode* pImage
        = getEffectPropertyChild(effect, props, "image");
    string imageName;
    if (pImage)
        imageName = pImage->getStringValue();
    string absFileName = osgDB::findDataFile(imageName, options);
    return TexTuple(absFileName, minFilter, magFilter, sWrap, tWrap, rWrap,
                    texType);
}

void setAttrs(const TexTuple& attrs, Texture* tex,
              const osgDB::ReaderWriter::Options* options)
{
    const string& imageName = attrs.get<0>();
    if (imageName.empty()) {
        throw BuilderException("no image file");
    } else {
        osgDB::ReaderWriter::ReadResult result
            = osgDB::Registry::instance()->readImage(imageName, options);
        if (result.success()) {
            osg::Image* image = result.getImage();
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
                   const osgDB::ReaderWriter::Options* options);
protected:
    typedef map<TexTuple, ref_ptr<T> > TexMap;
    TexMap texMap;
    const string _type;
};

template<typename T>
Texture* TexBuilder<T>::build(Effect* effect, const SGPropertyNode* props,
                              const osgDB::ReaderWriter::Options* options)
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
                   const osgDB::ReaderWriter::Options* options);
};

Texture* WhiteTextureBuilder::build(Effect* effect, const SGPropertyNode*,
                                    const osgDB::ReaderWriter::Options* options)
{
    return StateAttributeFactory::instance()->getWhiteTexture();
}

namespace
{
TextureBuilder::Registrar installWhite("white", new WhiteTextureBuilder);
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
                   const osgDB::ReaderWriter::Options* options);
protected:
    typedef map<int, ref_ptr<Texture3D> > NoiseMap;
    NoiseMap _noises;
};

Texture* NoiseBuilder::build(Effect* effect, const SGPropertyNode* props,
                             const osgDB::ReaderWriter::Options* options)
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

}
