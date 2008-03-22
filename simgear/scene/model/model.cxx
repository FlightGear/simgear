// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#include <simgear_config.h>
#endif

#include <osg/ref_ptr>
#include <osgDB/ReadFile>
#include <osgDB/SharedStateManager>

#include <simgear/scene/util/SGSceneFeatures.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>

#include "model.hxx"

SG_USING_STD(vector);

osg::Texture2D*
SGLoadTexture2D(bool staticTexture, const std::string& path,
                const osgDB::ReaderWriter::Options* options,
                bool wrapu, bool wrapv, int)
{
  osg::Image* image;
  if (options)
    image = osgDB::readImageFile(path, options);
  else
    image = osgDB::readImageFile(path);
  osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
  texture->setImage(image);
  if (staticTexture)
    texture->setDataVariance(osg::Object::STATIC);
  if (wrapu)
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
  else
    texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP);
  if (wrapv)
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
  else
    texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP);

  if (image) {
    int s = image->s();
    int t = image->t();

    if (s <= t && 32 <= s) {
      SGSceneFeatures::instance()->setTextureCompression(texture.get());
    } else if (t < s && 32 <= t) {
      SGSceneFeatures::instance()->setTextureCompression(texture.get());
    }
  }

  // Make sure the texture is shared if we already have the same texture
  // somewhere ...
  {
    osg::ref_ptr<osg::Node> tmpNode = new osg::Node;
    osg::StateSet* stateSet = tmpNode->getOrCreateStateSet();
    stateSet->setTextureAttribute(0, texture.get());

    // OSGFIXME: don't forget that mutex here
    osgDB::Registry* registry = osgDB::Registry::instance();
    registry->getSharedStateManager()->share(tmpNode.get(), 0);

    // should be the same, but be paranoid ...
    stateSet = tmpNode->getStateSet();
    osg::StateAttribute* stateAttr;
    stateAttr = stateSet->getTextureAttribute(0, osg::StateAttribute::TEXTURE);
    osg::Texture2D* texture2D = dynamic_cast<osg::Texture2D*>(stateAttr);
    if (texture2D)
      texture = texture2D;
  }

  return texture.release();
}

// end of model.cxx
