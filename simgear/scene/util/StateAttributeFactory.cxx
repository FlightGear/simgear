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
    _standardAlphaFunc = new AlphaFunc;
    _standardAlphaFunc->setFunction(osg::AlphaFunc::GREATER);
    _standardAlphaFunc->setReferenceValue(0.01f);
    _standardAlphaFunc->setDataVariance(Object::STATIC);
    _smooth = new ShadeModel;
    _smooth->setMode(ShadeModel::SMOOTH);
    _smooth->setDataVariance(Object::STATIC);
    _flat = new ShadeModel(ShadeModel::FLAT);
    _flat->setDataVariance(Object::STATIC);
    _standardBlendFunc = new BlendFunc;
    _standardBlendFunc->setSource(BlendFunc::SRC_ALPHA);
    _standardBlendFunc->setDestination(BlendFunc::ONE_MINUS_SRC_ALPHA);
    _standardBlendFunc->setDataVariance(Object::STATIC);
    _standardTexEnv = new TexEnv;
    _standardTexEnv->setMode(TexEnv::MODULATE);
    _standardTexEnv->setDataVariance(Object::STATIC);
    osg::Image *dummyImage = new osg::Image;
    dummyImage->allocateImage(1, 1, 1, GL_LUMINANCE_ALPHA,
                              GL_UNSIGNED_BYTE);
    unsigned char* imageBytes = dummyImage->data(0, 0);
    imageBytes[0] = 255;
    imageBytes[1] = 255;
    _whiteTexture = new osg::Texture2D;
    _whiteTexture->setImage(dummyImage);
    _whiteTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _whiteTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _whiteTexture->setDataVariance(osg::Object::STATIC);
    // And now the transparent texture
    dummyImage = new osg::Image;
    dummyImage->allocateImage(1, 1, 1, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE);
    imageBytes = dummyImage->data(0, 0);
    imageBytes[0] = 255;
    imageBytes[1] = 0;
    _transparentTexture = new osg::Texture2D;
    _transparentTexture->setImage(dummyImage);
    _transparentTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _transparentTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _transparentTexture->setDataVariance(osg::Object::STATIC);
    // And a null normal map texture
    dummyImage = new osg::Image;
    dummyImage->allocateImage(1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE);
    imageBytes = dummyImage->data(0, 0);
    imageBytes[0] = 127;
    imageBytes[1] = 127;
    imageBytes[2] = 255;
    _nullNormalmapTexture = new osg::Texture2D;
    _nullNormalmapTexture->setImage(dummyImage);
    _nullNormalmapTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    _nullNormalmapTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    _nullNormalmapTexture->setDataVariance(osg::Object::STATIC);
    _white = new Vec4Array(1);
    (*_white)[0].set(1.0f, 1.0f, 1.0f, 1.0f);
    _white->setDataVariance(Object::STATIC);
    _cullFaceFront = new CullFace(CullFace::FRONT);
    _cullFaceFront->setDataVariance(Object::STATIC);
    _cullFaceBack = new CullFace(CullFace::BACK);
    _cullFaceBack->setDataVariance(Object::STATIC);
    _depthWritesDisabled = new Depth(Depth::LESS, 0.0, 1.0, false);
    _depthWritesDisabled->setDataVariance(Object::STATIC);
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
}
