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
#include <simgear/scene/util/OsgSingleton.hxx>

#include <string>
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
          typename SubstitutePolicy, typename BVHPolicy>
class ModelRegistryCallback : public osgDB::Registry::ReadFileCallback {
public:
    ModelRegistryCallback(const std::string& extension) :
        _processPolicy(extension), _cachePolicy(extension),
        _optimizePolicy(extension),
        _substitutePolicy(extension), _bvhPolicy(extension)
    {
    }
    virtual osgDB::ReaderWriter::ReadResult
    readNode(const std::string& fileName,
             const osgDB::Options* opt)
    {
        using namespace osg;
        using namespace osgDB;
        using osgDB::ReaderWriter;
//        Registry* registry = Registry::instance();
        ref_ptr<osg::Node> optimizedNode = _cachePolicy.find(fileName, opt);
        if (!optimizedNode.valid()) {
            std::string otherFileName = _substitutePolicy.substitute(fileName,
                                                                     opt);
            ReaderWriter::ReadResult res;
            if (!otherFileName.empty()) {
                res = loadUsingReaderWriter(otherFileName, opt);
                if (res.validNode())
                    optimizedNode = res.getNode();
            }
            if (!optimizedNode.valid()) {
                res = loadUsingReaderWriter(fileName, opt);
                if (!res.validNode())
                    return res;
                ref_ptr<osg::Node> processedNode
                    = _processPolicy.process(res.getNode(), fileName, opt);
                optimizedNode = _optimizePolicy.optimize(processedNode.get(),
                                                         fileName, opt);
            }
            if (opt->getPluginStringData("SimGear::BOUNDINGVOLUMES") != "OFF")
                _bvhPolicy.buildBVH(fileName, optimizedNode.get());
            _cachePolicy.addToCache(fileName, optimizedNode.get());
        }
        return ReaderWriter::ReadResult(optimizedNode.get());
    }
protected:
    static osgDB::ReaderWriter::ReadResult
    loadUsingReaderWriter(const std::string& fileName,
                          const osgDB::Options* opt)
    {
        using namespace osgDB;
        ReaderWriter* rw = Registry::instance()
            ->getReaderWriterForExtension(osgDB::getFileExtension(fileName));
        if (!rw)
            return ReaderWriter::ReadResult(); // FILE_NOT_HANDLED
        return rw->readNode(fileName, opt);
    }
    
    ProcessPolicy _processPolicy;
    CachePolicy _cachePolicy;
    OptimizePolicy _optimizePolicy;
    SubstitutePolicy _substitutePolicy;
    BVHPolicy _bvhPolicy;
    virtual ~ModelRegistryCallback() {}
};

// Predefined policies

struct DefaultProcessPolicy {
    DefaultProcessPolicy(const std::string& extension) {}
    osg::Node* process(osg::Node* node, const std::string& filename,
                       const osgDB::Options* opt);
};

struct DefaultCachePolicy {
    DefaultCachePolicy(const std::string& extension) {}
    osg::Node* find(const std::string& fileName,
                    const osgDB::Options* opt);
    void addToCache(const std::string& filename, osg::Node* node);
};

struct NoCachePolicy {
    NoCachePolicy(const std::string& extension) {}
    osg::Node* find(const std::string& fileName,
                    const osgDB::Options* opt)
    {
        return 0;
    }
    void addToCache(const std::string& filename, osg::Node* node) {}
};

class OptimizeModelPolicy {
public:
    OptimizeModelPolicy(const std::string& extension);
    osg::Node* optimize(osg::Node* node, const std::string& fileName,
                        const osgDB::Options* opt);
protected:
    unsigned _osgOptions;
};

struct NoOptimizePolicy {
    NoOptimizePolicy(const std::string& extension) {}
    osg::Node* optimize(osg::Node* node, const std::string& fileName,
                        const osgDB::Options* opt)
    {
        return node;
    }
};

struct OSGSubstitutePolicy {
    OSGSubstitutePolicy(const std::string& extension) {}
    std::string substitute(const std::string& name,
                           const osgDB::Options* opt);
};

struct NoSubstitutePolicy {
    NoSubstitutePolicy(const std::string& extension) {}
    std::string substitute(const std::string& name,
                           const osgDB::Options* opt)
    {
        return std::string();
    }
};

struct BuildLeafBVHPolicy {
    BuildLeafBVHPolicy(const std::string& extension) {}
    void buildBVH(const std::string& fileName, osg::Node* node);
};

struct BuildGroupBVHPolicy {
    BuildGroupBVHPolicy(const std::string& extension) {}
    void buildBVH(const std::string& fileName, osg::Node* node);
};

struct NoBuildBVHPolicy {
    NoBuildBVHPolicy(const std::string& extension) {}
    void buildBVH(const std::string& fileName, osg::Node* node);
};

typedef ModelRegistryCallback<DefaultProcessPolicy, DefaultCachePolicy,
                              OptimizeModelPolicy,
                              OSGSubstitutePolicy, BuildLeafBVHPolicy>
DefaultCallback;

// The manager for the callbacks
class ModelRegistry : public osgDB::Registry::ReadFileCallback,
                      public ReferencedSingleton<ModelRegistry> {
public:
    ModelRegistry();
    virtual osgDB::ReaderWriter::ReadResult
    readImage(const std::string& fileName,
              const osgDB::Options* opt);
    virtual osgDB::ReaderWriter::ReadResult
    readNode(const std::string& fileName,
             const osgDB::Options* opt);
    void addImageCallbackForExtension(const std::string& extension,
                                      osgDB::Registry::ReadFileCallback*
                                      callback);
    void addNodeCallbackForExtension(const std::string& extension,
                                     osgDB::Registry::ReadFileCallback*
                                     callback);
    virtual ~ModelRegistry() {}
protected:
    typedef std::map<std::string, osg::ref_ptr<osgDB::Registry::ReadFileCallback> >
    CallbackMap;
    CallbackMap imageCallbackMap;
    CallbackMap nodeCallbackMap;
    osg::ref_ptr<DefaultCallback> _defaultCallback;
};

// Callback that only loads the file without any caching or
// postprocessing.
typedef ModelRegistryCallback<DefaultProcessPolicy, NoCachePolicy,
                              NoOptimizePolicy,
                              NoSubstitutePolicy, BuildLeafBVHPolicy>
LoadOnlyCallback;

// Proxy for registering extension-based callbacks

template<typename T>
class ModelRegistryCallbackProxy
{
public:
    ModelRegistryCallbackProxy(std::string extension)
    {
        ModelRegistry::instance()
            ->addNodeCallbackForExtension(extension, new T(extension));
    }
};
}
#endif // _SG_MODELREGISTRY_HXX
