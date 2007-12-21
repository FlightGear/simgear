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

#include <OpenThreads/ScopedLock>

#include <osg/Image>
#include "StateAttributeFactory.hxx"

using namespace osg;

namespace simgear
{
StateAttributeFactory::StateAttributeFactory()
{
    _standardAlphaFunc = new AlphaFunc;
    _standardAlphaFunc->setFunction(osg::AlphaFunc::GREATER);
    _standardAlphaFunc->setReferenceValue(0.01);
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
}

osg::ref_ptr<StateAttributeFactory> StateAttributeFactory::_theInstance;
OpenThreads::Mutex StateAttributeFactory::_instanceMutex;

StateAttributeFactory* StateAttributeFactory::instance()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_instanceMutex);
    if (!_theInstance.valid()) {
        _theInstance = new StateAttributeFactory;
    }
    return _theInstance.get();
}
}
