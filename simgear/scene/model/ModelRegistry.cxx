// ModelRegistry.cxx -- interface to the OSG model registry
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
#include <simgear/scene/util/SGImageUtils.hxx>
#include "../material/mipmap.hxx"

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
#include <osg/Version>
#include <osgDB/Archive>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/Registry>
#include <osgDB/SharedStateManager>
#include <osgUtil/Optimizer>
#include <osg/Texture>

#include <simgear/sg_inlines.h>

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

#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

using namespace std;
using namespace osg;
using namespace osgUtil;
using namespace osgDB;
using namespace simgear;

namespace {
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

static bool isPowerOfTwo(int width, int height)
{
    return (((width & (width - 1)) == 0) && ((height & (height - 1))) == 0);
}
osg::Node* DefaultProcessPolicy::process(osg::Node* node, const std::string& filename,
    const Options* opt)
{
    TextureNameVisitor nameVisitor;
    node->accept(nameVisitor);
    return node;
}

//#define LOCAL_IMAGE_CACHE
#ifdef LOCAL_IMAGE_CACHE
typedef std::map<std::string, osg::ref_ptr<Image>> ImageMap;
ImageMap _imageMap;
//typedef std::map<std::string, osg::ref_ptr<osg::Image> >    ImageMap;
//ImageMap _imageMap;
osg::Image* getImageByName(const std::string& filename)
{
    ImageMap::iterator itr = _imageMap.find(filename);
    if (itr != _imageMap.end()) return itr->second.get();
    return nullptr;
}
#endif

ReaderWriter::ReadResult
ModelRegistry::readImage(const string& fileName,
    const Options* opt)
{
    /*
     * processor is the interface to the osg_nvtt plugin
     */
    static bool init = false;
    static osgDB::ImageProcessor *processor = 0;
    int max_texture_size = SGSceneFeatures::instance()->getMaxTextureSize();
    if (!init) {
        processor = osgDB::Registry::instance()->getImageProcessor();
        init = true;
    }

    bool persist = true;
    bool cache_active = SGSceneFeatures::instance()->getTextureCacheActive();
    bool compress_solid = SGSceneFeatures::instance()->getTextureCacheCompressionActive();
    bool compress_transparent = SGSceneFeatures::instance()->getTextureCacheCompressionActiveTransparent();

    //
    // heuristically less than 2048 is more likely to be a badly reported size rather than
    // something that is valid so we'll have a minimum size of 2048.
    if (max_texture_size < 2048)
        max_texture_size = 2048;

    std::string fileExtension = getFileExtension(fileName);
    CallbackMap::iterator iter = imageCallbackMap.find(fileExtension);

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

    if (cache_active) {
        if (fileExtension != "dds" && fileExtension != "gz") {
            std::string root = getPathRoot(absFileName);
            std::string prr = getPathRelative(root, absFileName);
            std::string cache_root = SGSceneFeatures::instance()->getTextureCompressionPath().c_str();
            std::string newName = cache_root + "/" + prr;

            SGPath file(absFileName);
            std::stringstream tstream;
            tstream << std::hex << file.modTime();
            newName += "." + tstream.str();

            newName += ".cache.dds";
            if (!fileExists(newName)) {
                res = registry->readImageImplementation(absFileName, opt);

                osg::ref_ptr<osg::Image> srcImage = res.getImage();
                int width = srcImage->s();
                bool transparent = srcImage->isImageTranslucent();
                int height = srcImage->t();

                if (height >= max_texture_size)
                {
                    SG_LOG(SG_IO, SG_WARN, "Image texture too high " << width << "," << height << absFileName);
                    osg::ref_ptr<osg::Image> resizedImage;
                    int factor = height / max_texture_size;
                    if (ImageUtils::resizeImage(srcImage, width / factor, height / factor, resizedImage))
                        srcImage = resizedImage;
                    width = srcImage->s();
                    height = srcImage->t();
                }
                if (width >= max_texture_size)
                {
                    SG_LOG(SG_IO, SG_WARN, "Image texture too wide " << width << "," << height << absFileName);
                    osg::ref_ptr<osg::Image> resizedImage;
                    int factor = width / max_texture_size;
                    if (ImageUtils::resizeImage(srcImage, width / factor, height / factor, resizedImage))
                        srcImage = resizedImage;
                    width = srcImage->s();
                    height = srcImage->t();
                }

                //
                // only cache power of two textures that are of a reasonable size
                if (width >= 64 && height >= 64 && isPowerOfTwo(width, height)) {
                    simgear::effect::MipMapTuple mipmapFunctions(simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE);

                    SGPath filePath(newName);
                    filePath.create_dir();

                    // setup the options string for saving the texture as we don't want OSG to auto flip the texture
                    // as this complicates loading as it requires a flag to flip it back which will preclude the
                    // image from being cached because we will have to clone the options to set the flag and thus lose
                    // the link to the cache in the options from the caller.
                    osg::ref_ptr<Options> nopt;
                    nopt = opt->cloneOptions();
                    std::string optionstring = nopt->getOptionString();

                    if (!optionstring.empty())
                        optionstring += " ";

                    nopt->setOptionString(optionstring + "ddsNoAutoFlipWrite");

                    /*
                     * decide if we need to compress this.
                     */
                    bool compress = (transparent && compress_transparent) || (!transparent && compress_solid);

                    if (compress) {
                        if (processor)
                        {
                            if (transparent)
                                processor->compress(*srcImage, osg::Texture::USE_S3TC_DXT5_COMPRESSION, true, true, osgDB::ImageProcessor::USE_CPU, osgDB::ImageProcessor::PRODUCTION);
                            else
                                processor->compress(*srcImage, osg::Texture::USE_S3TC_DXT1_COMPRESSION, true, true, osgDB::ImageProcessor::USE_CPU, osgDB::ImageProcessor::PRODUCTION);
                            //processor->generateMipMap(*srcImage, true, osgDB::ImageProcessor::USE_CPU);
                        }
                        else {
                            simgear::effect::MipMapTuple mipmapFunctions(simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE);
                            SG_LOG(SG_IO, SG_WARN, "Texture compression plugin (osg_nvtt) not available; storing uncompressed image: " << newName);
                            srcImage = simgear::effect::computeMipmap(srcImage, mipmapFunctions);
                        }
                    }
                    else {
                        if (processor) {
                            processor->generateMipMap(*srcImage, true, osgDB::ImageProcessor::USE_CPU);
                        }
                        else {
                            simgear::effect::MipMapTuple mipmapFunctions(simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE);
                            srcImage = simgear::effect::computeMipmap(srcImage, mipmapFunctions);
                        }
                    }
                    if (persist) {
                        registry->writeImage(*srcImage, newName, nopt);
                        //printf(" ->> written to %s\n", newName.c_str());

                    }
                    else {
                        return srcImage;
                    }
                    absFileName = newName;
                }
            }
            else
                absFileName = newName;
        }
    }
    res = registry->readImageImplementation(absFileName, opt);

    if (!res.success()) {
        SG_LOG(SG_IO, SG_WARN, "Image loading failed:" << res.message());
        return res;
    }

    osg::ref_ptr<osg::Image> srcImage1 = res.getImage();

    //printf(" --> finished loading %s [%s] (%s) %d\n", absFileName.c_str(), srcImage1->getFileName().c_str(), res.loadedFromCache() ? "from cache" : "from disk", res.getImage()->getOrigin());
    /*
    * Fixup the filename - as when loading from eg. dds.gz the originating filename is lost in the conversion due to the way the OSG loader works
    */
    if (srcImage1->getFileName().empty()) {
        srcImage1->setFileName(absFileName);
    }

    if (srcImage1->getName().empty()) {
        srcImage1->setName(absFileName);
    }

    if (res.loadedFromCache())
        SG_LOG(SG_IO, SG_BULK, "Returning cached image \""
            << res.getImage()->getFileName() << "\"");
    else
        SG_LOG(SG_IO, SG_BULK, "Reading image \""
            << res.getImage()->getFileName() << "\"");

    // as of March 2018 all patents have expired, https://en.wikipedia.org/wiki/S3_Texture_Compression#Patent
    // there is support for S3TC DXT1..5 in MESA  https://www.phoronix.com/scan.php?page=news_item&px=S3TC-Lands-In-Mesa
    // so it seems that there isn't a valid reason to warn any longer; and beside this is one of those cases where it should
    // really only be a developer message
#ifdef WARN_DDS_TEXTURES
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

        SG_LOG(SG_IO, SG_WARN, "Image \"" << fileName << "\"\n"
            "uses compressed textures which cannot be supported on "
            "some systems.\n"
            "Please decompress this texture for improved portability.");
        break;

    default:
        break;
    }
#endif

    return res;
}


osg::Node* DefaultCachePolicy::find(const string& fileName,
                                    const Options* opt)
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
                | Optimizer::INDEX_MESH
                | Optimizer::VERTEX_POSTTRANSFORM
                | Optimizer::VERTEX_PRETRANSFORM)
{
}

osg::Node* OptimizeModelPolicy::optimize(osg::Node* node,
                                         const string& fileName,
                                         const osgDB::Options* opt)
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
                                       const Options* opt)
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
                        const Options* opt)
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
#if OSG_VERSION_LESS_THAN(3,5,4)
      //RJH - see OSG 11ddd53eb46d15903d036b594bfa3826d9e89393 -
      //      Removed redundent Referenced::s/getThreadSafeReferenceCounting() and associated static and env vars
      //      as there are now inapprorpiate and no longer supported
    Referenced::setThreadSafeReferenceCounting(true);
#endif

    Registry* registry = Registry::instance();
    Options* options = new Options;
    int cacheOptions = Options::CACHE_ALL;
    options->
      setObjectCacheHint((Options::CacheHintOptions)cacheOptions);
    registry->setOptions(options);
    registry->getOrCreateSharedStateManager()->
      setShareMode(SharedStateManager::SHARE_ALL);
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
                   const Options* opt)
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

        if (sgopt && sgopt->getInstantiateMaterialEffects()) {
            optimized = instantiateMaterialEffects(optimized.get(), sgopt);
        } else if (sgopt && sgopt->getInstantiateEffects()) {
            optimized = instantiateEffects(optimized.get(), sgopt);
        }

        return optimized.release();
    }
};

struct ACProcessPolicy {
    ACProcessPolicy(const string& extension) {}
    Node* process(Node* node, const string& filename,
                  const Options* opt)
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

struct OBJProcessPolicy {
    OBJProcessPolicy(const string& extension) {}
    Node* process(Node* node, const string& filename,
                  const Options* opt)
    {
        SG_UNUSED(filename);
        SG_UNUSED(opt);
        return node;
    }
};


typedef ModelRegistryCallback<OBJProcessPolicy,
                              DefaultCachePolicy,
                              ACOptimizePolicy,
                              OSGSubstitutePolicy, BuildLeafBVHPolicy>
OBJCallback;


// we get optimal geometry from the loader (Hah!).
struct IVEOptimizePolicy : public OptimizeModelPolicy {
    IVEOptimizePolicy(const string& extension) :
        OptimizeModelPolicy(extension)
    {
        _osgOptions &= ~Optimizer::TRISTRIP_GEOMETRY;
    }
    Node* optimize(Node* node, const string& fileName,
        const Options* opt)
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

        if (sgopt && sgopt->getInstantiateMaterialEffects()) {
            optimized = instantiateMaterialEffects(optimized.get(), sgopt);
        }
        else if (sgopt && sgopt->getInstantiateEffects()) {
            optimized = instantiateEffects(optimized.get(), sgopt);
        }

        return optimized.release();
    }
};

struct IVEProcessPolicy {
    IVEProcessPolicy(const string& extension) {}
    Node* process(Node* node, const string& filename,
        const Options* opt)
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

typedef ModelRegistryCallback<IVEProcessPolicy, DefaultCachePolicy,
    IVEOptimizePolicy,
    OSGSubstitutePolicy, BuildLeafBVHPolicy>
    IVECallback;

namespace
{
ModelRegistryCallbackProxy<ACCallback> g_acRegister("ac");
ModelRegistryCallbackProxy<OBJCallback> g_objRegister("obj");
ModelRegistryCallbackProxy<IVECallback> g_iveRegister("ive");
ModelRegistryCallbackProxy<IVECallback> g_osgtRegister("osgt");
ModelRegistryCallbackProxy<IVECallback> g_osgbRegister("osgb");

}
