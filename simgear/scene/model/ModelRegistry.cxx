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
#include <simgear/io/sg_file.hxx>
#include <simgear/threads/SGGuard.hxx>

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

static int nearestPowerOfTwo(unsigned int _v)
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
// a cache which evicts the least recently used item when it is full
#include <map>
#include <list>
#include <utility>

#include <boost/optional.hpp>
template<class Key, class Value>
class lru_cache
{
public:
    SGMutex _mutex;

    typedef Key key_type;
    typedef Value value_type;
    typedef std::list<key_type> list_type;
    typedef std::map<
        key_type,
        std::pair<value_type, typename list_type::iterator>
    > map_type;

    lru_cache(size_t capacity)
        : m_capacity(capacity)
    {
    }

    ~lru_cache()
    {
    }


    size_t size() const
    {
        return m_map.size();
    }

    size_t capacity() const
    {
        return m_capacity;
    }

    bool empty() const
    {
        return m_map.empty();
    }

    bool contains(const key_type &key)
    {
        SGGuard<SGMutex> scopeLock(_mutex);
        return m_map.find(key) != m_map.end();
    }

    void insert(const key_type &key, const value_type &value)
    {
        SGGuard<SGMutex> scopeLock(_mutex);
        typename map_type::iterator i = m_map.find(key);
        if (i == m_map.end()) {
            // insert item into the cache, but first check if it is full
            if (size() >= m_capacity) {
                // cache is full, evict the least recently used item
                evict();
            }

            // insert the new item
            m_list.push_front(key);
            m_map[key] = std::make_pair(value, m_list.begin());
        }
    }
    boost::optional<key_type> findValue(const std::string &requiredValue)
    {
        SGGuard<SGMutex> scopeLock(_mutex);
        for (typename map_type::iterator it = m_map.begin(); it != m_map.end(); ++it)
            if (it->second.first == requiredValue)
                return it->first;
        return boost::none;
    }
    boost::optional<value_type> get(const key_type &key)
    {
        SGGuard<SGMutex> scopeLock(_mutex);
        // lookup value in the cache
        typename map_type::iterator i = m_map.find(key);
        if (i == m_map.end()) {
            // value not in cache
            return boost::none;
        }

        // return the value, but first update its place in the most
        // recently used list
        typename list_type::iterator j = i->second.second;
        if (j != m_list.begin()) {
            // move item to the front of the most recently used list
            m_list.erase(j);
            m_list.push_front(key);

            // update iterator in map
            j = m_list.begin();
            const value_type &value = i->second.first;
            m_map[key] = std::make_pair(value, j);

            // return the value
            return value;
        }
        else {
            // the item is already at the front of the most recently
            // used list so just return it
            return i->second.first;
        }
    }

    void clear()
    {
        SGGuard<SGMutex> scopeLock(_mutex);
        m_map.clear();
        m_list.clear();
    }

private:
    void evict()
    {
        SGGuard<SGMutex> scopeLock(_mutex);
        // evict item from the end of most recently used list
        typename list_type::iterator i = --m_list.end();
        m_map.erase(*i);
        m_list.erase(i);
    }

private:
    map_type m_map;
    list_type m_list;
    size_t m_capacity;
};
lru_cache < std::string, std::string> filename_hash_cache(100000);
lru_cache < std::string, bool> filesCleaned(100000);
static bool refreshCache = false;

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
        SG_LOG(SG_IO, SG_ALERT, "Cannot find image file \""
            << fileName << "\"");
        return ReaderWriter::ReadResult::FILE_NOT_FOUND;
    }
    Registry* registry = Registry::instance();
    ReaderWriter::ReadResult res;

    if (cache_active) {
        if (fileExtension != "dds" && fileExtension != "gz") {
            const SGReaderWriterOptions* sgoptC = dynamic_cast<const SGReaderWriterOptions*>(opt);

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
                boost::optional<std::string> cachehash = filename_hash_cache.get(absFileName);
                if (cachehash) {
                    hash = *cachehash;
//                    SG_LOG(SG_IO, SG_ALERT, "Hash for " + absFileName + " in cache " + hash);
                }
                else {
                    //                  SG_LOG(SG_IO, SG_ALERT, "Creating hash for " + absFileName);
                    try {
                        hash = f.computeHash();
                    }
                    catch (sg_io_exception &e) {
                        SG_LOG(SG_INPUT, SG_ALERT, "Modelregistry::failed to compute filehash '" << absFileName << "' " << e.getFormattedMessage());
                        hash = std::string();
                    }
                    if (hash != std::string()) {
                        filename_hash_cache.insert(absFileName, hash);
                        boost::optional<std::string> cacheFilename = filename_hash_cache.findValue(hash);

                        // possibly a shared texture - but warn the user to allow investigation.
                        if (cacheFilename && *cacheFilename != absFileName) {
                            SG_LOG(SG_IO, SG_ALERT, " Already have " + hash + " : " + *cacheFilename + " not " + absFileName);
                        }
                        //                    SG_LOG(SG_IO, SG_ALERT, " >>>> " + hash + " :: " + newName);
                    }
                    newName = cache_root + "/" + hash.substr(0, 2) + "/" + hash + ".cache.dds";
                }
            }
            else
            {
                tstream << std::hex << file.modTime();
                newName += "." + tstream.str();
                newName += ".cache.dds";
            }
            bool doRefresh = refreshCache;
            //if (fileExists(newName) && sgoptC && sgoptC->getLoadOriginHint() == SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS) {
            //    doRefresh = true;
            //    
            //}

            if (newName != std::string() && fileExists(newName) && doRefresh) {
                if (!filesCleaned.contains(newName)) {
                    SG_LOG(SG_IO, SG_ALERT, "Removing previously cached effects image " + newName);
                    SGPath(newName).remove();
                    filesCleaned.insert(newName, true);
                }

            }

            if (newName != std::string() && !fileExists(newName)) {
                res = registry->readImageImplementation(absFileName, opt);
                if (res.validImage()) {
                    osg::ref_ptr<osg::Image> srcImage = res.getImage();
                    int width = srcImage->s();
                    bool transparent = srcImage->isImageTranslucent();
                    bool isNormalMap = false;
                    bool isEffect = false;
                    /*
                    * decide if we need to compress this.
                    */
                    bool can_compress = (transparent && compress_transparent) || (!transparent && compress_solid);

                    int height = srcImage->t();

                    // use the new file origin to determine any special processing
                    // we handle the following
                    // - normal maps
                    // - images loaded from effects
                    if (sgoptC && transparent && sgoptC->getLoadOriginHint() == SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS_NORMALIZED) {
                        isNormalMap = true;
                    }
                    else if (sgoptC && transparent && sgoptC->getLoadOriginHint() == SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS) {
                        SG_LOG(SG_IO, SG_ALERT, "From effects transparent " + absFileName);
                        isEffect = true;
//                        can_compress = false;
                    }
                    else if (sgoptC && transparent && sgoptC->getLoadOriginHint() == SGReaderWriterOptions::LoadOriginHint::ORIGIN_EFFECTS) {
                        SG_LOG(SG_IO, SG_ALERT, "From effects " + absFileName);
                        isEffect = true;
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
SG_LOG(SG_IO, SG_WARN, pot_message << " " << absFileName);

                        // unlikely that after resizing in height the width will still be outside of the max texture size.
                        if (height > max_texture_size)
                        {
                            SG_LOG(SG_IO, SG_WARN, "Image texture too high (max " << max_texture_size << ") " << width << "," << height << " " << absFileName);
                            int factor = height / max_texture_size;
                            height /= factor;
                            width /= factor;
                            resize = true;
                        }
                        if (width > max_texture_size)
                        {
                            SG_LOG(SG_IO, SG_WARN, "Image texture too wide (max " << max_texture_size << ") " << width << "," << height << " " << absFileName);
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
                                    else{
                                        if (transparent) {
                                            targetFormat = osg::Texture::USE_S3TC_DXT3_COMPRESSION;
                                        }
                                        else
                                            targetFormat = osg::Texture::USE_S3TC_DXT1_COMPRESSION;
                                    }

                                    if (processor)
                                    {
                                        SG_LOG(SG_IO, SG_ALERT, "Creating " << targetFormat << " for " + absFileName);
                                        // normal maps:
                                        // nvdxt.exe - quality_highest - rescaleKaiser - Kaiser - dxt5nm - norm
                                        processor->compress(*srcImage, targetFormat, true, true, osgDB::ImageProcessor::USE_CPU, osgDB::ImageProcessor::PRODUCTION);
                                        SG_LOG(SG_IO, SG_ALERT, "-- finished creating DDS: " + newName);
                                        //processor->generateMipMap(*srcImage, true, osgDB::ImageProcessor::USE_CPU);
                                    }
                                    else {
                                        simgear::effect::MipMapTuple mipmapFunctions(simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE);
                                        SG_LOG(SG_IO, SG_WARN, "Texture compression plugin (osg_nvtt) not available; storing uncompressed image: " << absFileName);
                                        srcImage = simgear::effect::computeMipmap(srcImage, mipmapFunctions);
                                    }
                                }
                                else {
                                    SG_LOG(SG_IO, SG_ALERT, "Creating uncompressed DDS for " + absFileName);
                                    if (processor) {
                                        processor->generateMipMap(*srcImage, true, osgDB::ImageProcessor::USE_CPU);
                                    }
                                    else {
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
                                    FILE *f = ::fopen(mdlDirectory.c_str(), "a");
                                    if (f)
                                    {
                                        ::fprintf(f, "%s, %s\n", absFileName.c_str(), newName.c_str());
                                        ::fclose(f);
                                    }
                                }
                                absFileName = newName;
                            }
                            catch (...) {
                                SG_LOG(SG_IO, SG_ALERT, "Exception processing " << absFileName << " may be corrupted");
                            }
                        }
                        else
                            SG_LOG(SG_IO, SG_WARN, absFileName + " too small " << width << "," << height);
                    }
                }
            }
            else {
                if (newName != std::string())
                    absFileName = newName;
            }
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
//    srcImage1->setFileName(originalFileName);

    if(cache_active && getFileExtension(absFileName) != "dds")
    {
        if (processor) {
            processor->generateMipMap(*srcImage1, true, osgDB::ImageProcessor::USE_CPU);
            SG_LOG(SG_IO, SG_ALERT, "Created nvtt mipmaps DDS for " + absFileName);
        }
        else {
            simgear::effect::MipMapTuple mipmapFunctions(simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE, simgear::effect::AVERAGE);
            srcImage1 = simgear::effect::computeMipmap(srcImage1, mipmapFunctions);
            SG_LOG(SG_IO, SG_ALERT, "Created sg mipmaps DDS for " + absFileName);
        }
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
