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


#include <simgear/debug/ErrorReportingCallback.hxx>

#include <simgear/io/sg_file.hxx>
#include <simgear/misc/lru_cache.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

#include "BoundingVolumeBuildVisitor.hxx"
#include "model.hxx"

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
      SGSceneFeatures::instance()->applyTextureCompression(texture);
    } else if (t < s && 32 <= t) {
      SGSceneFeatures::instance()->applyTextureCompression(texture);
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

static int nearestPowerOfTwo(int _v)
{
    //    uint v; // compute the next highest power of 2 of 32-bit v
    unsigned int v = (unsigned int)_v;
    bool neg = _v < 0;
    if (neg)
        v = (unsigned int)(-_v);

    v &= (2 << 16) - 1; // make +ve

    // bit twiddle to round up to nearest pot.
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    if (neg)
        _v = -(int)v;
    else
        _v = (int)v;
    return v;
}
    
static bool isPowerOfTwo(int v)
{
    return ((v & (v - 1)) == 0);
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

// least recently used cache to speed up the process of finding a file
// after the first time - otherwise each time we'll have to generate a hash
// of the contents.
static lru_cache < std::string, std::string> filename_hash_cache(100000);

// experimental (incomplete) features to allow maintenance of the filecache.
//lru_cache < std::string, bool> filesCleaned(100000);
//static bool refreshCache = false;

ReaderWriter::ReadResult
ModelRegistry::readImage(const string& fileName,
    const Options* opt)
{
    // experimental feature to see if we can reload textures during model load
    // as otherwise texture creation/editting requires a restart or a change to 
    // a different filenaem
    //if (SGSceneFeatures::instance()->getReloadCache()) {
    //    SG_LOG(SG_IO, SG_INFO, "Clearing DDS-TC LRU Cache");
    //    filename_hash_cache.clear();
    //    SGSceneFeatures::instance()->setReloadCache(false);
    //}
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
    string originalFileName = absFileName;
    if (!fileExists(absFileName)) {
        SG_LOG(SG_IO, SG_DEV_ALERT, "Cannot find image file \""
            << fileName << "\"");
        return ReaderWriter::ReadResult::FILE_NOT_FOUND;
    }
    Registry* registry = Registry::instance();
    ReaderWriter::ReadResult res;

    const SGReaderWriterOptions* sgoptC = dynamic_cast<const SGReaderWriterOptions*>(opt);

    simgear::ErrorReportContext ec;
    if (sgoptC && sgoptC->getModelData()) {
        ec.addFromMap(sgoptC->getModelData()->getErrorContext());
    }

    // only do this if the cache is active, the filename isn't already in dds or compressed with gzip
    // and providing that the load origin hint permits the usage of the cache.
    if (cache_active && fileExtension != "dds" && fileExtension != "gz" &&
        (!sgoptC
            || (sgoptC->getLoadOriginHint() != SGReaderWriterOptions::LoadOriginHint::ORIGIN_SPLASH_SCREEN
                && sgoptC->getLoadOriginHint() != SGReaderWriterOptions::LoadOriginHint::ORIGIN_CANVAS))) {
        std::string root = getPathRoot(absFileName);
        std::string prr = getPathRelative(root, absFileName);
        std::string cache_root = SGSceneFeatures::instance()->getTextureCompressionPath().c_str();
        std::string newName = cache_root + "/" + prr;

        SGPath file(absFileName);
        std::stringstream tstream;

        // calucate and use hash for storing cached image. This also
        // helps with sharing of identical images between models.
        if (fileExists(absFileName)) {
            SGFile f(absFileName);
            std::string hash;

            //                std::string attr = "";
            //                if (sgoptC && sgoptC->getLoadOriginHint() == SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS_NORMALIZED) {
            //                    attr += " effnorm";
            //                }
            //                else if (sgoptC && sgoptC->getLoadOriginHint() == SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS) {
            //                    attr += " eff";
            //                    //                        can_compress = false;
            //                }
            //                SG_LOG(SG_IO, SG_INFO, absFileName << attr);
            boost::optional<std::string> cachehash = filename_hash_cache.get(absFileName);
            if (cachehash) {
                hash = *cachehash;
            }
            else {
                try {
                    hash = f.computeHash();
                }
                catch (sg_io_exception& e) {
                    SG_LOG(SG_IO, SG_DEV_ALERT, "Modelregistry::failed to compute filehash '" << absFileName << "' " << e.getFormattedMessage());
                    hash = std::string();
                }
            }
            if (hash != std::string()) {
                filename_hash_cache.insert(absFileName, hash);
                boost::optional<std::string> cacheFilename = filename_hash_cache.findValue(hash);

                // possibly a shared texture - but warn the user to allow investigation.
                if (cacheFilename && *cacheFilename != absFileName) {
                    SG_LOG(SG_IO, SG_INFO, " Already have " + hash + " : " + *cacheFilename + " not " + absFileName);
                }
            }
            newName = cache_root + "/" + hash.substr(0, 2) + "/" + hash + ".cache.dds";
        }
        else
        {
            tstream << std::hex << file.modTime();
            newName += "." + tstream.str();
            newName += ".cache.dds";
        }
        //bool doRefresh = refreshCache;
        //if (newName != std::string() && fileExists(newName) && doRefresh) {
        //    if (!filesCleaned.contains(newName)) {
        //        SG_LOG(SG_IO, SG_INFO, "Removing previously cached effects image " + newName);
        //        SGPath(newName).remove();
        //        filesCleaned.insert(newName, true);
        //    }
        //}

        if (newName != std::string() && !fileExists(newName)) {
            res = registry->readImageImplementation(absFileName, opt);
            if (res.validImage()) {
                osg::ref_ptr<osg::Image> srcImage = res.getImage();
                int width = srcImage->s();
                //int packing = srcImage->getPacking();
                //printf("packing %d format %x pixel size %d InternalTextureFormat %x\n", packing, srcImage->getPixelFormat(), srcImage->getPixelSizeInBits(), srcImage->getInternalTextureFormat() );
                bool transparent = srcImage->isImageTranslucent();
                bool isNormalMap = false;
                bool isEffect = false;
                /*
                * decide if we need to compress this.
                */
                bool can_compress = (transparent && compress_transparent) || (!transparent && compress_solid);

                if (srcImage->getPixelSizeInBits() <= 16) {
                    SG_LOG(SG_IO, SG_INFO, "Ignoring " + absFileName + " for inclusion into the texture cache because pixel density too low at " << srcImage->getPixelSizeInBits() << " bits per pixek");
                    can_compress = false;
                }

                int height = srcImage->t();

                // use the new file origin to determine any special processing
                // we handle the following
                // - normal maps
                // - images loaded from effects
                if (sgoptC && transparent && sgoptC->getLoadOriginHint() == SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS_NORMALIZED) {
                    isNormalMap = true;
                }
                else if (sgoptC && transparent && sgoptC->getLoadOriginHint() == SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS) {
                    SG_LOG(SG_IO, SG_INFO, "From effects transparent " + absFileName + " will generate mipmap only");
                    isEffect = true;
                    can_compress = false;
                }
                else if (sgoptC && !transparent && sgoptC->getLoadOriginHint() == SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS) {
                    SG_LOG(SG_IO, SG_INFO, "From effects " + absFileName + " will generate mipmap only");
                    isEffect = true;
                    //                        can_compress = false;
                }
                if (can_compress)
                {
                    std::string pot_message;
                    bool resize = false;
                    if (!isPowerOfTwo(width)) {
                        width = nearestPowerOfTwo(width);
                        resize = true;
                        pot_message += std::string(" not POT: resized width to ") + std::to_string(width);
                    }
                    if (!isPowerOfTwo(height)) {
                        height = nearestPowerOfTwo(height);
                        resize = true;
                        pot_message += std::string(" not POT: resized height to ") + std::to_string(height);
                    }

                    if (pot_message.size())
                        SG_LOG(SG_IO, SG_DEV_WARN, pot_message << " " << absFileName);

                    // unlikely that after resizing in height the width will still be outside of the max texture size.
                    if (height > max_texture_size)
                    {
                        SG_LOG(SG_IO, SG_DEV_WARN, "Image texture too high (max " << max_texture_size << ") " << width << "," << height << " " << absFileName);
                        int factor = height / max_texture_size;
                        height /= factor;
                        width /= factor;
                        resize = true;
                    }
                    if (width > max_texture_size)
                    {
                        SG_LOG(SG_IO, SG_DEV_WARN, "Image texture too wide (max " << max_texture_size << ") " << width << "," << height << " " << absFileName);
                        int factor = width / max_texture_size;
                        height /= factor;
                        width /= factor;
                        resize = true;
                    }

                    if (resize) {
                        osg::ref_ptr<osg::Image> resizedImage;

                        if (ImageUtils::resizeImage(srcImage, width, height, resizedImage))
                            srcImage = resizedImage;
                    }

                    //
                    // only cache power of two textures that are of a reasonable size
                    if (width >= 4 && height >= 4) {

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

                        //GLenum srcImageType = srcImage->getDataType();
                        //                            printf("--- %-80s --> f=%8x t=%8x\n", newName.c_str(), srcImage->getPixelFormat(), srcImageType);

                        try
                        {
                            if (can_compress) {
                                osg::Texture::InternalFormatMode targetFormat = osg::Texture::USE_S3TC_DXT1_COMPRESSION;
                                if (isNormalMap) {
                                    if (transparent) {
                                        targetFormat = osg::Texture::USE_S3TC_DXT5_COMPRESSION;
                                    }
                                    else
                                        targetFormat = osg::Texture::USE_S3TC_DXT5_COMPRESSION;
                                }
                                else if (isEffect)
                                {
                                    if (transparent) {
                                        targetFormat = osg::Texture::USE_S3TC_DXT5_COMPRESSION;
                                    }
                                    else
                                        targetFormat = osg::Texture::USE_S3TC_DXT1_COMPRESSION;
                                }
                                else {
                                    if (transparent) {
                                        targetFormat = osg::Texture::USE_S3TC_DXT3_COMPRESSION;
                                    }
                                    else
                                        targetFormat = osg::Texture::USE_S3TC_DXT1_COMPRESSION;
                                }

                                if (processor)
                                {
                                    SG_LOG(SG_IO, SG_DEV_ALERT, "Creating " << targetFormat << " for " + absFileName);
                                    // normal maps:
                                    // nvdxt.exe - quality_highest - rescaleKaiser - Kaiser - dxt5nm - norm
                                    processor->compress(*srcImage, targetFormat, true, true, osgDB::ImageProcessor::USE_CPU, osgDB::ImageProcessor::PRODUCTION);
                                    SG_LOG(SG_IO, SG_INFO, "-- finished creating DDS: " + newName);
                                    //processor->generateMipMap(*srcImage, true, osgDB::ImageProcessor::USE_CPU);
                                }
                                else {
                                    simgear::effect::MipMapTuple mipmapFunctions(simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE);
                                    SG_LOG(SG_IO, SG_INFO, "Texture compression plugin (osg_nvtt) not available; storing uncompressed image: " << absFileName);
                                    srcImage = simgear::effect::computeMipmap(srcImage, mipmapFunctions);
                                }
                            }
                            else {
                                SG_LOG(SG_IO, SG_INFO, "Creating uncompressed DDS for " + absFileName);
                                //if (processor) {
                                //    processor->generateMipMap(*srcImage, true, osgDB::ImageProcessor::USE_CPU);
                                //}
                                //else 
                                {
                                    simgear::effect::MipMapTuple mipmapFunctions(simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE);
                                    srcImage = simgear::effect::computeMipmap(srcImage, mipmapFunctions);
                                }
                            }
                            //}
                            //else
                            //    printf("--- no compress or mipmap of format %s\n", newName.c_str());
                            registry->writeImage(*srcImage, newName, nopt);
                            {
                                std::string mdlDirectory = cache_root + "/cache-index.txt";
                                FILE* f = ::fopen(mdlDirectory.c_str(), "a");
                                if (f)
                                {
                                    ::fprintf(f, "%s, %s\n", absFileName.c_str(), newName.c_str());
                                    ::fclose(f);
                                }
                            }
                            absFileName = newName;
                        }
                        catch (...) {
                            SG_LOG(SG_IO, SG_DEV_ALERT, "Exception processing " << absFileName << " may be corrupted");
                        }
                    }
                    else
                        SG_LOG(SG_IO, SG_DEV_WARN, absFileName + " too small " << width << "," << height);
                }
            }
        }
        else {
            if (newName != std::string())
                absFileName = newName;
        }
    }
//    SG_LOG(SG_IO, SG_ALERT, "Loading image file "<< absFileName);


    try {
        res = registry->readImageImplementation(absFileName, opt);
    } catch (std::bad_alloc&) {
        simgear::reportFailure(simgear::LoadFailure::OutOfMemory, simgear::ErrorCode::ThreeDModelLoad,
                               "Out of memory loading texture", sg_location{absFileName});
        return ReaderWriter::ReadResult::INSUFFICIENT_MEMORY_TO_LOAD;
    }

    if (!res.success()) {
        SG_LOG(SG_IO, SG_DEV_WARN, "Image loading failed:" << res.message());
        return res;
    }

    osg::ref_ptr<osg::Image> srcImage1 = res.getImage();

    /*
     * Fixup the filename - as when loading from eg. dds.gz the originating filename is lost in the conversion due to the way the OSG loader works
     */
    //if (srcImage1->getFileName().empty()) {
    //    srcImage1->setFileName(absFileName);
    //}
    srcImage1->setFileName(originalFileName);

    if(cache_active && getFileExtension(absFileName) != "dds"&& getFileExtension(absFileName) != "gz")
    {
        // In testing the internal mipmap generation works better than the external nvtt one
        // (less artefacts); it might be that there are flags we can use to make this better
        // but for now we'll just using the built in one
        //if (processor) {
        //     processor->generateMipMap(*srcImage1, true, osgDB::ImageProcessor::USE_CPU);
        //     SG_LOG(SG_IO, SG_INFO, "Created nvtt mipmaps DDS for " + absFileName);
        // }
        // else
        {
            simgear::effect::MipMapTuple mipmapFunctions(simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE);
            srcImage1 = simgear::effect::computeMipmap(srcImage1, mipmapFunctions);
            SG_LOG(SG_IO, SG_DEBUG, "Created sg mipmaps DDS for " + absFileName);
        }
    }

    if (res.loadedFromCache())
        SG_LOG(SG_IO, SG_BULK, "Returning cached image \""
            << res.getImage()->getFileName() << "\"");
    else
        SG_LOG(SG_IO, SG_BULK, "Reading image \""
            << res.getImage()->getFileName() << "\"");

    return res;
}


osg::ref_ptr<osg::Node> DefaultCachePolicy::find(const string& fileName, const Options* opt)
{
    Registry* registry = Registry::instance();
#if OSG_VERSION_LESS_THAN(3,4,0)
    osg::ref_ptr<osg::Object> cachedObject = registry->getFromObjectCache(fileName);
#else
    osg::ref_ptr<osg::Object> cachedObject = registry->getRefFromObjectCache(fileName);
#endif

    ref_ptr<osg::Node> cachedNode = dynamic_cast<osg::Node*>(cachedObject.get());
    if (cachedNode.valid())
        SG_LOG(SG_IO, SG_BULK, "Got cached model \"" << fileName << "\"");
    else
        SG_LOG(SG_IO, SG_BULK, "Reading model \"" << fileName << "\"");
    return cachedNode;
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
    // propogate error context from the caller
    simgear::ErrorReportContext ec;
    auto sgopt = dynamic_cast<const SGReaderWriterOptions*>(opt);
    if (sgopt) {
        if (sgopt->getModelData()) {
            ec.addFromMap(sgopt->getModelData()->getErrorContext());
        }

        ec.addFromMap(sgopt->getErrorContext());
    }

    CallbackMap::iterator iter
        = nodeCallbackMap.find(getFileExtension(fileName));
    ReaderWriter::ReadResult result;
    if (iter != nodeCallbackMap.end() && iter->second.valid())
        result = iter->second->readNode(fileName, opt);
    else
        result = _defaultCallback->readNode(fileName, opt);

    if (!result.validNode()) {
        simgear::reportFailure(simgear::LoadFailure::BadData, simgear::ErrorCode::ThreeDModelLoad,
                               "Failed to load 3D model:" + result.message(), sg_location{fileName});
    }

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
