#include "ModelRegistry.hxx"

#include <osg/observer_ptr>
#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/NodeCallback>
#include <osg/Switch>
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osgDB/Archive>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osgDB/SharedStateManager>
#include <osgUtil/Optimizer>

#include <simgear/scene/util/SGSceneFeatures.hxx>
#include <simgear/scene/util/SGStateAttributeVisitor.hxx>
#include <simgear/scene/util/SGTextureStateAttributeVisitor.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>

using namespace std;
using namespace osg;
using namespace osgDB;
using namespace simgear;

// Little helper class that holds an extra reference to a
// loaded 3d model.
// Since we clone all structural nodes from our 3d models,
// the database pager will only see one single reference to
// top node of the model and expire it relatively fast.
// We attach that extra reference to every model cloned from
// a base model in the pager. When that cloned model is deleted
// this extra reference is deleted too. So if there are no
// cloned models left the model will expire.
namespace {
class SGDatabaseReference : public Observer {
public:
  SGDatabaseReference(Referenced* referenced) :
    mReferenced(referenced)
  { }
  virtual void objectDeleted(void*)
  {
    mReferenced = 0;
  }
private:
  ref_ptr<Referenced> mReferenced;
};

// Visitor for 
class SGTextureUpdateVisitor : public SGTextureStateAttributeVisitor {
public:
  SGTextureUpdateVisitor(const FilePathList& pathList) :
    mPathList(pathList)
  { }
  Texture2D* textureReplace(int unit,
                            StateSet::RefAttributePair& refAttr)
  {
    Texture2D* texture;
    texture = dynamic_cast<Texture2D*>(refAttr.first.get());
    if (!texture)
      return 0;
    
    ref_ptr<Image> image = texture->getImage(0);
    if (!image)
      return 0;

    // The currently loaded file name
    string fullFilePath = image->getFileName();
    // The short name
    string fileName = getSimpleFileName(fullFilePath);
    // The name that should be found with the current database path
    string fullLiveryFile = findFileInPath(fileName, mPathList);
    // If they are identical then there is nothing to do
    if (fullLiveryFile == fullFilePath)
      return 0;

    image = readImageFile(fullLiveryFile);
    if (!image)
      return 0;

    CopyOp copyOp(CopyOp::DEEP_COPY_ALL & ~CopyOp::DEEP_COPY_IMAGES);
    texture = static_cast<Texture2D*>(copyOp(texture));
    if (!texture)
      return 0;
    texture->setImage(image.get());
    return texture;
  }
  virtual void apply(StateSet* stateSet)
  {
    if (!stateSet)
      return;

    // get a copy that we can safely modify the statesets values.
    StateSet::TextureAttributeList attrList;
    attrList = stateSet->getTextureAttributeList();
    for (unsigned unit = 0; unit < attrList.size(); ++unit) {
      StateSet::AttributeList::iterator i = attrList[unit].begin();
      while (i != attrList[unit].end()) {
        Texture2D* texture = textureReplace(unit, i->second);
        if (texture) {
          stateSet->removeTextureAttribute(unit, i->second.first.get());
          stateSet->setTextureAttribute(unit, texture, i->second.second);
          stateSet->setTextureMode(unit, GL_TEXTURE_2D, StateAttribute::ON);
        }
        ++i;
      }
    }
  }

private:
  FilePathList mPathList;
};

class SGTexCompressionVisitor : public SGTextureStateAttributeVisitor {
public:
  virtual void apply(int, StateSet::RefAttributePair& refAttr)
  {
    Texture2D* texture;
    texture = dynamic_cast<Texture2D*>(refAttr.first.get());
    if (!texture)
      return;

    // Hmm, true??
    texture->setDataVariance(osg::Object::STATIC);

    Image* image = texture->getImage(0);
    if (!image)
      return;

    int s = image->s();
    int t = image->t();

    if (s <= t && 32 <= s) {
      SGSceneFeatures::instance()->setTextureCompression(texture);
    } else if (t < s && 32 <= t) {
      SGSceneFeatures::instance()->setTextureCompression(texture);
    }
  }
};

class SGTexDataVarianceVisitor : public SGTextureStateAttributeVisitor {
public:
  virtual void apply(int, StateSet::RefAttributePair& refAttr)
  {
    Texture* texture;
    texture = dynamic_cast<Texture*>(refAttr.first.get());
    if (!texture)
      return;
    
    texture->setDataVariance(Object::STATIC);
  }
};

class SGAcMaterialCrippleVisitor : public SGStateAttributeVisitor {
public:
  virtual void apply(StateSet::RefAttributePair& refAttr)
  {
    Material* material;
    material = dynamic_cast<Material*>(refAttr.first.get());
    if (!material)
      return;
    material->setColorMode(Material::AMBIENT_AND_DIFFUSE);
  }
};
} // namespace

ReaderWriter::ReadResult
ModelRegistry::readImage(const string& fileName,
                         const ReaderWriter::Options* opt)
{
    CallbackMap::iterator iter
        = imageCallbackMap.find(getFileExtension(fileName));
    if (iter != imageCallbackMap.end() && iter->second.valid())
        return iter->second->readImage(fileName, opt);
    string absFileName = findDataFile(fileName);
    if (!fileExists(absFileName)) {
        SG_LOG(SG_IO, SG_ALERT, "Cannot find image file \""
               << fileName << "\"");
        return ReaderWriter::ReadResult::FILE_NOT_FOUND;
    }

    Registry* registry = Registry::instance();
    ReaderWriter::ReadResult res;
    res = registry->readImageImplementation(absFileName, opt);
    if (res.loadedFromCache())
        SG_LOG(SG_IO, SG_INFO, "Returning cached image \""
               << res.getImage()->getFileName() << "\"");
    else
        SG_LOG(SG_IO, SG_INFO, "Reading image \""
               << res.getImage()->getFileName() << "\"");

    return res;
}

ReaderWriter::ReadResult
ModelRegistry::readNode(const string& fileName,
                        const ReaderWriter::Options* opt)
{
    Registry* registry = Registry::instance();
    ReaderWriter::ReadResult res;
    Node* cached = 0;
    CallbackMap::iterator iter
        = nodeCallbackMap.find(getFileExtension(fileName));
    if (iter != nodeCallbackMap.end() && iter->second.valid())
        return iter->second->readNode(fileName, opt);
    // First, look for a file with the same name, and the extension
    // ".osg" and, if it exists, load it instead. This allows for
    // substitution of optimized models for ones named in the scenery.
    bool optimizeModel = true;
    string fileSansExtension = getNameLessExtension(fileName);
    string osgFileName = fileSansExtension + ".osg";
    string absFileName = findDataFile(osgFileName);
    // The absolute file name is passed to the reader plugin, which
    // calls findDataFile again... but that's OK, it should find the
    // file by its absolute path straight away.
    if (fileExists(absFileName)) {
        optimizeModel = false;
    } else {
        absFileName = findDataFile(fileName);
    }
    if (!fileExists(absFileName)) {
        SG_LOG(SG_IO, SG_ALERT, "Cannot find model file \""
               << fileName << "\"");
        return ReaderWriter::ReadResult::FILE_NOT_FOUND;
    }
    cached
        = dynamic_cast<Node*>(registry->getFromObjectCache(absFileName));
    if (cached) {
        SG_LOG(SG_IO, SG_INFO, "Got cached model \""
               << absFileName << "\"");
    } else {
        SG_LOG(SG_IO, SG_INFO, "Reading model \""
               << absFileName << "\"");
        res = registry->readNodeImplementation(absFileName, opt);
        if (!res.validNode())
            return res;

        bool needTristrip = true;
        if (getLowerCaseFileExtension(fileName) == "ac") {
            // we get optimal geometry from the loader.
            needTristrip = false;
            Matrix m(1, 0, 0, 0,
                     0, 0, 1, 0,
                     0, -1, 0, 0,
                     0, 0, 0, 1);
        
            ref_ptr<Group> root = new Group;
            MatrixTransform* transform = new MatrixTransform;
            root->addChild(transform);
        
            transform->setDataVariance(Object::STATIC);
            transform->setMatrix(m);
            transform->addChild(res.getNode());
        
            res = ReaderWriter::ReadResult(0);

            if (optimizeModel) {
                osgUtil::Optimizer optimizer;
                unsigned opts = osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS;
                optimizer.optimize(root.get(), opts);
            }

            // strip away unneeded groups
            if (root->getNumChildren() == 1 && root->getName().empty()) {
                res = ReaderWriter::ReadResult(root->getChild(0));
            } else
                res = ReaderWriter::ReadResult(root.get());
        
            // Ok, this step is questionable.
            // It is there to have the same visual appearance of ac objects for the
            // first cut. Osg's ac3d loader will correctly set materials from the
            // ac file. But the old plib loader used GL_AMBIENT_AND_DIFFUSE for the
            // materials that in effect igored the ambient part specified in the
            // file. We emulate that for the first cut here by changing all
            // ac models here. But in the long term we should use the
            // unchanged model and fix the input files instead ...
            SGAcMaterialCrippleVisitor matCriple;
            res.getNode()->accept(matCriple);
        }

        if (optimizeModel) {
            osgUtil::Optimizer optimizer;
            unsigned opts = 0;
            // Don't use this one. It will break animation names ...
            // opts |= osgUtil::Optimizer::REMOVE_REDUNDANT_NODES;

            // opts |= osgUtil::Optimizer::REMOVE_LOADED_PROXY_NODES;
            // opts |= osgUtil::Optimizer::COMBINE_ADJACENT_LODS;
            // opts |= osgUtil::Optimizer::SHARE_DUPLICATE_STATE;
            opts |= osgUtil::Optimizer::MERGE_GEOMETRY;
            // opts |= osgUtil::Optimizer::CHECK_GEOMETRY;
            // opts |= osgUtil::Optimizer::SPATIALIZE_GROUPS;
            // opts |= osgUtil::Optimizer::COPY_SHARED_NODES;
            opts |= osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS;
            if (needTristrip)
                opts |= osgUtil::Optimizer::TRISTRIP_GEOMETRY;
            // opts |= osgUtil::Optimizer::TESSELATE_GEOMETRY;
            // opts |= osgUtil::Optimizer::OPTIMIZE_TEXTURE_SETTINGS;
            optimizer.optimize(res.getNode(), opts);
        }
        // Make sure the data variance of sharable objects is set to STATIC ...
        SGTexDataVarianceVisitor dataVarianceVisitor;
        res.getNode()->accept(dataVarianceVisitor);
        // ... so that textures are now globally shared
        registry->getSharedStateManager()->share(res.getNode());
      
        SGTexCompressionVisitor texComp;
        res.getNode()->accept(texComp);
        cached = res.getNode();
        registry->addEntryToObjectCache(absFileName, cached);
    }
    // Add an extra reference to the model stored in the database.
    // That it to avoid expiring the object from the cache even if it is still
    // in use. Note that the object cache will think that a model is unused
    // if the reference count is 1. If we clone all structural nodes here
    // we need that extra reference to the original object
    SGDatabaseReference* databaseReference;
    databaseReference = new SGDatabaseReference(cached);
    CopyOp::CopyFlags flags = CopyOp::DEEP_COPY_ALL;
    flags &= ~CopyOp::DEEP_COPY_TEXTURES;
    flags &= ~CopyOp::DEEP_COPY_IMAGES;
    flags &= ~CopyOp::DEEP_COPY_ARRAYS;
    flags &= ~CopyOp::DEEP_COPY_PRIMITIVES;
    // This will safe display lists ...
    flags &= ~CopyOp::DEEP_COPY_DRAWABLES;
    flags &= ~CopyOp::DEEP_COPY_SHAPES;
    res = ReaderWriter::ReadResult(CopyOp(flags)(cached));
    res.getNode()->addObserver(databaseReference);

    // Update liveries
    SGTextureUpdateVisitor liveryUpdate(getDataFilePathList());
    res.getNode()->accept(liveryUpdate);

    // Make sure the data variance of sharable objects is set to STATIC ...
    SGTexDataVarianceVisitor dataVarianceVisitor;
    res.getNode()->accept(dataVarianceVisitor);
    // ... so that textures are now globally shared
    registry->getOrCreateSharedStateManager()->share(res.getNode(), 0);

    return res;
}

void
ModelRegistry::addImageCallbackForExtension(const string& extension,
                                            Registry::ReadFileCallback* callback)
{
    imageCallbackMap.insert(CallbackMap::value_type(extension, callback));
}

void
ModelRegistry::addNodeCallbackForExtension(const string& extension,
                                           Registry::ReadFileCallback* callback)
{
    nodeCallbackMap.insert(CallbackMap::value_type(extension, callback));
}

ref_ptr<ModelRegistry> ModelRegistry::instance;

ModelRegistry* ModelRegistry::getInstance()

{
    if (!instance.valid())
        instance = new ModelRegistry;
    return instance.get();
}

class SGReadCallbackInstaller {
public:
  SGReadCallbackInstaller()
  {
    // XXX I understand why we want this, but this seems like a weird
    // place to set this option.
    Referenced::setThreadSafeReferenceCounting(true);

    Registry* registry = Registry::instance();
    ReaderWriter::Options* options = new ReaderWriter::Options;
    // We manage node caching ourselves
    int cacheOptions = ReaderWriter::Options::CACHE_ALL
      & ~ReaderWriter::Options::CACHE_NODES;
    options->
      setObjectCacheHint((ReaderWriter::Options::CacheHintOptions)cacheOptions);
    registry->setOptions(options);
    registry->getOrCreateSharedStateManager()->
      setShareMode(SharedStateManager::SHARE_TEXTURES);
    registry->setReadFileCallback(ModelRegistry::getInstance());
  }
};

static SGReadCallbackInstaller readCallbackInstaller;

ReaderWriter::ReadResult
OSGFileCallback::readImage(const string& fileName,
                           const ReaderWriter::Options* opt)
{
    return Registry::instance()->readImageImplementation(fileName, opt);
}

ReaderWriter::ReadResult
OSGFileCallback::readNode(const string& fileName,
                          const ReaderWriter::Options* opt)
{
    return Registry::instance()->readNodeImplementation(fileName, opt);
}
