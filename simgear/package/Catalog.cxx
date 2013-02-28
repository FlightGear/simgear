

#include <simgear/package/Catalog.hxx>

#include <boost/foreach.hpp>
#include <algorithm>
#include <fstream>
#include <cstring>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/package/Install.hxx>

namespace simgear {
    
namespace pkg {

CatalogList static_catalogs;

//////////////////////////////////////////////////////////////////////////////

class Catalog::Downloader : public HTTP::Request
{
public:
    Downloader(Catalog* aOwner, const std::string& aUrl) :
        HTTP::Request(aUrl),
        m_owner(aOwner)
    {        
    }
    
protected:
    virtual void responseHeadersComplete()
    {
        
    }
    
    virtual void gotBodyData(const char* s, int n)
    {
        m_buffer += std::string(s, n);
    }
    
    virtual void responseComplete()
    {        
        if (responseCode() != 200) {
            SG_LOG(SG_GENERAL, SG_ALERT, "catalog download failure:" << m_owner->url());
            m_owner->refreshComplete(false);
            return;
        }
        
        SGPropertyNode* props = new SGPropertyNode;
        
        try {
            readProperties(m_buffer.data(), m_buffer.size(), props);
            m_owner->parseProps(props);
        } catch (sg_exception& e) {
            SG_LOG(SG_GENERAL, SG_ALERT, "catalog parse failure:" << m_owner->url());
            m_owner->refreshComplete(false);
            return;
        }
        
        // cache the catalog data, now we have a valid install root
        Dir d(m_owner->installRoot());
        SGPath p = d.file("catalog.xml");

        std::ofstream f(p.c_str(), std::ios::out | std::ios::trunc);
        f.write(m_buffer.data(), m_buffer.size());
        f.close();
        
        time(&m_owner->m_retrievedTime);
        m_owner->writeTimestamp();
        m_owner->refreshComplete(true);
    }
    
private:
    Catalog* m_owner;  
    std::string m_buffer;
};

//////////////////////////////////////////////////////////////////////////////

CatalogList Catalog::allCatalogs()
{
    return static_catalogs;
}

Catalog::Catalog(Root *aRoot) :
    m_root(aRoot),
    m_retrievedTime(0)
{
    static_catalogs.push_back(this);
}

Catalog::~Catalog()
{
    CatalogList::iterator it = std::find(static_catalogs.begin(), static_catalogs.end(), this);
    static_catalogs.erase(it);
}

Catalog* Catalog::createFromUrl(Root* aRoot, const std::string& aUrl)
{
    Catalog* c = new Catalog(aRoot);
    Downloader* dl = new Downloader(c, aUrl);
    aRoot->getHTTPClient()->makeRequest(dl);
    
    return c;
}
    
Catalog* Catalog::createFromPath(Root* aRoot, const SGPath& aPath)
{
    SGPath xml = aPath;
    xml.append("catalog.xml");
    if (!xml.exists()) {
        return NULL;
    }
    
    SGPropertyNode_ptr props;
    try {
        props = new SGPropertyNode;
        readProperties(xml.str(), props);
    } catch (sg_exception& e) {
        return NULL;    
    }
    
    Catalog* c = new Catalog(aRoot);
    c->m_installRoot = aPath;
    c->parseProps(props);
    c->parseTimestamp();
    
    return c;
}

PackageList
Catalog::packagesMatching(const SGPropertyNode* aFilter) const
{
    PackageList r;
    BOOST_FOREACH(Package* p, m_packages) {
        if (p->matches(aFilter)) {
            r.push_back(p);
        }
    }
    return r;
}

PackageList
Catalog::packagesNeedingUpdate() const
{
    PackageList r;
    BOOST_FOREACH(Package* p, m_packages) {
        if (!p->isInstalled()) {
            continue;
        }
        
        if (p->install()->hasUpdate()) {
            r.push_back(p);
        }
    }
    return r;
}

void Catalog::refresh()
{
    Downloader* dl = new Downloader(this, url());
    m_root->getHTTPClient()->makeRequest(dl);
    m_root->catalogRefreshBegin(this);
}

void Catalog::parseProps(const SGPropertyNode* aProps)
{
    // copy everything except package children?
    m_props = new SGPropertyNode;
    
    int nChildren = aProps->nChildren();
    for (int i = 0; i < nChildren; i++) {
        const SGPropertyNode* pkgProps = aProps->getChild(i);
        if (strcmp(pkgProps->getName(), "package") == 0) {
            Package* p = new Package(pkgProps, this);
            m_packages.push_back(p);   
        } else {
            SGPropertyNode* c = m_props->getChild(pkgProps->getName(), pkgProps->getIndex(), true);
            copyProperties(pkgProps, c);
        }
    } // of children iteration
    
    if (m_installRoot.isNull()) {
        m_installRoot = m_root->path();
        m_installRoot.append(id());
        
        Dir d(m_installRoot);
        d.create(0755);
    }
}

Package* Catalog::getPackageById(const std::string& aId) const
{
    BOOST_FOREACH(Package* p, m_packages) {
        if (p->id() == aId) {
            return p;
        }
    }
    
    return NULL; // not found
}

std::string Catalog::id() const
{
    return m_props->getStringValue("id");
}

std::string Catalog::url() const
{
    return m_props->getStringValue("url");
}

std::string Catalog::description() const
{
    return getLocalisedString(m_props, "description");
}

void Catalog::parseTimestamp()
{
    SGPath timestampFile = m_installRoot;
    timestampFile.append(".timestamp");
    std::ifstream f(timestampFile.c_str(), std::ios::in);
    f >> m_retrievedTime;
}

void Catalog::writeTimestamp()
{
    SGPath timestampFile = m_installRoot;
    timestampFile.append(".timestamp");
    std::ofstream f(timestampFile.c_str(), std::ios::out | std::ios::trunc);
    f << m_retrievedTime << std::endl;
}

unsigned int Catalog::ageInSeconds() const
{
    time_t now;
    time(&now);
    int diff = ::difftime(now, m_retrievedTime);
    return (diff < 0) ? 0 : diff;
}

std::string Catalog::getLocalisedString(const SGPropertyNode* aRoot, const char* aName) const
{
    if (aRoot->hasChild(m_root->getLocale())) {
        const SGPropertyNode* localeRoot = aRoot->getChild(m_root->getLocale().c_str());
        if (localeRoot->hasChild(aName)) {
            return localeRoot->getStringValue(aName);
        }
    }
    
    return aRoot->getStringValue(aName);
}

void Catalog::refreshComplete(bool aSuccess)
{
    m_root->catalogRefreshComplete(this, aSuccess);
}


} // of namespace pkg

} // of namespace simgear
