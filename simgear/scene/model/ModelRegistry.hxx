// ModelRegistry.hxx -- interface to the OSG model registry
//
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
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>

#include <simgear/compiler.h>

#include STL_STRING
#include <map>

namespace simgear
{
class ModelRegistry : public osgDB::Registry::ReadFileCallback {
public:
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
protected:
    static osg::ref_ptr<ModelRegistry> instance;
    typedef std::map<std::string, osg::ref_ptr<osgDB::Registry::ReadFileCallback> >
    CallbackMap;
    CallbackMap imageCallbackMap;
    CallbackMap nodeCallbackMap;
};

// Proxy for registering extension-based callbacks

template<typename T>
class ModelRegistryCallbackProxy
{
public:
    ModelRegistryCallbackProxy(std::string extension)
    {
        ModelRegistry::getInstance()->addNodeCallbackForExtension(extension,
                                                                  new T);
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
