

#ifndef SG_PACKAGE_ROOT_HXX
#define SG_PACKAGE_ROOT_HXX

#include <vector>
#include <map>
#include <deque>
#include <set>

#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Delegate.hxx>

class SGPropertyNode;

namespace simgear
{
    
namespace HTTP { class Client; }
    
namespace pkg
{

// forward decls
class Package;
class Catalog;
class Install;

typedef std::vector<Package*> PackageList;
typedef std::vector<Catalog*> CatalogList;

typedef std::map<std::string, Catalog*> CatalogDict;

class Root
{
public:
    Root(const SGPath& aPath);
    virtual ~Root();
    
    SGPath path() const
        { return m_path; }
    
    void setLocale(const std::string& aLocale);
        
    void setDelegate(Delegate* aDelegate);
        
    std::string getLocale() const;
    
    CatalogList catalogs() const;
        
    void setMaxAgeSeconds(int seconds);

    void setHTTPClient(HTTP::Client* aHTTP);

    HTTP::Client* getHTTPClient() const;

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
     
    Package* getPackageById(const std::string& aId) const;
    
    Catalog* getCatalogById(const std::string& aId) const;
    
    void scheduleToUpdate(Install* aInstall);
private:
    friend class Install;
    friend class Catalog;    
    

    void catalogRefreshBegin(Catalog* aCat);
    void catalogRefreshComplete(Catalog* aCat, bool aSuccess);
        
    void startNext(Install* aCurrent);
    
    void startInstall(Install* aInstall);
    void installProgress(Install* aInstall, unsigned int aBytes, unsigned int aTotal);
    void finishInstall(Install* aInstall);    
    void failedInstall(Install* aInstall, Delegate::FailureCode aReason);
    
    SGPath m_path;
    std::string m_locale;
    HTTP::Client* m_http;
    CatalogDict m_catalogs;
    unsigned int m_maxAgeSeconds;
    Delegate* m_delegate;
    
    std::set<Catalog*> m_refreshing;
    std::deque<Install*> m_updateDeque;
};  
    
} // of namespace pkg

} // of namespace simgear

#endif // of SG_PACKAGE_ROOT_HXX
