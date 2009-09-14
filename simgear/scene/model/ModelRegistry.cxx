// ModelRegistry.hxx -- interface to the OSG model registry
//
// Copyright (C) 2005-2007 Mathias Froehlich 
// Copyright (C) 2007  Tim Moore <timoore@redhat.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "ModelRegistry.hxx"

#include <algorithm>
#include <utility>
#include <vector>

#include <OpenThreads/ScopedLock>

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
#include <simgear/scene/util/NodeAndDrawableVisitor.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>

#include "BoundingVolumeBuildVisitor.hxx"

using namespace std;
using namespace osg;
using namespace osgUtil;
using namespace osgDB;
using namespace simgear;

using OpenThreads::ReentrantMutex;
using OpenThreads::ScopedLock;

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

// Set the name of a Texture to the simple name of its image
// file. This can be used to do livery substitution after the image
// has been deallocated.
class TextureNameVisitor  : public NodeAndDrawableVisitor {
public:
    TextureNameVisitor(NodeVisitor::TraversalMode tm = NodeVisitor::TRAVERSE_ALL_CHILDREN) :
        NodeAndDrawableVisitor(tm)
    {
    }

    virtual void apply(Node& node)
    {
        nameTextures(node.getStateSet());
        traverse(node);
    }

    virtual void apply(Drawable& drawable)
    {
        nameTextures(drawable.getStateSet());
    }
protected:
    void nameTextures(StateSet* stateSet)
    {
        if (!stateSet)
            return;
        int numUnits = stateSet->getTextureAttributeList().size();
        for (int i = 0; i < numUnits; ++i) {
            StateAttribute* attr
                = stateSet->getTextureAttribute(i, StateAttribute::TEXTURE);
            Texture2D* texture = dynamic_cast<Texture2D*>(attr);
            if (!texture || !texture->getName().empty())
                continue;
            const Image *image = texture->getImage();
            if (!image)
                continue;
            texture->setName(image->getFileName());
        }
    }
};

// Change the StateSets of a model to hold different textures based on
// a livery path.

class TextureUpdateVisitor : public NodeAndDrawableVisitor {
public:
    TextureUpdateVisitor(const FilePathList& pathList) :
        NodeAndDrawableVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN),
        _pathList(pathList)
    {
    }
    
    virtual void apply(Node& node)
    {
        StateSet* stateSet = cloneStateSet(node.getStateSet());
        if (stateSet)
            node.setStateSet(stateSet);
        traverse(node);
    }

    virtual void apply(Drawable& drawable)
    {
        StateSet* stateSet = cloneStateSet(drawable.getStateSet());
        if (stateSet)
            drawable.setStateSet(stateSet);
    }
    // Copied from Mathias' earlier SGTextureUpdateVisitor
protected:
    Texture2D* textureReplace(int unit, const StateAttribute* attr)
    {
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
    
    StateSet* cloneStateSet(const StateSet* stateSet)
    {
        typedef pair<int, Texture2D*> Tex2D;
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
private:
    FilePathList _pathList;
};

// Create new userdata structs in a copied model.
// The BVH trees are shared with the original model, but the velocity fields
// should usually be distinct fields for distinct models.
class UserDataCopyVisitor : public osg::NodeVisitor {
public:
    UserDataCopyVisitor() :
        osg::NodeVisitor(osg::NodeVisitor::NODE_VISITOR,
                         osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    {
    }
    virtual void apply(osg::Node& node)
    {
        osg::ref_ptr<SGSceneUserData> userData;
        userData = SGSceneUserData::getSceneUserData(&node);
        if (userData.valid()) {
            SGSceneUserData* newUserData  = new SGSceneUserData(*userData);
            newUserData->setVelocity(0);
            node.setUserData(newUserData);
        }
        node.traverse(*this);
    }
};

class SGTexCompressionVisitor : public SGTextureStateAttributeVisitor {
public:
  virtual void apply(int, StateSet::RefAttributePair& refAttr)
  {
    Texture2D* texture;
    texture = dynamic_cast<Texture2D*>(refAttr.first.get());
    if (!texture)
      return;

    // Do not touch dynamically generated textures.
    if (texture->getReadPBuffer())
      return;
    if (texture->getDataVariance() == osg::Object::DYNAMIC)
      return;

    // If no image attached, we assume this one is dynamically generated
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

    // Cannot be static if this is a render to texture thing
    if (texture->getReadPBuffer())
      return;
    if (texture->getDataVariance() == osg::Object::DYNAMIC)
      return;
    // If no image attached, we assume this one is dynamically generated
    Image* image = texture->getImage(0);
    if (!image)
      return;
    
    texture->setDataVariance(Object::STATIC);
  }

  virtual void apply(StateSet* stateSet)
  {
    if (!stateSet)
      return;
    stateSet->setDataVariance(Object::STATIC);
    SGTextureStateAttributeVisitor::apply(stateSet);
  }
};

} // namespace

Node* DefaultProcessPolicy::process(Node* node, const string& filename,
                                    const ReaderWriter::Options* opt)
{
    TextureNameVisitor nameVisitor;
    node->accept(nameVisitor);
    return node;
}

ReaderWriter::ReadResult
ModelRegistry::readImage(const string& fileName,
                         const ReaderWriter::Options* opt)
{
    ScopedLock<ReentrantMutex> lock(readerMutex);
    CallbackMap::iterator iter
        = imageCallbackMap.find(getFileExtension(fileName));
    // XXX Workaround for OSG plugin bug
    {
        if (iter != imageCallbackMap.end() && iter->second.valid())
            return iter->second->readImage(fileName, opt);
        string absFileName = findDataFile(fileName, opt);
        if (!fileExists(absFileName)) {
            SG_LOG(SG_IO, SG_ALERT, "Cannot find image file \""
                   << fileName << "\"");
            return ReaderWriter::ReadResult::FILE_NOT_FOUND;
        }

        Registry* registry = Registry::instance();
        ReaderWriter::ReadResult res;
        res = registry->readImageImplementation(absFileName, opt);
        if (!res.success()) {
          SG_LOG(SG_IO, SG_WARN, "Image loading failed:" << res.message());
          return res;
        }
        
        if (res.loadedFromCache())
            SG_LOG(SG_IO, SG_BULK, "Returning cached image \""
                   << res.getImage()->getFileName() << "\"");
        else
            SG_LOG(SG_IO, SG_BULK, "Reading image \""
                   << res.getImage()->getFileName() << "\"");

        return res;
    }
}


osg::Node* DefaultCachePolicy::find(const string& fileName,
                                    const ReaderWriter::Options* opt)
{
    Registry* registry = Registry::instance();
    osg::Node* cached
        = dynamic_cast<Node*>(registry->getFromObjectCache(fileName));
    if (cached)
        SG_LOG(SG_IO, SG_BULK, "Got cached model \""
               << fileName << "\"");
    else
        SG_LOG(SG_IO, SG_BULK, "Reading model \""
               << fileName << "\"");
    return cached;
}

void DefaultCachePolicy::addToCache(const string& fileName,
                                    osg::Node* node)
{
    Registry::instance()->addEntryToObjectCache(fileName, node);
}

// Optimizations we don't use:
// Don't use this one. It will break animation names ...
// opts |= osgUtil::Optimizer::REMOVE_REDUNDANT_NODES;
//
// opts |= osgUtil::Optimizer::REMOVE_LOADED_PROXY_NODES;
// opts |= osgUtil::Optimizer::COMBINE_ADJACENT_LODS;
// opts |= osgUtil::Optimizer::CHECK_GEOMETRY;
// opts |= osgUtil::Optimizer::SPATIALIZE_GROUPS;
// opts |= osgUtil::Optimizer::COPY_SHARED_NODES;
// opts |= osgUtil::Optimizer::TESSELATE_GEOMETRY;
// opts |= osgUtil::Optimizer::OPTIMIZE_TEXTURE_SETTINGS;

OptimizeModelPolicy::OptimizeModelPolicy(const string& extension) :
    _osgOptions(Optimizer::SHARE_DUPLICATE_STATE
                | Optimizer::MERGE_GEOMETRY
                | Optimizer::FLATTEN_STATIC_TRANSFORMS
                | Optimizer::TRISTRIP_GEOMETRY)
{
}

osg::Node* OptimizeModelPolicy::optimize(osg::Node* node,
                                         const string& fileName,
                                         const osgDB::ReaderWriter::Options* opt)
{
    osgUtil::Optimizer optimizer;
    optimizer.optimize(node, _osgOptions);

    // Make sure the data variance of sharable objects is set to
    // STATIC so that textures will be globally shared.
    SGTexDataVarianceVisitor dataVarianceVisitor;
    node->accept(dataVarianceVisitor);

    SGTexCompressionVisitor texComp;
    node->accept(texComp);
    return node;
}

osg::Node* DefaultCopyPolicy::copy(osg::Node* model, const string& fileName,
                                   const osgDB::ReaderWriter::Options* opt)
{
    // Add an extra reference to the model stored in the database.
    // That is to avoid expiring the object from the cache even if it is still
    // in use. Note that the object cache will think that a model is unused
    // if the reference count is 1. If we clone all structural nodes here
    // we need that extra reference to the original object
    SGDatabaseReference* databaseReference;
    databaseReference = new SGDatabaseReference(model);
    CopyOp::CopyFlags flags = CopyOp::DEEP_COPY_ALL;
    flags &= ~CopyOp::DEEP_COPY_TEXTURES;
    flags &= ~CopyOp::DEEP_COPY_IMAGES;
    flags &= ~CopyOp::DEEP_COPY_STATESETS;
    flags &= ~CopyOp::DEEP_COPY_STATEATTRIBUTES;
    flags &= ~CopyOp::DEEP_COPY_ARRAYS;
    flags &= ~CopyOp::DEEP_COPY_PRIMITIVES;
    // This will safe display lists ...
    flags &= ~CopyOp::DEEP_COPY_DRAWABLES;
    flags &= ~CopyOp::DEEP_COPY_SHAPES;
    osg::Node* res = CopyOp(flags)(model);
    res->addObserver(databaseReference);

    // Update liveries
    TextureUpdateVisitor liveryUpdate(opt->getDatabasePathList());
    res->accept(liveryUpdate);

    // Copy the userdata fields, still sharing the boundingvolumes,
    // but introducing new data for velocities.
    UserDataCopyVisitor userDataCopyVisitor;
    res->accept(userDataCopyVisitor);

    return res;
}

string OSGSubstitutePolicy::substitute(const string& name,
                                       const ReaderWriter::Options* opt)
{
    string fileSansExtension = getNameLessExtension(name);
    string osgFileName = fileSansExtension + ".osg";
    string absFileName = findDataFile(osgFileName, opt);
    return absFileName;
}


void
BuildLeafBVHPolicy::buildBVH(const std::string& fileName, osg::Node* node)
{
    SG_LOG(SG_IO, SG_BULK, "Building leaf attached boundingvolume tree for \""
           << fileName << "\".");
    BoundingVolumeBuildVisitor bvBuilder(true);
    node->accept(bvBuilder);
}

void
BuildGroupBVHPolicy::buildBVH(const std::string& fileName, osg::Node* node)
{
    SG_LOG(SG_IO, SG_BULK, "Building group attached boundingvolume tree for \""
           << fileName << "\".");
    BoundingVolumeBuildVisitor bvBuilder(false);
    node->accept(bvBuilder);
}

void
NoBuildBVHPolicy::buildBVH(const std::string& fileName, osg::Node*)
{
    SG_LOG(SG_IO, SG_BULK, "Omitting boundingvolume tree for \""
           << fileName << "\".");
}

ModelRegistry::ModelRegistry() :
    _defaultCallback(new DefaultCallback(""))
{
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

ReaderWriter::ReadResult
ModelRegistry::readNode(const string& fileName,
                        const ReaderWriter::Options* opt)
{
    ScopedLock<ReentrantMutex> lock(readerMutex);

    // XXX Workaround for OSG plugin bug.
//    Registry* registry = Registry::instance();
    ReaderWriter::ReadResult res;
    CallbackMap::iterator iter
        = nodeCallbackMap.find(getFileExtension(fileName));
    ReaderWriter::ReadResult result;
    if (iter != nodeCallbackMap.end() && iter->second.valid())
        result = iter->second->readNode(fileName, opt);
    else
        result = _defaultCallback->readNode(fileName, opt);

    return result;
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
    int cacheOptions = ReaderWriter::Options::CACHE_ALL;
    options->
      setObjectCacheHint((ReaderWriter::Options::CacheHintOptions)cacheOptions);
    registry->setOptions(options);
    registry->getOrCreateSharedStateManager()->
      setShareMode(SharedStateManager::SHARE_STATESETS);
    registry->setReadFileCallback(ModelRegistry::instance());
  }
};

static SGReadCallbackInstaller readCallbackInstaller;

// we get optimal geometry from the loader.
struct ACOptimizePolicy : public OptimizeModelPolicy {
    ACOptimizePolicy(const string& extension)  :
        OptimizeModelPolicy(extension)
    {
        _osgOptions &= ~Optimizer::TRISTRIP_GEOMETRY;
    }
    Node* optimize(Node* node, const string& fileName,
                   const ReaderWriter::Options* opt)
    {
        ref_ptr<Node> optimized
            = OptimizeModelPolicy::optimize(node, fileName, opt);
        Group* group = dynamic_cast<Group*>(optimized.get());
        MatrixTransform* transform
            = dynamic_cast<MatrixTransform*>(optimized.get());
        if (((transform && transform->getMatrix().isIdentity()) || group)
            && group->getName().empty()
            && group->getNumChildren() == 1) {
            optimized = static_cast<Node*>(group->getChild(0));
            group = dynamic_cast<Group*>(optimized.get());
            if (group && group->getName().empty()
                && group->getNumChildren() == 1)
                optimized = static_cast<Node*>(group->getChild(0));
        }
        return optimized.release();
    }
};

struct ACProcessPolicy {
    ACProcessPolicy(const string& extension) {}
    Node* process(Node* node, const string& filename,
                  const ReaderWriter::Options* opt)
    {
        Matrix m(1, 0, 0, 0,
                 0, 0, 1, 0,
                 0, -1, 0, 0,
                 0, 0, 0, 1);
        // XXX Does there need to be a Group node here to trick the
        // optimizer into optimizing the static transform?
        osg::Group* root = new Group;
        MatrixTransform* transform = new MatrixTransform;
        root->addChild(transform);
        
        transform->setDataVariance(Object::STATIC);
        transform->setMatrix(m);
        transform->addChild(node);

        return root;
    }
};

typedef ModelRegistryCallback<ACProcessPolicy, DefaultCachePolicy,
                              ACOptimizePolicy, DefaultCopyPolicy,
                              OSGSubstitutePolicy, BuildLeafBVHPolicy>
ACCallback;

namespace
{
ModelRegistryCallbackProxy<ACCallback> g_acRegister("ac");
}
