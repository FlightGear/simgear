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

#ifndef SG_PACKAGE_ROOT_HXX
#define SG_PACKAGE_ROOT_HXX

#include <vector>
#include <memory> // for unique_ptr

#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Delegate.hxx>
#include <simgear/package/PackageCommon.hxx>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

class SGPropertyNode;

namespace simgear
{
    
namespace HTTP {
    class Client;
    class Request;
}
    
namespace pkg
{

class Root : public SGReferenced
{
public:
    Root(const SGPath& aPath, const std::string& aVersion);
    virtual ~Root();
    
    SGPath path() const;
    
    void setLocale(const std::string& aLocale);
        
    void addDelegate(Delegate* aDelegate);
    
    void removeDelegate(Delegate* aDelegate);
    
    std::string getLocale() const;
    
    CatalogList catalogs() const;
    
    /**
     * retrive all catalogs, including currently disabled ones
     */
    CatalogList allCatalogs() const;
    
    
    void setMaxAgeSeconds(unsigned int seconds);
    unsigned int maxAgeSeconds() const;
    
    void setHTTPClient(HTTP::Client* aHTTP);

    /**
     * Submit an HTTP request. The Root may delay or queue requests if it needs
     * too, for example during startup when the HTTP engine may not have been
     * set yet.
     */
    void makeHTTPRequest(HTTP::Request* req);

    /**
     * Cancel an HTTP request.
     */
    void cancelHTTPRequest(HTTP::Request* req, const std::string& reason);
    
    /**
     * The catalog XML/property version in use. This is used to make incomaptible
     * changes to the package/catalog syntax
     */
    int catalogVersion() const;

    /**
     * the version string of the application. Catalogs must match this version,
     * or they will be ignored / rejected.
     */
    std::string applicationVersion() const;

    /**
     * refresh catalogs which are more than the maximum age (24 hours by default)
     * set force to true, to download all catalogs regardless of age.
     */
    void refresh(bool aForce = false);

    /**
     *
     */
    PackageList allPackages(Type ty = AircraftPackage) const;

    /**
     * retrieve packages matching a filter.
     * filter consists of required / minimum values, AND-ed together.
     */
    PackageList packagesMatching(const SGPropertyNode* aFilter) const;
    
    /**
     * retrieve all the packages which are installed
     * and have a pending update
     */ 
    PackageList packagesNeedingUpdate() const;
     
    PackageRef getPackageById(const std::string& aId) const;
    
    CatalogRef getCatalogById(const std::string& aId) const;
    
    CatalogRef getCatalogByUrl(const std::string& aUrl) const;
    
    void scheduleToUpdate(InstallRef aInstall);
    
    /**
     * remove a catalog. Will uninstall all packages originating
     * from the catalog too.
     */
    bool removeCatalogById(const std::string& aId);
    
    /**
     * remove a catalog by reference (used when abandoning installs, since
     * there may not be a valid catalog Id)
     */
    bool removeCatalog(CatalogRef cat);

    
    /**
     * request thumbnail data from the cache / network
     */
    void requestThumbnailData(const std::string& aUrl);
    
    bool isInstallQueued(InstallRef aInstall) const;

    /**
     * Mark all 'to be updated' packages for update now
     */
    void scheduleAllUpdates();

    /**
     * @brief list of catalog IDs, the user has explicitly removed via
     * removeCatalogById(). This is important to allow the user to opt-out
     * of migrated packages.
     *
     * This information is stored in a helper file, in the root directory
     */
    string_list explicitlyRemovedCatalogs() const;

    /**
     * @brief Given a relative path to a file, return the packages which provide it.
     * If the path starts with a type-based prefix (eg 'Aircraft' or 'AI/Aircraft'), the
     * corresponding package type will be considered. The next item must correspond to
     * the package directory name.
     * 
     * If the path contains components more specific than this, they will be checked
     * against matching packkages 'provides' list. If the path does not contain such
     * components, a match of the type+directory name will be considered sufficient.
     * 
     * @param path 
     * @return PackageList 
     */
    PackageList packagesProviding(const std::string& path, bool onlyInstalled) const;

private:
    friend class Install;
    friend class Catalog;    
    friend class Package;
    
    InstallRef existingInstallForPackage(PackageRef p) const;
    
    void catalogRefreshStatus(CatalogRef aCat, Delegate::StatusCode aReason);
        
    void startNext(InstallRef aCurrent);
    
    void startInstall(InstallRef aInstall);
    void installProgress(InstallRef aInstall, unsigned int aBytes, unsigned int aTotal);
    void finishInstall(InstallRef aInstall, Delegate::StatusCode aReason);
    void cancelDownload(InstallRef aInstall);
    
    void registerInstall(InstallRef ins);
    void unregisterInstall(InstallRef ins);
    
    class ThumbnailDownloader;
    class RootPrivate;
    std::unique_ptr<RootPrivate> d;
};
  
typedef SGSharedPtr<Root> RootRef;
  
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_ROOT_HXX
