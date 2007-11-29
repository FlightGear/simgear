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
#ifndef _SG_MODELREGISTRY_HXX
#define _SG_MODELREGISTRY_HXX 1

#include <osg/ref_ptr>
#include <osg/Node>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>

#include <simgear/compiler.h>

#include STL_STRING
#include <map>

// Class to register per file extension read callbacks with the OSG
// registry, mostly to control caching and post load optimization /
// copying that happens above the level of the ReaderWriter.
namespace simgear
{

// Different caching and optimization strategies are needed for
// different file types. Most loaded files should be optimized and the
// optimized version should be cached. When an .osg file is
// substituted for another, it is assumed to be optimized already but
// it should be cached too (under the name of the original?). .stg
// files should not be cached (that's the pager's job) but the files
// it causes to be loaded should be. .btg files are already optimized
// and shouldn't be cached.
//
// Complicating this is the effect that removing CACHE_NODES has from
// the ReaderWriter options: it switches the object cache with an
// empty one, so that's not an option for the files that could be
// loaded from a .stg file. So, we'll let
// Registry::readNodeImplementation cache a loaded file and then add
// the optimized version to the cache ourselves, replacing the
// original subgraph.
//
// To support all these options with a minimum of duplication, the
// readNode function is specified as a template with a bunch of
// pluggable (and predefined) policies.
template <typename ProcessPolicy, typename CachePolicy, typename OptimizePolicy,
          typename CopyPolicy, typename SubstitutePolicy>
class ModelRegistryCallback : public osgDB::Registry::ReadFileCallback {
public:
    ModelRegistryCallback(const std::string& extension) :
        _processPolicy(extension), _cachePolicy(extension),
        _optimizePolicy(extension), _copyPolicy(extension),
        _substitutePolicy(extension)
    {
    }
    virtual osgDB::ReaderWriter::ReadResult
    readNode(const std::string& fileName,
             const osgDB::ReaderWriter::Options* opt)
    {
        using namespace osg;
        using namespace osgDB;
        using osgDB::ReaderWriter;
        Registry* registry = Registry::instance();
        std::string usedFileName = _substitutePolicy.substitute(fileName, opt);
        if (usedFileName.empty())
            usedFileName = fileName;
        ref_ptr<osg::Node> loadedNode = _cachePolicy.find(usedFileName, opt);
        if (!loadedNode.valid()) {
            ReaderWriter* rw = registry ->getReaderWriterForExtension(osgDB::getFileExtension(usedFileName));
            if (!rw)
                return ReaderWriter::ReadResult(); // FILE_NOT_HANDLED
            ReaderWriter::ReadResult res = rw->readNode(usedFileName, opt);
            if (!res.validNode())
                return res;
            ref_ptr<osg::Node> processedNode
                = _processPolicy.process(res.getNode(), usedFileName, opt);
            ref_ptr<osg::Node> optimizedNode
                = _optimizePolicy.optimize(processedNode.get(), usedFileName,
                                           opt);
            _cachePolicy.addToCache(usedFileName, optimizedNode.get());
            loadedNode = optimizedNode;
        }
        return ReaderWriter::ReadResult(_copyPolicy.copy(loadedNode.get(),
                                                         usedFileName,
                                                         opt));
    }
protected:
    ProcessPolicy _processPolicy;
    CachePolicy _cachePolicy;
    OptimizePolicy _optimizePolicy;
    CopyPolicy _copyPolicy;
    SubstitutePolicy _substitutePolicy;
    virtual ~ModelRegistryCallback() {}
};

// Predefined policies

struct DefaultProcessPolicy {
    DefaultProcessPolicy(const std::string& extension) {}
    osg::Node* process(osg::Node* node, const std::string& filename,
                       const osgDB::ReaderWriter::Options* opt)
    {
        return node;
    }
};

struct DefaultCachePolicy {
    DefaultCachePolicy(const std::string& extension) {}
    osg::Node* find(const std::string& fileName,
                    const osgDB::ReaderWriter::Options* opt);
    void addToCache(const std::string& filename, osg::Node* node);
};

struct NoCachePolicy {
    NoCachePolicy(const std::string& extension) {}
    osg::Node* find(const std::string& fileName,
                    const osgDB::ReaderWriter::Options* opt)
    {
        return 0;
    }
    void addToCache(const std::string& filename, osg::Node* node) {}
};

class OptimizeModelPolicy {
public:
    OptimizeModelPolicy(const std::string& extension);
    osg::Node* optimize(osg::Node* node, const std::string& fileName,
                        const osgDB::ReaderWriter::Options* opt);
protected:
    unsigned _osgOptions;
};

struct NoOptimizePolicy {
    NoOptimizePolicy(const std::string& extension) {}
    osg::Node* optimize(osg::Node* node, const std::string& fileName,
                        const osgDB::ReaderWriter::Options* opt)
    {
        return node;
    }
};

struct DefaultCopyPolicy {
    DefaultCopyPolicy(const std::string& extension) {}
    osg::Node* copy(osg::Node* node, const std::string& fileName,
                    const osgDB::ReaderWriter::Options* opt);
};

struct NoCopyPolicy {
    NoCopyPolicy(const std::string& extension) {}
    osg::Node* copy(osg::Node* node, const std::string& fileName,
                    const osgDB::ReaderWriter::Options* opt)
    {
        return node;
    }
};

struct OSGSubstitutePolicy {
    OSGSubstitutePolicy(const std::string& extension) {}
    std::string substitute(const std::string& name,
                           const osgDB::ReaderWriter::Options* opt);
};

struct NoSubstitutePolicy {
    NoSubstitutePolicy(const std::string& extension) {}
    std::string substitute(const std::string& name,
                           const osgDB::ReaderWriter::Options* opt)
    {
        return std::string();
    }
};
typedef ModelRegistryCallback<DefaultProcessPolicy, DefaultCachePolicy,
                              OptimizeModelPolicy, DefaultCopyPolicy,
                              OSGSubstitutePolicy> DefaultCallback;

// The manager for the callbacks
class ModelRegistry : public osgDB::Registry::ReadFileCallback {
public:
    ModelRegistry();
    virtual osgDB::ReaderWriter::ReadResult
    readImage(const std::string& fileName,
              const osgDB::ReaderWriter::Options* opt);
    virtual osgDB::ReaderWriter::ReadResult
    readNode(const std::string& fileName,
             const osgDB::ReaderWriter::Options* opt);
    void addImageCallbackForExtension(const std::string& extension,
                                      osgDB::Registry::ReadFileCallback*
                                      callback);
    void addNodeCallbackForExtension(const std::string& extension,
                                     osgDB::Registry::ReadFileCallback*
                                     callback);
    static ModelRegistry* getInstance();
    virtual ~ModelRegistry() {}
protected:
    static osg::ref_ptr<ModelRegistry> instance;
    typedef std::map<std::string, osg::ref_ptr<osgDB::Registry::ReadFileCallback> >
    CallbackMap;
    CallbackMap imageCallbackMap;
    CallbackMap nodeCallbackMap;
    osg::ref_ptr<DefaultCallback> _defaultCallback;
};

// Callback that only loads the file without any caching or
// postprocessing.
typedef ModelRegistryCallback<DefaultProcessPolicy, NoCachePolicy,
                              NoOptimizePolicy, NoCopyPolicy,
                              NoSubstitutePolicy> LoadOnlyCallback;

// Proxy for registering extension-based callbacks

template<typename T>
class ModelRegistryCallbackProxy
{
public:
    ModelRegistryCallbackProxy(std::string extension)
    {
        ModelRegistry::getInstance()
            ->addNodeCallbackForExtension(extension, new T(extension));
    }
};

// Callback for file extensions that load files using the default OSG
// implementation.

class OSGFileCallback : public osgDB::Registry::ReadFileCallback {
public:
    virtual osgDB::ReaderWriter::ReadResult
    readImage(const std::string& fileName,
              const osgDB::ReaderWriter::Options* opt);
    virtual osgDB::ReaderWriter::ReadResult
    readNode(const std::string& fileName,
             const osgDB::ReaderWriter::Options* opt);
};

}
#endif // _SG_MODELREGISTRY_HXX
