// Copyright (C) 2013  James Turner - zakalawe@mac.com
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

#ifndef SG_PACKAGE_CATALOG_HXX
#define SG_PACKAGE_CATALOG_HXX

#include <vector>
#include <ctime>
#include <map>

#include <boost/bind.hpp>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/function_list.hxx>

#include <simgear/package/Delegate.hxx>

namespace simgear
{

namespace HTTP { class Client; }

namespace pkg
{

// forward decls
class Package;
class Catalog;
class Root;
class Install;

typedef SGSharedPtr<Package> PackageRef;
typedef SGSharedPtr<Catalog> CatalogRef;
typedef SGSharedPtr<Install> InstallRef;

typedef std::vector<PackageRef> PackageList;
typedef std::vector<CatalogRef> CatalogList;

class Catalog : public SGReferenced
{
public:
    virtual ~Catalog();

    static CatalogRef createFromUrl(Root* aRoot, const std::string& aUrl);

    static CatalogRef createFromPath(Root* aRoot, const SGPath& aPath);

    Root* root() const
        { return m_root;};

    /**
     * uninstall this catalog entirely, including all installed packages
     */
    bool uninstall();

    /**
     * perform a refresh of the catalog contents
     */
    void refresh();

    /**
     * Get all packages in this catalog.
     */
    PackageList const& packages() const;

    /**
     * retrieve packages in this catalog matching a filter.
     * filter consists of required / minimum values, AND-ed together.
     */
    PackageList packagesMatching(const SGPropertyNode* aFilter) const;

    /**
     * packages which are locally installed
     */
    PackageList installedPackages() const;

    /**
     * retrieve all the packages in the catalog which are installed
     * and have a pendig update
     */
    PackageList packagesNeedingUpdate() const;

    SGPath installRoot() const
         { return m_installRoot; }

    std::string id() const;

    std::string url() const;

    std::string description() const;

    PackageRef getPackageById(const std::string& aId) const;

    PackageRef getPackageByPath(const std::string& aPath) const;

    /**
     * test if the catalog data was retrieved longer ago than the
     * maximum permitted age for this catalog.
     */
    bool needsRefresh() const;

    unsigned int ageInSeconds() const;

    /**
     * access the raw property data in the catalog
     */
    SGPropertyNode* properties() const;

    Delegate::StatusCode status() const;

    typedef boost::function<void(Catalog*)> Callback;

    void addStatusCallback(const Callback& cb);

    template<class C>
    void addStatusCallback(C* instance, void (C::*mem_func)(Catalog*))
    {
      return addStatusCallback(boost::bind(mem_func, instance, _1));
    }
private:
    Catalog(Root* aRoot);

    class Downloader;
    friend class Downloader;

    void parseProps(const SGPropertyNode* aProps);

    void refreshComplete(Delegate::StatusCode aReason);

    void parseTimestamp();
    void writeTimestamp();

    std::string getLocalisedString(const SGPropertyNode* aRoot, const char* aName) const;

    void changeStatus(Delegate::StatusCode newStatus);

    Root* m_root;
    SGPropertyNode_ptr m_props;
    SGPath m_installRoot;
    std::string m_url;
    Delegate::StatusCode m_status;

    PackageList m_packages;
    time_t m_retrievedTime;

    typedef std::map<std::string, Package*> PackageWeakMap;
    PackageWeakMap m_variantDict;

    function_list<Callback> m_statusCallbacks;
};

} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_CATALOG_HXX
