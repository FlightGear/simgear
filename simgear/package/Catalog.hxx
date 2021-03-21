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

#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/props.hxx>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/structure/function_list.hxx>
#include <simgear/io/HTTPRequest.hxx>

#include <simgear/package/Delegate.hxx>
#include <simgear/package/PackageCommon.hxx>

namespace simgear
{

namespace HTTP { class Client; }

namespace pkg
{
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
    PackageList packages(Type ty = AircraftPackage) const;

    /**
     * retrieve packages in this catalog matching a filter.
     * filter consists of required / minimum values, AND-ed together.
     */
    PackageList packagesMatching(const SGPropertyNode* aFilter) const;

    /**
     * packages which are locally installed
     */
    PackageList installedPackages(Type ty = AircraftPackage) const;

    /**
     * retrieve all the packages in the catalog which are installed
     * and have a pending update
     */
    PackageList packagesNeedingUpdate() const;

    SGPath installRoot() const
         { return m_installRoot; }

    std::string id() const;

    std::string url() const;

    /**
     * update the URL of a package. Does not trigger a refresh, but resets
     * error state if the previous URL was not found.
     */
    void setUrl(const std::string& url);

    std::string name() const;

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
    
    /**
     * is this Catalog usable? This may be false if the catalog is currently
     * failing a version check or cannot be updated
     */
    bool isEnabled() const;

    typedef std::function<void(Catalog*)> Callback;

    void addStatusCallback(const Callback& cb);

    template<class C>
    void addStatusCallback(C* instance, void (C::*mem_func)(Catalog*))
    {
      return addStatusCallback(std::bind(mem_func, instance, std::placeholders::_1));
    }
    
    bool isUserEnabled() const;
    void setUserEnabled(bool b);

    /**
     * Given a list of package IDs, mark all which exist in this package,
     * for installation. ANy packahe IDs not present in this catalog,
     * will be ignored.
     *
     * @result The number for packages newly marked for installation.
     */
    int markPackagesForInstallation(const string_list &packageIds);

    /**
     * When a catalog is added due to migration, this will contain the
     * Catalog which triggered the add. Usually this will be a catalog
     * corresponding to an earlier version.
     *
     * Note it's only valid at the time, the migration actually took place;
     * when the new catalog is loaded from disk, this value will return
     * null.
     *
     * This is intended to allow Uis to show a 'catalog was migrated'
     * feedback, when they see a catalog refresh, which has a non-null
     * value of this method.
     */
    CatalogRef migratedFrom() const;

  private:
    Catalog(Root* aRoot);

    class Downloader;
    friend class Downloader;
    friend class Root;
    
    void parseProps(const SGPropertyNode* aProps);

    void refreshComplete(Delegate::StatusCode aReason);

    void parseTimestamp();
    void writeTimestamp();

    /**
     * @brief wipe the catalog directory from the disk
     */
    bool removeDirectory();
    
    /**
     * @brief Helper to ensure all packages are at least somewhat valid, in terms
     * of an ID, name and directory.
     */
    bool validatePackages() const;
    
    std::string getLocalisedString(const SGPropertyNode* aRoot, const char* aName) const;

    void changeStatus(Delegate::StatusCode newStatus);

    void processAlternate(SGPropertyNode_ptr alt);

    PackageList packagesProviding(const Type inferredType, const std::string& path, const std::string& subpath) const;

    Root* m_root;
    SGPropertyNode_ptr m_props;
    SGPath m_installRoot;
    std::string m_url;
    Delegate::StatusCode m_status = Delegate::FAIL_UNKNOWN;
    HTTP::Request_ptr m_refreshRequest;
    bool m_userEnabled = true;
    
    PackageList m_packages;
    time_t m_retrievedTime = 0;

    typedef std::map<std::string, Package*> PackageWeakMap;
    PackageWeakMap m_variantDict;

    function_list<Callback> m_statusCallbacks;

    CatalogRef m_migratedFrom;
};

} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_CATALOG_HXX
