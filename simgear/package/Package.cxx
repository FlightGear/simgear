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

#include <simgear/package/Package.hxx>

#include <cassert>
#include <boost/foreach.hpp>

#include <simgear/debug/logstream.hxx> 
#include <simgear/structure/exception.hxx>

#include <simgear/package/Catalog.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Root.hxx>

namespace simgear {
    
namespace pkg {

Package::Package(const SGPropertyNode* aProps, CatalogRef aCatalog) :
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

InstallRef Package::install()
{
    SGPath p(m_catalog->installRoot());
    p.append("Aircraft");
    p.append(id());
    if (p.exists()) {
        return Install::createFromPath(p, m_catalog);
    }
    
    InstallRef ins(new Install(this, p));
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
    
std::string Package::name() const
{
    return m_props->getStringValue("name");
}

std::string Package::description() const
{
    return getLocalisedProp("decription");
}
    
SGPropertyNode* Package::properties() const
{
    return m_props.ptr();
}

string_list Package::thumbnailUrls() const
{
    string_list r;
    BOOST_FOREACH(SGPropertyNode* dl, m_props->getChildren("thumbnail")) {
        r.push_back(dl->getStringValue());
    }
    return r;
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

PackageList Package::dependencies() const
{
    PackageList result;
    
    BOOST_FOREACH(SGPropertyNode* dep, m_props->getChildren("depends")) {
        std::string depName = dep->getStringValue("package");
        unsigned int rev = dep->getIntValue("revision", 0);
        
    // prefer local hangar package if possible, in case someone does something
    // silly with naming. Of course flightgear's aircraft search doesn't know
    // about hanagrs, so names still need to be unique.
        PackageRef depPkg = m_catalog->getPackageById(depName);
        if (!depPkg) {   
            Root* rt = m_catalog->root();
            depPkg = rt->getPackageById(depName);
            if (!depPkg) {
                throw sg_exception("Couldn't satisfy dependency of " + id() + " : " + depName);
            }
        }
        
        if (depPkg->revision() < rev) {
            throw sg_range_exception("Couldn't find suitable revision of " + depName);
        }
    
    // forbid recursive dependency graphs, we don't need that level
    // of complexity for aircraft resources
        assert(depPkg->dependencies() == PackageList());
        
        result.push_back(depPkg);
    }
    
    return result;
}

} // of namespace pkg

} // of namespace simgear
