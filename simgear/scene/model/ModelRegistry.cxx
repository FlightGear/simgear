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
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/NodeAndDrawableVisitor.hxx>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>

#include "BoundingVolumeBuildVisitor.hxx"
#include "model.hxx"

using namespace std;
using namespace osg;
using namespace osgUtil;
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
    CallbackMap::iterator iter
        = imageCallbackMap.find(getFileExtension(fileName));
    {
        if (iter != imageCallbackMap.end() && iter->second.valid())
            return iter->second->readImage(fileName, opt);
        string absFileName = SGModelLib::findDataFile(fileName, opt);
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

        // Check for precompressed textures that depend on an extension
        switch (res.getImage()->getPixelFormat()) {

            // GL_EXT_texture_compression_s3tc
            // patented, no way to decompress these
#ifndef GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT   0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT  0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT  0x83F3
#endif
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            
            // GL_EXT_texture_sRGB
            // patented, no way to decompress these
#ifndef GL_EXT_texture_sRGB
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT  0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            
            // GL_TDFX_texture_compression_FXT1
            // can decompress these in software but
            // no code present in simgear.
#ifndef GL_3DFX_texture_compression_FXT1
#define GL_COMPRESSED_RGB_FXT1_3DFX       0x86B0
#define GL_COMPRESSED_RGBA_FXT1_3DFX      0x86B1
#endif
        case GL_COMPRESSED_RGB_FXT1_3DFX:
        case GL_COMPRESSED_RGBA_FXT1_3DFX:
            
            // GL_EXT_texture_compression_rgtc
            // can decompress these in software but
            // no code present in simgear.
#ifndef GL_EXT_texture_compression_rgtc
#define GL_COMPRESSED_RED_RGTC1_EXT       0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
#define GL_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
#endif
        case GL_COMPRESSED_RED_RGTC1_EXT:
        case GL_COMPRESSED_SIGNED_RED_RGTC1_EXT:
        case GL_COMPRESSED_RED_GREEN_RGTC2_EXT:
        case GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT:

            SG_LOG(SG_IO, SG_ALERT, "Image \"" << res.getImage()->getFileName()
                   << "\" contains non portable compressed textures.\n"
                   "Usage of these textures depend on an extension that"
                   " is not guaranteed to be present.");
            break;

        default:
            break;
        }

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

string OSGSubstitutePolicy::substitute(const string& name,
                                       const ReaderWriter::Options* opt)
{
    string fileSansExtension = getNameLessExtension(name);
    string osgFileName = fileSansExtension + ".osg";
    string absFileName = SGModelLib::findDataFile(osgFileName, opt);
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

// we get optimal geometry from the loader (Hah!).
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
        const SGReaderWriterOptions* sgopt
            = dynamic_cast<const SGReaderWriterOptions*>(opt);
        if (sgopt && sgopt->getInstantiateEffects())
            optimized = instantiateEffects(optimized.get(), sgopt);
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
                              ACOptimizePolicy,
                              OSGSubstitutePolicy, BuildLeafBVHPolicy>
ACCallback;

namespace
{
ModelRegistryCallbackProxy<ACCallback> g_acRegister("ac");
}
