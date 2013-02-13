

#include <simgear/package/Package.hxx>

#include <boost/foreach.hpp>

#include <simgear/debug/logstream.hxx> 
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Root.hxx>

namespace simgear {
    
namespace pkg {

Package::Package(const SGPropertyNode* aProps, Catalog* aCatalog) :
    m_catalog(aCatalog)
{
    initWithProps(aProps);
}

void Package::initWithProps(const SGPropertyNode* aProps)
{
    m_props = const_cast<SGPropertyNode*>(aProps);
// cache tag values
    BOOST_FOREACH(const SGPropertyNode* c, aProps->getChildren("tag")) {
        m_tags.insert(c->getStringValue());
    }
}

bool Package::matches(const SGPropertyNode* aFilter) const
{
    int nChildren = aFilter->nChildren();
    for (int i = 0; i < nChildren; i++) {
        const SGPropertyNode* c = aFilter->getChild(i);
        if (strutils::starts_with(c->getName(), "rating-")) {
            int minRating = c->getIntValue();
            std::string rname = c->getName() + 7;
            int ourRating = m_props->getChild("rating")->getIntValue(rname, 0);
            if (ourRating < minRating) {
                return false;
            }
        }
        
        if (strcmp(c->getName(), "tag") == 0) {
            std::string tag(c->getStringValue());
            if (m_tags.find(tag) == m_tags.end()) {
                return false;
            }
        }
        
        SG_LOG(SG_GENERAL, SG_WARN, "unknown filter term:" << c->getName());
    } // of filter props iteration
    
    return true;
}

bool Package::isInstalled() const
{
    SGPath p(m_catalog->installRoot());
    p.append("Aircraft");
    p.append(id());
    
    // anything to check for? look for a valid revision file?
    return p.exists();
}

Install* Package::install()
{
    SGPath p(m_catalog->installRoot());
    p.append("Aircraft");
    p.append(id());
    if (p.exists()) {
        return Install::createFromPath(p, m_catalog);
    }
    
    Install* ins = new Install(this, p);
    m_catalog->root()->scheduleToUpdate(ins);
    return ins;
}

std::string Package::id() const
{
    return m_props->getStringValue("id");
}

std::string Package::md5() const
{
    return m_props->getStringValue("md5");
}

unsigned int Package::revision() const
{
    return m_props->getIntValue("revision");
}

string_list Package::downloadUrls() const
{
    string_list r;
    BOOST_FOREACH(SGPropertyNode* dl, m_props->getChildren("download")) {
        r.push_back(dl->getStringValue());
    }
    return r;
}

std::string Package::getLocalisedProp(const std::string& aName) const
{
    return getLocalisedString(m_props, aName.c_str());
}

std::string Package::getLocalisedString(const SGPropertyNode* aRoot, const char* aName) const
{
    std::string locale = m_catalog->root()->getLocale();
    if (aRoot->hasChild(locale)) {
        const SGPropertyNode* localeRoot = aRoot->getChild(locale.c_str());
        if (localeRoot->hasChild(aName)) {
            return localeRoot->getStringValue(aName);
        }
    }
    
    return aRoot->getStringValue(aName);
}

} // of namespace pkg

} // of namespace simgear
