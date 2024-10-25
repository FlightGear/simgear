// Copyright (C) 2007 Tim Moore
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <simgear_config.h>
#include "StateAttributeFactory.hxx"

#include <osg/AlphaFunc>
#include <osg/Array>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Depth>
#include <osg/ShadeModel>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TexEnv>

#include <osg/Image>

#include <simgear/debug/logstream.hxx>

#include "Noise.hxx"

using namespace osg;

namespace simgear
{
StateAttributeFactory::StateAttributeFactory()
{
    // Standard blend function
    _standardBlendFunc = new BlendFunc;
    _standardBlendFunc->setSource(BlendFunc::SRC_ALPHA);
    _standardBlendFunc->setDestination(BlendFunc::ONE_MINUS_SRC_ALPHA);
    _standardBlendFunc->setDataVariance(Object::STATIC);

    // White color
    _white = new Vec4Array(1);
    (*_white)[0].set(1.0f, 1.0f, 1.0f, 1.0f);
    _white->setDataVariance(Object::STATIC);

    // White texture
    osg::ref_ptr<osg::Image> whiteImage = new osg::Image;
    whiteImage->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    unsigned char* whiteImageBytes = whiteImage->data();
    whiteImageBytes[0] = 255;
    whiteImageBytes[1] = 255;
    whiteImageBytes[2] = 255;
    whiteImageBytes[3] = 255;
    _whiteTexture = new osg::Texture2D;
    _whiteTexture->setImage(whiteImage);
    _whiteTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _whiteTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _whiteTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _whiteTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _whiteTexture->setDataVariance(osg::Object::STATIC);

    // Transparent texture
    osg::ref_ptr<osg::Image> transparentImage = new osg::Image;
    transparentImage->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    unsigned char* transparentImageBytes = transparentImage->data();
    transparentImageBytes[0] = 255;
    transparentImageBytes[1] = 255;
    transparentImageBytes[2] = 255;
    transparentImageBytes[3] = 0;
    _transparentTexture = new osg::Texture2D;
    _transparentTexture->setImage(transparentImage);
    _transparentTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _transparentTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _transparentTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _transparentTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _transparentTexture->setDataVariance(osg::Object::STATIC);

    // And a null normal map texture
    osg::ref_ptr<osg::Image> nullNormalMapImage = new osg::Image;
    nullNormalMapImage->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    unsigned char* nullNormalMapBytes = nullNormalMapImage->data();
    nullNormalMapBytes[0] = 128;
    nullNormalMapBytes[1] = 128;
    nullNormalMapBytes[2] = 255;
    nullNormalMapBytes[3] = 255;
    _nullNormalmapTexture = new osg::Texture2D;
    _nullNormalmapTexture->setImage(nullNormalMapImage);
    _nullNormalmapTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
    _nullNormalmapTexture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
    _nullNormalmapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _nullNormalmapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _nullNormalmapTexture->setDataVariance(osg::Object::STATIC);

    // Cull front facing polygons
    _cullFaceFront = new CullFace(CullFace::FRONT);
    _cullFaceFront->setDataVariance(Object::STATIC);

    // Cull back facing polygons
    _cullFaceBack = new CullFace(CullFace::BACK);
    _cullFaceBack->setDataVariance(Object::STATIC);

    // Standard depth function
    _standardDepth = new Depth(Depth::LESS, 0.0, 1.0, true);
    _standardDepth->setDataVariance(Object::STATIC);

    // Standard depth function with writes disabled
    _standardDepthWritesDisabled = new Depth(Depth::LESS, 0.0, 1.0, false);
    _standardDepthWritesDisabled->setDataVariance(Object::STATIC);
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

    SG_LOG(SG_TERRAIN, SG_BULK, "creating 3D noise texture... ");

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

    SG_LOG(SG_TERRAIN, SG_BULK, "DONE");

    return image;
}

osg::Texture3D* StateAttributeFactory::getNoiseTexture(int size)
{
    std::lock_guard<std::mutex> lock(StateAttributeFactory::_noise_mutex); // Lock the _noises for this scope
    NoiseMap::iterator itr = _noises.find(size);
    if (itr != _noises.end())
        return itr->second.get();
    Texture3D* noiseTexture = new osg::Texture3D;
    noiseTexture->setFilter(osg::Texture3D::MIN_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setFilter(osg::Texture3D::MAG_FILTER, osg::Texture3D::LINEAR);
    noiseTexture->setWrap(osg::Texture3D::WRAP_S, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_T, osg::Texture3D::REPEAT);
    noiseTexture->setWrap(osg::Texture3D::WRAP_R, osg::Texture3D::REPEAT);
    noiseTexture->setImage( make3DNoiseImage(size) );
    _noises.insert(std::make_pair(size, noiseTexture));
    return noiseTexture;
}

// anchor the destructor into this file, to avoid ref_ptr warnings
StateAttributeFactory::~StateAttributeFactory()
{
  
}

} // namespace simgear
