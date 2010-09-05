// ResourceManager.cxx -- manage finding resources by names/paths
// Copyright (C) 2010 James Turner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#include <simgear_config.h>

#include <simgear/misc/ResourceManager.hxx>

namespace simgear
{
    
static ResourceManager* static_manager = NULL;

ResourceManager::ResourceManager()
{
    
}

ResourceManager* ResourceManager::instance()
{
    if (!static_manager) {
        static_manager = new ResourceManager();
    }
    
    return static_manager;
}    

/**
 * trivial provider using a fixed base path
 */
class BasePathProvider : public ResourceProvider
{
public:
    BasePathProvider(const SGPath& aBase, int aPriority) :
        ResourceProvider(aPriority),
        _base(aBase)
    {
        
    }
    
    virtual SGPath resolve(const std::string& aResource, SGPath&) const
    {
        SGPath p(_base, aResource);
        return p.exists() ? p : SGPath();
    }
private:
    SGPath _base;  
};

void ResourceManager::addBasePath(const SGPath& aPath, Priority aPriority)
{
    addProvider(new BasePathProvider(aPath, aPriority));
}

void ResourceManager::addProvider(ResourceProvider* aProvider)
{
    ProviderVec::iterator it = _providers.begin();
    for (; it != _providers.end(); ++it) {
      if (aProvider->priority() > (*it)->priority()) {
        _providers.insert(it, aProvider);
        return;
      }
    }
    
    // fell out of the iteration, goes to the end of the vec
    _providers.push_back(aProvider);
}

SGPath ResourceManager::findPath(const std::string& aResource, SGPath aContext)
{
    if (!aContext.isNull()) {
        SGPath r(aContext, aResource);
        if (r.exists()) {
            return r;
        }
    }
    
    ProviderVec::iterator it = _providers.begin();
    for (; it != _providers.end(); ++it) {
      SGPath path = (*it)->resolve(aResource, aContext);
      if (!path.isNull()) {
        return path;
      }
    }
    
    return SGPath();
}

} // of namespace simgear
