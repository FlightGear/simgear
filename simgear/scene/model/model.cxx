// model.cxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifdef HAVE_CONFIG_H
#include <simgear_config.h>
#endif

#include <utility>

#include <boost/foreach.hpp>

#include <osg/ref_ptr>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/ReadFile>
#include <osgDB/SharedStateManager>

#include <simgear/math/SGMath.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/util/CopyOp.hxx>
#include <simgear/scene/util/SplicingVisitor.hxx>


#include <simgear/structure/exception.hxx>
#include <simgear/structure/Singleton.hxx>
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
using namespace std;
using namespace osg;
using simgear::CopyOp;

Node* copyModel(Node* model)
{
    const CopyOp::CopyFlags flags = (CopyOp::DEEP_COPY_ALL
                                     & ~CopyOp::DEEP_COPY_TEXTURES
                                     & ~CopyOp::DEEP_COPY_IMAGES
                                     & ~CopyOp::DEEP_COPY_STATESETS
                                     & ~CopyOp::DEEP_COPY_STATEATTRIBUTES
                                     & ~CopyOp::DEEP_COPY_ARRAYS
                                     & ~CopyOp::DEEP_COPY_PRIMITIVES
                                     // This will preserve display lists ...
                                     & ~CopyOp::DEEP_COPY_DRAWABLES
                                     & ~CopyOp::DEEP_COPY_SHAPES);
    return (CopyOp(flags))(model);
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

namespace
{
class MakeEffectVisitor : public SplicingVisitor
{
public:
    typedef std::map<string, SGPropertyNode_ptr> EffectMap;
    using SplicingVisitor::apply;
    MakeEffectVisitor(const osgDB::ReaderWriter::Options* options = 0)
        : _options(options)
    {
    }
    virtual void apply(osg::Group& node);
    virtual void apply(osg::Geode& geode);
    EffectMap& getEffectMap() { return _effectMap; }
    const EffectMap& getEffectMap() const { return _effectMap; }
    void setDefaultEffect(SGPropertyNode* effect)
    {
        _currentEffectParent = effect;
    }
    SGPropertyNode* getDefaultEffect() { return _currentEffectParent; }
protected:
    EffectMap _effectMap;
    SGPropertyNode_ptr _currentEffectParent;
    osg::ref_ptr<const osgDB::ReaderWriter::Options> _options;
};

void MakeEffectVisitor::apply(osg::Group& node)
{
    SGPropertyNode_ptr savedEffectRoot;
    const string& nodeName = node.getName();
    bool restoreEffect = false;
    if (!nodeName.empty()) {
        EffectMap::iterator eitr = _effectMap.find(nodeName);
        if (eitr != _effectMap.end()) {
            savedEffectRoot = _currentEffectParent;
            _currentEffectParent = eitr->second;
            restoreEffect = true;
        }
    }
    SplicingVisitor::apply(node);
    // If a new node was created, copy the user data too.
    ref_ptr<SGSceneUserData> userData = SGSceneUserData::getSceneUserData(&node);
    if (userData.valid() && _childStack.back().back().get() != &node)
        _childStack.back().back()->setUserData(new SGSceneUserData(*userData));
    if (restoreEffect)
        _currentEffectParent = savedEffectRoot;
}

void MakeEffectVisitor::apply(osg::Geode& geode)
{
    if (pushNode(getNewNode(geode)))
        return;
    osg::StateSet* ss = geode.getStateSet();
    if (!ss) {
        pushNode(&geode);
        return;
    }
    SGPropertyNode_ptr ssRoot = new SGPropertyNode;
    makeParametersFromStateSet(ssRoot, ss);
    SGPropertyNode_ptr effectRoot = new SGPropertyNode;
    effect::mergePropertyTrees(effectRoot, ssRoot, _currentEffectParent);
    Effect* effect = makeEffect(effectRoot, true, _options);
    EffectGeode* eg = dynamic_cast<EffectGeode*>(&geode);
    if (eg) {
        eg->setEffect(effect);
    } else {
        eg = new EffectGeode;
        eg->setEffect(effect);
        ref_ptr<SGSceneUserData> userData = SGSceneUserData::getSceneUserData(&geode);
        if (userData.valid())
            eg->setUserData(new SGSceneUserData(*userData));
        for (int i = 0; i < geode.getNumDrawables(); ++i)
            eg->addDrawable(geode.getDrawable(i));
    }
    pushResultNode(&geode, eg);

}

}

namespace
{
class DefaultEffect : public simgear::Singleton<DefaultEffect>
{
public:
    DefaultEffect()
    {
        _effect = new SGPropertyNode;
        makeChild(_effect.ptr(), "inherits-from")
            ->setStringValue("Effects/model-default");
    }
    virtual ~DefaultEffect() {}
    SGPropertyNode* getEffect() { return _effect.ptr(); }
protected:
    SGPropertyNode_ptr _effect;
};
}

ref_ptr<Node> instantiateEffects(osg::Node* modelGroup,
                                 PropertyList& effectProps,
                                 const osgDB::ReaderWriter::Options* options)
{
    SGPropertyNode_ptr defaultEffectPropRoot;
    MakeEffectVisitor visitor(options);
    MakeEffectVisitor::EffectMap& emap = visitor.getEffectMap();
    for (PropertyList::iterator itr = effectProps.begin(),
             end = effectProps.end();
         itr != end;
        ++itr)
    {
        SGPropertyNode_ptr configNode = *itr;
        std::vector<SGPropertyNode_ptr> objectNames =
            configNode->getChildren("object-name");
        SGPropertyNode* defaultNode = configNode->getChild("default");
        if (defaultNode && defaultNode->getValue<bool>())
            defaultEffectPropRoot = configNode;
        BOOST_FOREACH(SGPropertyNode_ptr objNameNode, objectNames) {
            emap.insert(make_pair(objNameNode->getStringValue(), configNode));
        }
        configNode->removeChild("default");
        configNode->removeChildren("object-name");
    }
    if (!defaultEffectPropRoot)
        defaultEffectPropRoot = DefaultEffect::instance()->getEffect();
    visitor.setDefaultEffect(defaultEffectPropRoot.ptr());
    modelGroup->accept(visitor);
    osg::NodeList& result = visitor.getResults();
    return ref_ptr<Node>(result[0].get());
}
}
// end of model.cxx
