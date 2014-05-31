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
#include <memory> // for auto_ptr

#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Delegate.hxx>

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

// forward decls
class Package;
class Catalog;
class Install;
  
typedef SGSharedPtr<Package> PackageRef;
typedef SGSharedPtr<Catalog> CatalogRef;
typedef SGSharedPtr<Install> InstallRef;
  
typedef std::vector<PackageRef> PackageList;
typedef std::vector<CatalogRef> CatalogList;

class Root : public SGReferenced
{
public:
    Root(const SGPath& aPath, const std::string& aVersion);
    virtual ~Root();
    
    SGPath path() const;
    
    void setLocale(const std::string& aLocale);
        
    void setDelegate(Delegate* aDelegate);
        
    std::string getLocale() const;
    
    CatalogList catalogs() const;
        
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
     * the version string of the root. Catalogs must match this version,
     * or they will be ignored / rejected.
     */
    std::string catalogVersion() const;
    
    /**
     * refresh catalogs which are more than the maximum age (24 hours by default)
     * set force to true, to download all catalogs regardless of age.
     */
    void refresh(bool aForce = false);

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
    
    void scheduleToUpdate(InstallRef aInstall);
private:
    friend class Install;
    friend class Catalog;    
    

    void catalogRefreshBegin(CatalogRef aCat);
    void catalogRefreshComplete(CatalogRef aCat, Delegate::FailureCode aReason);
        
    void startNext(InstallRef aCurrent);
    
    void startInstall(InstallRef aInstall);
    void installProgress(InstallRef aInstall, unsigned int aBytes, unsigned int aTotal);
    void finishInstall(InstallRef aInstall);
    void failedInstall(InstallRef aInstall, Delegate::FailureCode aReason);

    class RootPrivate;
    std::auto_ptr<RootPrivate> d;
};
  
typedef SGSharedPtr<Root> RootRef;
  
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_ROOT_HXX
