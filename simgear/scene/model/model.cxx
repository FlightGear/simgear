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

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/util/CopyOp.hxx>
#include <simgear/scene/util/SplicingVisitor.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/Singleton.hxx>
#include <simgear/structure/exception.hxx>

#include "model.hxx"

using std::vector;

osg::Texture2D*
SGLoadTexture2D(bool staticTexture, const std::string& path,
                const osgDB::Options* options,
                bool wrapu, bool wrapv, int)
{
  osg::ref_ptr<osg::Image> image;
  if (options)
#if OSG_VERSION_LESS_THAN(3,4,0)
      image = osgDB::readImageFile(path, options);
#else
      image = osgDB::readRefImageFile(path, options);
#endif
  else
#if OSG_VERSION_LESS_THAN(3,4,0)
      image = osgDB::readImageFile(path);
#else
      image = osgDB::readRefImageFile(path);
#endif

  osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
  texture->setImage(image);
  texture->setMaxAnisotropy(SGSceneFeatures::instance()->getTextureFilter());

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
      SGSceneFeatures::instance()->applyTextureCompression(texture.get());
    } else if (t < s && 32 <= t) {
      SGSceneFeatures::instance()->applyTextureCompression(texture.get());
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

void TextureUpdateVisitor::apply(osg::Node& node)
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

#if OSG_VERSION_LESS_THAN(3,4,0)
    Image* newImage = readImageFile(fullLiveryFile);
#else
    osg::ref_ptr<Image> newImage = readRefImageFile(fullLiveryFile);
#endif
    if (!newImage)
        return 0;

    CopyOp copyOp(CopyOp::DEEP_COPY_ALL & ~CopyOp::DEEP_COPY_IMAGES);
    Texture2D* newTexture = static_cast<Texture2D*>(copyOp(texture));
    if (!newTexture)
        return 0;

    newTexture->setImage(newImage);
#if OSG_VERSION_LESS_THAN(3,4,0)
    if (newImage->valid())
#else
    if (newImage.valid())
#endif
    {
        newTexture->setMaxAnisotropy(SGSceneFeatures::instance()->getTextureFilter());
    }

    return newTexture;
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
    MakeEffectVisitor(const SGReaderWriterOptions* options = 0)
        : _options(options), _modelPath(SGPath{})
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

    void setModelPath(const SGPath& p)
    {
        _modelPath = p;
    }

protected:
    EffectMap _effectMap;
    SGPropertyNode_ptr _currentEffectParent;
    osg::ref_ptr<const SGReaderWriterOptions> _options;
    SGPath _modelPath;
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
    Effect* effect = makeEffect(effectRoot, true, _options.get(), _modelPath);
    EffectGeode* eg = dynamic_cast<EffectGeode*>(&geode);
    if (eg) {
        eg->setEffect(effect);
    } else {
        eg = new EffectGeode;
        eg->setEffect(effect);
        ref_ptr<SGSceneUserData> userData = SGSceneUserData::getSceneUserData(&geode);
        if (userData.valid())
            eg->setUserData(new SGSceneUserData(*userData));
        for (unsigned i = 0; i < geode.getNumDrawables(); ++i) {
            osg::Drawable *drawable = geode.getDrawable(i);
            eg->addDrawable(drawable);

            // Generate tangent vectors etc if needed
            osg::Geometry *geom = dynamic_cast<osg::Geometry*>(drawable);
            if(geom) eg->runGenerators(geom);
        }
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
                                 const SGReaderWriterOptions* options,
                                 const SGPath& modelPath)
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
        for (auto objNameNode : objectNames) {
            emap.insert(make_pair(objNameNode->getStringValue(), configNode));
        }
        configNode->removeChild("default");
        configNode->removeChildren("object-name");
    }
    if (!defaultEffectPropRoot)
        defaultEffectPropRoot = DefaultEffect::instance()->getEffect();
    visitor.setDefaultEffect(defaultEffectPropRoot.ptr());
    visitor.setModelPath(modelPath);
    modelGroup->accept(visitor);
    osg::NodeList& result = visitor.getResults();
    return ref_ptr<Node>(result[0].get());
}

ref_ptr<Node> instantiateMaterialEffects(osg::Node* modelGroup,
                                         const SGReaderWriterOptions* options,
                                         const SGPath& modelPath)
{

    SGPropertyNode_ptr effect;
    PropertyList effectProps;

    if (options->getMaterialLib()) {
      const SGGeod loc = SGGeod(options->getLocation());
      SGMaterialCache* matcache = options->getMaterialLib()->generateMatCache(loc);
      SGMaterial* mat = matcache->find(options->getMaterialName());
      delete matcache;

      if (mat) {
        effect = new SGPropertyNode();
        makeChild(effect, "inherits-from")->setStringValue(mat->get_effect_name());
      } else {
        effect = DefaultEffect::instance()->getEffect();
        SG_LOG( SG_TERRAIN, SG_ALERT, "Unable to get effect for " << options->getMaterialName());
        simgear::reportFailure(simgear::LoadFailure::NotFound, simgear::ErrorCode::LoadEffectsShaders,
                               "Unable to get effect for material:" + options->getMaterialName());
      }
    } else {
      effect = DefaultEffect::instance()->getEffect();
    }

    effect->addChild("default")->setBoolValue(true);
    effectProps.push_back(effect);
    return instantiateEffects(modelGroup, effectProps, options, modelPath);
}

}
// end of model.cxx
