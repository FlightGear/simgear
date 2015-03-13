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

bool checkVersion(const std::string& aVersion, SGPropertyNode_ptr props)
{
    BOOST_FOREACH(SGPropertyNode* v, props->getChildren("version")) {
        if (v->getStringValue() == aVersion) {
            return true;
        }
    }
    return false;
}

std::string redirectUrlForVersion(const std::string& aVersion, SGPropertyNode_ptr props)
{
    BOOST_FOREACH(SGPropertyNode* v, props->getChildren("alternate-version")) {
        if (v->getStringValue("version") == aVersion) {
            return v->getStringValue("url");
        }
    }
    
    return std::string();
}

//////////////////////////////////////////////////////////////////////////////

class Catalog::Downloader : public HTTP::Request
{
public:
    Downloader(CatalogRef aOwner, const std::string& aUrl) :
        HTTP::Request(aUrl),
        m_owner(aOwner)
    {
        // refreshing
        m_owner->changeStatus(Delegate::FAIL_IN_PROGRESS);

    }

protected:
    virtual void gotBodyData(const char* s, int n)
    {
        m_buffer += std::string(s, n);
    }

    virtual void onDone()
    {
        if (responseCode() != 200) {
            SG_LOG(SG_GENERAL, SG_ALERT, "catalog download failure:" << m_owner->url());
            m_owner->refreshComplete(Delegate::FAIL_DOWNLOAD);
            return;
        }

        SGPropertyNode* props = new SGPropertyNode;

        try {
            readProperties(m_buffer.data(), m_buffer.size(), props);
            m_owner->parseProps(props);
        } catch (sg_exception& e) {
            SG_LOG(SG_GENERAL, SG_ALERT, "catalog parse failure:" << m_owner->url());
            m_owner->refreshComplete(Delegate::FAIL_EXTRACT);
            return;
        }

        std::string ver(m_owner->root()->catalogVersion());
        if (!checkVersion(ver, props)) {
            SG_LOG(SG_GENERAL, SG_WARN, "downloaded catalog " << m_owner->url() << ", version mismatch:\n\t"
                   << props->getStringValue("version") << " vs required " << ver);

            // check for a version redirect entry
            std::string url = redirectUrlForVersion(ver, props);
            if (!url.empty()) {
                SG_LOG(SG_GENERAL, SG_WARN, "redirecting from " << m_owner->url() <<
                       " to \n\t" << url);

                // update the URL and kick off a new request
                m_owner->m_url = url;
                Downloader* dl = new Downloader(m_owner, url);
                m_owner->root()->makeHTTPRequest(dl);
            } else {
                m_owner->refreshComplete(Delegate::FAIL_VERSION);
            }

            return;
        } // of version check failed

        // cache the catalog data, now we have a valid install root
        Dir d(m_owner->installRoot());
        SGPath p = d.file("catalog.xml");

        std::ofstream f(p.c_str(), std::ios::out | std::ios::trunc);
        f.write(m_buffer.data(), m_buffer.size());
        f.close();

        time(&m_owner->m_retrievedTime);
        m_owner->writeTimestamp();
        m_owner->refreshComplete(Delegate::CATALOG_REFRESHED);
    }

private:
    bool checkVersion(const std::string& aVersion, SGPropertyNode* aProps)
    {
        BOOST_FOREACH(SGPropertyNode* v, aProps->getChildren("version")) {
            if (v->getStringValue() == aVersion) {
                return true;
            }
        }
        return false;
    }

    CatalogRef m_owner;
    std::string m_buffer;
};

//////////////////////////////////////////////////////////////////////////////

Catalog::Catalog(Root *aRoot) :
    m_root(aRoot),
    m_status(Delegate::FAIL_UNKNOWN),
    m_retrievedTime(0)
{
}

Catalog::~Catalog()
{
}

CatalogRef Catalog::createFromUrl(Root* aRoot, const std::string& aUrl)
{
    CatalogRef c = new Catalog(aRoot);
    c->m_url = aUrl;
    Downloader* dl = new Downloader(c, aUrl);
    aRoot->makeHTTPRequest(dl);

    return c;
}

CatalogRef Catalog::createFromPath(Root* aRoot, const SGPath& aPath)
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

    if (props->getStringValue("version") != aRoot->catalogVersion()) {
        std::string redirect = redirectUrlForVersion(aRoot->catalogVersion(), props);
        if (!redirect.empty()) {
            SG_LOG(SG_GENERAL, SG_WARN, "catalog at " << aPath << ", version mismatch:\n\t"
                   << "redirecting to alternate URL:" << redirect);
            CatalogRef c = Catalog::createFromUrl(aRoot, redirect);
            c->m_installRoot = aPath;
            return c;
        } else {
            SG_LOG(SG_GENERAL, SG_WARN, "skipping catalog at " << aPath << ", version mismatch:\n\t"
               << props->getStringValue("version") << " vs required " << aRoot->catalogVersion());
            return NULL;
        }

    }

    CatalogRef c = new Catalog(aRoot);
    c->m_installRoot = aPath;
    c->parseProps(props); // will set status
    c->parseTimestamp();

    return c;
}

bool Catalog::uninstall()
{
    bool ok;
    bool atLeastOneFailure = false;
    
    BOOST_FOREACH(PackageRef p, installedPackages()) {
        ok = p->existingInstall()->uninstall();
        if (!ok) {
            SG_LOG(SG_GENERAL, SG_WARN, "uninstall of package " <<
                p->id() << " failed");
            // continue trying other packages, bailing out here
            // gains us nothing
            atLeastOneFailure = true;
        }
    }

    Dir d(m_installRoot);
    ok = d.remove(true /* recursive */);
    if (!ok) {
        atLeastOneFailure = true;
    }
    
    changeStatus(atLeastOneFailure ? Delegate::FAIL_FILESYSTEM
                                   : Delegate::FAIL_SUCCESS);
    return ok;
}

PackageList const&
Catalog::packages() const
{
  return m_packages;
}

PackageList
Catalog::packagesMatching(const SGPropertyNode* aFilter) const
{
    PackageList r;
    BOOST_FOREACH(PackageRef p, m_packages) {
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
    BOOST_FOREACH(PackageRef p, m_packages) {
        if (!p->isInstalled()) {
            continue;
        }

        if (p->install()->hasUpdate()) {
            r.push_back(p);
        }
    }
    return r;
}

PackageList
Catalog::installedPackages() const
{
  PackageList r;
  BOOST_FOREACH(PackageRef p, m_packages) {
    if (p->isInstalled()) {
      r.push_back(p);
    }
  }
  return r;
}

InstallRef Catalog::installForPackage(PackageRef pkg) const
{
    PackageInstallDict::const_iterator it = m_installed.find(pkg);
    if (it == m_installed.end()) {
        // check if it exists on disk, create

        SGPath p(pkg->pathOnDisk());
        if (p.exists()) {
            return Install::createFromPath(p, CatalogRef(const_cast<Catalog*>(this)));
        }

        return NULL;
    }

    return it->second;
}

void Catalog::refresh()
{
    Downloader* dl = new Downloader(this, url());
    // will iupdate status to IN_PROGRESS
    m_root->makeHTTPRequest(dl);
    m_root->catalogRefreshBegin(this);
}

void Catalog::parseProps(const SGPropertyNode* aProps)
{
    // copy everything except package children?
    m_props = new SGPropertyNode;

    m_variantDict.clear(); // will rebuild during parse
    std::set<PackageRef> orphans;
    orphans.insert(m_packages.begin(), m_packages.end());

    int nChildren = aProps->nChildren();
    for (int i = 0; i < nChildren; i++) {
        const SGPropertyNode* pkgProps = aProps->getChild(i);
        if (strcmp(pkgProps->getName(), "package") == 0) {
            PackageRef p = getPackageById(pkgProps->getStringValue("id"));
            if (p) {
                // existing package
                p->updateFromProps(pkgProps);
                orphans.erase(p); // not an orphan
            } else {
                // new package
                p = new Package(pkgProps, this);
                m_packages.push_back(p);
            }

            string_list vars(p->variants());
            for (string_list::iterator it = vars.begin(); it != vars.end(); ++it) {
                m_variantDict[*it] = p.ptr();
            }
        } else {
            SGPropertyNode* c = m_props->getChild(pkgProps->getName(), pkgProps->getIndex(), true);
            copyProperties(pkgProps, c);
        }
    } // of children iteration

    if (!orphans.empty()) {
        SG_LOG(SG_GENERAL, SG_WARN, "have orphan packages: will become inaccesible");
        std::set<PackageRef>::iterator it;
        for (it = orphans.begin(); it != orphans.end(); ++it) {
            SG_LOG(SG_GENERAL, SG_WARN, "\torphan package:" << (*it)->qualifiedId());
            PackageList::iterator pit = std::find(m_packages.begin(), m_packages.end(), *it);
            assert(pit != m_packages.end());
            m_packages.erase(pit);
        }
    }

    if (!m_url.empty()) {
        if (m_url != m_props->getStringValue("url")) {
            // this effectively allows packages to migrate to new locations,
            // although if we're going to rely on that feature we should
            // maybe formalise it!
            SG_LOG(SG_GENERAL, SG_WARN, "package downloaded from:" << m_url
                   << " is now at: " << m_props->getStringValue("url"));
        }
    }

    m_url = m_props->getStringValue("url");

    if (m_installRoot.isNull()) {
        m_installRoot = m_root->path();
        m_installRoot.append(id());

        Dir d(m_installRoot);
        d.create(0755);
    }
    
    // parsed XML ok, mark status as valid
    changeStatus(Delegate::FAIL_SUCCESS);
}

PackageRef Catalog::getPackageById(const std::string& aId) const
{
    // search the variant dict here, so looking up aircraft variants
    // works as expected.
    PackageWeakMap::const_iterator it = m_variantDict.find(aId);
    if (it == m_variantDict.end())
        return NULL;

    return it->second;
}

std::string Catalog::id() const
{
    return m_props->getStringValue("id");
}

std::string Catalog::url() const
{
    return m_url;
}

std::string Catalog::description() const
{
    return getLocalisedString(m_props, "description");
}

SGPropertyNode* Catalog::properties() const
{
    return m_props.ptr();
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

bool Catalog::needsRefresh() const
{
    unsigned int maxAge = m_props->getIntValue("max-age-sec", m_root->maxAgeSeconds());
    return (ageInSeconds() > maxAge);
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

void Catalog::refreshComplete(Delegate::FailureCode aReason)
{
    m_root->catalogRefreshComplete(this, aReason);
    changeStatus(aReason);
}

void Catalog::registerInstall(Install* ins)
{
  if (!ins || ins->package()->catalog() != this) {
    return;
  }

  m_installed[ins->package()] = ins;
}

void Catalog::unregisterInstall(Install* ins)
{
  if (!ins || ins->package()->catalog() != this) {
    return;
  }

  m_installed.erase(ins->package());
}

void Catalog::changeStatus(Delegate::FailureCode newStatus)
{
    if (m_status == newStatus) {
        return;
    }
    
    m_status = newStatus;
    m_statusCallbacks(this);
}

void Catalog::addStatusCallback(const Callback& cb)
{
    m_statusCallbacks.push_back(cb);
}

Delegate::FailureCode Catalog::status() const
{
    return m_status;
}

} // of namespace pkg

} // of namespace simgear
