

#include <simgear/package/Root.hxx>

#include <boost/foreach.hpp>
#include <cstring>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Catalog.hxx>

namespace simgear {
    
namespace pkg {

void Root::setMaxAgeSeconds(int seconds)
{
    m_maxAgeSeconds = seconds;
}

void Root::setHTTPClient(HTTP::Client* aHTTP)
{
    m_http = aHTTP;
}

HTTP::Client* Root::getHTTPClient() const
{
    return m_http;
}

Root::Root(const SGPath& aPath) :
    m_path(aPath),
    m_http(NULL),
    m_maxAgeSeconds(60 * 60 * 24),
    m_delegate(NULL)
{
    if (getenv("LOCALE")) {
        m_locale = getenv("LOCALE");
    }
    
    Dir d(aPath);
    if (!d.exists()) {
        d.create(0755);
        return;
    }
    
    BOOST_FOREACH(SGPath c, d.children(Dir::TYPE_DIR)) {
        Catalog* cat = Catalog::createFromPath(this, c);
        if (cat) {
           m_catalogs[cat->id()] = cat;     
        }
    } // of child directories iteration
}

Root::~Root()
{
    
}

Catalog* Root::getCatalogById(const std::string& aId) const
{
    CatalogDict::const_iterator it = m_catalogs.find(aId);
    if (it == m_catalogs.end()) {
        return NULL;
    }
    
    return it->second;
}

Package* Root::getPackageById(const std::string& aName) const
{
    size_t lastDot = aName.rfind('.');
    
    Package* pkg = NULL;
    if (lastDot == std::string::npos) {
        // naked package ID
        CatalogDict::const_iterator it = m_catalogs.begin();
        for (; it != m_catalogs.end(); ++it) {
            pkg = it->second->getPackageById(aName);
            if (pkg) {
                return pkg;
            }
        }
        
        return NULL;
    }
    
    std::string catalogId = aName.substr(0, lastDot);
    std::string id = aName.substr(lastDot + 1);    
    Catalog* catalog = getCatalogById(catalogId);
    if (!catalog) {
        return NULL;
    }
            
    return catalog->getPackageById(id);
}

CatalogList Root::catalogs() const
{
    CatalogList r;
    CatalogDict::const_iterator it = m_catalogs.begin();
    for (; it != m_catalogs.end(); ++it) {
        r.push_back(it->second);
    }
    
    return r;
}

PackageList
Root::packagesMatching(const SGPropertyNode* aFilter) const
{
    PackageList r;
    
    CatalogDict::const_iterator it = m_catalogs.begin();
    for (; it != m_catalogs.end(); ++it) {
        PackageList r2(it->second->packagesMatching(aFilter));
        r.insert(r.end(), r2.begin(), r2.end());
    }
    
    return r;
}

PackageList
Root::packagesNeedingUpdate() const
{
    PackageList r;
    
    CatalogDict::const_iterator it = m_catalogs.begin();
    for (; it != m_catalogs.end(); ++it) {
        PackageList r2(it->second->packagesNeedingUpdate());
        r.insert(r.end(), r2.begin(), r2.end());
    }
    
    return r;
}

void Root::refresh(bool aForce)
{
    CatalogDict::iterator it = m_catalogs.begin();
    for (; it != m_catalogs.end(); ++it) {
        if (aForce || (it->second->ageInSeconds() > m_maxAgeSeconds)) {
            it->second->refresh();
        }
    }
}

void Root::setLocale(const std::string& aLocale)
{
    m_locale = aLocale;
}

std::string Root::getLocale() const
{
    return m_locale;
}

void Root::scheduleToUpdate(Install* aInstall)
{
    bool wasEmpty = m_updateDeque.empty();
    m_updateDeque.push_back(aInstall);
    if (wasEmpty) {
        aInstall->startUpdate();
    }
}

void Root::startInstall(Install* aInstall)
{
    if (m_delegate) {
        m_delegate->startInstall(aInstall);
    }
}

void Root::installProgress(Install* aInstall, unsigned int aBytes, unsigned int aTotal)
{
    if (m_delegate) {
        m_delegate->installProgress(aInstall, aBytes, aTotal);
    }
}

void Root::startNext(Install* aCurrent)
{
    if (m_updateDeque.front() != aCurrent) {
        SG_LOG(SG_GENERAL, SG_ALERT, "current install of package not head of the deque");
    } else {
        m_updateDeque.pop_front();
    }
    
    if (!m_updateDeque.empty()) {
        m_updateDeque.front()->startUpdate();
    }
}

void Root::finishInstall(Install* aInstall)
{
    if (m_delegate) {
        m_delegate->finishInstall(aInstall);
    }
    
    startNext(aInstall);
}

void Root::failedInstall(Install* aInstall, Delegate::FailureCode aReason)
{
    SG_LOG(SG_GENERAL, SG_ALERT, "failed to install package:" 
        << aInstall->package()->id() << ":" << aReason);
    if (m_delegate) {
        m_delegate->failedInstall(aInstall, aReason);
    }
    
    startNext(aInstall);
}

void Root::catalogRefreshBegin(Catalog* aCat)
{
    m_refreshing.insert(aCat);
}

void Root::catalogRefreshComplete(Catalog* aCat, bool aSuccess)
{
    m_refreshing.erase(aCat);
    if (m_refreshing.empty()) {
        if (m_delegate) {
            m_delegate->refreshComplete();
        }
    }
}

} // of namespace pkg

} // of namespace simgear
