// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-File-CopyrightText: 2010 James Turner <james@flightgear.org>

/**
 * @file
 * @brief  manage finding resources by names/paths
 */

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
class ResourceManager final
{
public:
    ~ResourceManager();   // non-virtual intentional

    typedef enum {
      PRIORITY_DEFAULT = 0,
      PRIORITY_FALLBACK = -100,
      PRIORITY_NORMAL = 100,
      PRIORITY_HIGH = 1000
    } Priority;

    static ResourceManager* instance();

    static bool haveInstance();

    static void reset();

    /**
     * add a simple fixed resource location, to resolve against
     */
    void addBasePath(const SGPath& aPath, Priority aPriority = PRIORITY_DEFAULT);

    /**
     *
     */
    void addProvider(ResourceProvider* aProvider);

    void removeProvider(ResourceProvider* aProvider);

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

    virtual ~ResourceProvider();

    virtual ResourceManager::Priority priority() const
    {
      return _priority;
    }

protected:
    ResourceProvider(ResourceManager::Priority aPriority) :
      _priority(aPriority)
    {}

    ResourceManager::Priority _priority = ResourceManager::PRIORITY_DEFAULT;
};

} // of simgear namespace

#endif // of header guard
