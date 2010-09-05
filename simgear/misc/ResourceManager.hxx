// ResourceManager.hxx -- manage finding resources by names/paths
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


#ifndef SG_RESOURCE_MANAGER_HXX
#define SG_RESOURCE_MANAGER_HXX

#include <vector>

#include <simgear/misc/sg_path.hxx>

namespace simgear
{

class ResourceProvider;

/**
 * singleton management of resources
 */
class ResourceManager
{
public:
    typedef enum {
      PRIORITY_DEFAULT = 0,
      PRIORITY_FALLBACK = -100,
      PRIORITY_NORMAL = 100,
      PRIORITY_HIGH = 1000
    } Priority;

    static ResourceManager* instance();    
    
    /**
     * add a simple fixed resource location, to resolve against
     */
    void addBasePath(const SGPath& aPath, Priority aPriority = PRIORITY_DEFAULT);
    
    /**
     *
     */
    void addProvider(ResourceProvider* aProvider);
    
    /**
     * given a resource name (or path), find the appropriate real resource
     * path.
     * @param aContext an optional current location to resolve relative names
     *   against (e.g a current directory)
     */
    SGPath findPath(const std::string& aResource, SGPath aContext = SGPath());
    
private:
    ResourceManager();
    
    typedef std::vector<ResourceProvider*> ProviderVec;
    ProviderVec _providers;
};      
    
class ResourceProvider
{
public:
    virtual SGPath resolve(const std::string& aResource, SGPath& aContext) const = 0;
    
    virtual int priority() const
    {
      return _priority;
    }
    
protected:
    ResourceProvider(int aPriority) :
      _priority(aPriority)
    {}
    
    int _priority;
};
    
} // of simgear namespace

#endif // of header guard
