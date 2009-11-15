// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#include <simgear_config.h>
#endif

#include <utility>

#include <osg/ref_ptr>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>
#include <osgDB/SharedStateManager>

#include <simgear/math/SGMath.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>

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

namespace simgear
{
using namespace osg;

Node* copyModel(Node* model)
{
    CopyOp::CopyFlags flags = CopyOp::DEEP_COPY_ALL;
    flags &= ~CopyOp::DEEP_COPY_TEXTURES;
    flags &= ~CopyOp::DEEP_COPY_IMAGES;
    flags &= ~CopyOp::DEEP_COPY_STATESETS;
    flags &= ~CopyOp::DEEP_COPY_STATEATTRIBUTES;
    flags &= ~CopyOp::DEEP_COPY_ARRAYS;
    flags &= ~CopyOp::DEEP_COPY_PRIMITIVES;
    // This will preserve display lists ...
    flags &= ~CopyOp::DEEP_COPY_DRAWABLES;
    flags &= ~CopyOp::DEEP_COPY_SHAPES;
    return static_cast<Node*>(model->clone(CopyOp(flags)));
}

TextureUpdateVisitor::TextureUpdateVisitor(const osgDB::FilePathList& pathList) :
    NodeAndDrawableVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN),
    _pathList(pathList)
{
}

void TextureUpdateVisitor::apply(Node& node)
{
    StateSet* stateSet = cloneStateSet(node.getStateSet());
    if (stateSet)
        node.setStateSet(stateSet);
    traverse(node);
}

void TextureUpdateVisitor::apply(Drawable& drawable)
{
    StateSet* stateSet = cloneStateSet(drawable.getStateSet());
    if (stateSet)
        drawable.setStateSet(stateSet);
}

Texture2D* TextureUpdateVisitor::textureReplace(int unit, const StateAttribute* attr)
{
    using namespace osgDB;
    const Texture2D* texture = dynamic_cast<const Texture2D*>(attr);

    if (!texture)
        return 0;

    const Image* image = texture->getImage();
    const string* fullFilePath = 0;
    if (image) {
        // The currently loaded file name
        fullFilePath = &image->getFileName();

    } else {
        fullFilePath = &texture->getName();
    }
    // The short name
    string fileName = getSimpleFileName(*fullFilePath);
    if (fileName.empty())
        return 0;
    // The name that should be found with the current database path
    string fullLiveryFile = findFileInPath(fileName, _pathList);
    // If it is empty or they are identical then there is nothing to do
    if (fullLiveryFile.empty() || fullLiveryFile == *fullFilePath)
        return 0;
    Image* newImage = readImageFile(fullLiveryFile);
    if (!newImage)
        return 0;
    CopyOp copyOp(CopyOp::DEEP_COPY_ALL & ~CopyOp::DEEP_COPY_IMAGES);
    Texture2D* newTexture = static_cast<Texture2D*>(copyOp(texture));
    if (!newTexture) {
        return 0;
    } else {
        newTexture->setImage(newImage);
        return newTexture;
    }
}

StateSet* TextureUpdateVisitor::cloneStateSet(const StateSet* stateSet)
{
    typedef std::pair<int, Texture2D*> Tex2D;
    vector<Tex2D> newTextures;
    StateSet* result = 0;

    if (!stateSet)
        return 0;
    int numUnits = stateSet->getTextureAttributeList().size();
    if (numUnits > 0) {
        for (int i = 0; i < numUnits; ++i) {
            const StateAttribute* attr
                = stateSet->getTextureAttribute(i, StateAttribute::TEXTURE);
            Texture2D* newTexture = textureReplace(i, attr);
            if (newTexture)
                newTextures.push_back(Tex2D(i, newTexture));
        }
        if (!newTextures.empty()) {
            result = static_cast<StateSet*>(stateSet->clone(CopyOp()));
            for (vector<Tex2D>::iterator i = newTextures.begin();
                 i != newTextures.end();
                 ++i) {
                result->setTextureAttribute(i->first, i->second);
            }
        }
    }
    return result;
}

UserDataCopyVisitor::UserDataCopyVisitor() :
    NodeVisitor(NodeVisitor::NODE_VISITOR,
                NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}

void UserDataCopyVisitor::apply(Node& node)
{
    ref_ptr<SGSceneUserData> userData;
    userData = SGSceneUserData::getSceneUserData(&node);
    if (userData.valid()) {
        SGSceneUserData* newUserData  = new SGSceneUserData(*userData);
        newUserData->setVelocity(0);
        node.setUserData(newUserData);
    }
    node.traverse(*this);
}

}
// end of model.cxx
