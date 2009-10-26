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

#include <simgear/math/SGMath.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>

#include "model.hxx"

using std::vector;

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

  return texture.release();
}

// end of model.cxx
