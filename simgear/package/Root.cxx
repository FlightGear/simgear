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

#include <simgear/package/Root.hxx>

#include <boost/foreach.hpp>
#include <cstring>
#include <map>
#include <deque>
#include <set>

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

typedef std::map<std::string, Catalog*> CatalogDict;
    
class Root::RootPrivate
{
public:
    RootPrivate() :
        http(NULL),
        maxAgeSeconds(60 * 60 * 24),
        delegate(NULL)
    {
        
    }
    
    SGPath path;
    std::string locale;
    HTTP::Client* http;
    CatalogDict catalogs;
    unsigned int maxAgeSeconds;
    Delegate* delegate;
    std::string version;
    
    std::set<Catalog*> refreshing;
    std::deque<Install*> updateDeque;
    std::deque<HTTP::Request_ptr> httpPendingRequests;
};
    
SGPath Root::path() const
{
    return d->path;
}
    
void Root::setMaxAgeSeconds(unsigned int seconds)
{
    d->maxAgeSeconds = seconds;
}
    
unsigned int Root::maxAgeSeconds() const
{
    return d->maxAgeSeconds;
}

void Root::setHTTPClient(HTTP::Client* aHTTP)
{
    d->http = aHTTP;
    BOOST_FOREACH(HTTP::Request_ptr req, d->httpPendingRequests) {
        d->http->makeRequest(req);
    }

    d->httpPendingRequests.clear();
}

void Root::makeHTTPRequest(HTTP::Request *req)
{
    if (d->http) {
        d->http->makeRequest(req);
        return;
    }
    
    d->httpPendingRequests.push_back(req);
}
    
Root::Root(const SGPath& aPath, const std::string& aVersion) :
    d(new RootPrivate)
{
    d->path = aPath;
    d->version = aVersion;
    if (getenv("LOCALE")) {
        d->locale = getenv("LOCALE");
    }
    
    Dir dir(aPath);
    if (!dir.exists()) {
        dir.create(0755);
        return;
    }
    
    BOOST_FOREACH(SGPath c, dir.children(Dir::TYPE_DIR)) {
        Catalog* cat = Catalog::createFromPath(this, c);
        if (cat) {
           d->catalogs[cat->id()] = cat;
        }
    } // of child directories iteration
}

Root::~Root()
{
    
}
    
std::string Root::catalogVersion() const
{
    return d->version;
}
    
Catalog* Root::getCatalogById(const std::string& aId) const
{
    CatalogDict::const_iterator it = d->catalogs.find(aId);
    if (it == d->catalogs.end()) {
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
        CatalogDict::const_iterator it = d->catalogs.begin();
        for (; it != d->catalogs.end(); ++it) {
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
    CatalogDict::const_iterator it = d->catalogs.begin();
    for (; it != d->catalogs.end(); ++it) {
        r.push_back(it->second);
    }
    
    return r;
}

PackageList
Root::packagesMatching(const SGPropertyNode* aFilter) const
{
    PackageList r;
    
    CatalogDict::const_iterator it = d->catalogs.begin();
    for (; it != d->catalogs.end(); ++it) {
        PackageList r2(it->second->packagesMatching(aFilter));
        r.insert(r.end(), r2.begin(), r2.end());
    }
    
    return r;
}

PackageList
Root::packagesNeedingUpdate() const
{
    PackageList r;
    
    CatalogDict::const_iterator it = d->catalogs.begin();
    for (; it != d->catalogs.end(); ++it) {
        PackageList r2(it->second->packagesNeedingUpdate());
        r.insert(r.end(), r2.begin(), r2.end());
    }
    
    return r;
}

void Root::refresh(bool aForce)
{
    CatalogDict::iterator it = d->catalogs.begin();
    for (; it != d->catalogs.end(); ++it) {
        if (aForce || it->second->needsRefresh()) {
            it->second->refresh();
        }
    }
}

void Root::setDelegate(simgear::pkg::Delegate *aDelegate)
{
    d->delegate = aDelegate;
}
    
void Root::setLocale(const std::string& aLocale)
{
    d->locale = aLocale;
}

std::string Root::getLocale() const
{
    return d->locale;
}

void Root::scheduleToUpdate(Install* aInstall)
{
    if (!aInstall) {
        throw sg_exception("missing argument to scheduleToUpdate");
    }
    
    PackageList deps = aInstall->package()->dependencies();
    BOOST_FOREACH(Package* dep, deps) {
        // will internally schedule for update if required
        // hence be careful, this method is re-entered in here!
        dep->install();
    }

    bool wasEmpty = d->updateDeque.empty();
    d->updateDeque.push_back(aInstall);
    
    if (wasEmpty) {
        aInstall->startUpdate();
    }
}

void Root::startInstall(Install* aInstall)
{
    if (d->delegate) {
        d->delegate->startInstall(aInstall);
    }
}

void Root::installProgress(Install* aInstall, unsigned int aBytes, unsigned int aTotal)
{
    if (d->delegate) {
        d->delegate->installProgress(aInstall, aBytes, aTotal);
    }
}

void Root::startNext(Install* aCurrent)
{
    if (d->updateDeque.front() != aCurrent) {
        SG_LOG(SG_GENERAL, SG_ALERT, "current install of package not head of the deque");
    } else {
        d->updateDeque.pop_front();
    }
    
    if (!d->updateDeque.empty()) {
        d->updateDeque.front()->startUpdate();
    }
}

void Root::finishInstall(Install* aInstall)
{
    if (d->delegate) {
        d->delegate->finishInstall(aInstall);
    }
    
    startNext(aInstall);
}

void Root::failedInstall(Install* aInstall, Delegate::FailureCode aReason)
{
    SG_LOG(SG_GENERAL, SG_ALERT, "failed to install package:" 
        << aInstall->package()->id() << ":" << aReason);
    if (d->delegate) {
        d->delegate->failedInstall(aInstall, aReason);
    }
    
    startNext(aInstall);
}

void Root::catalogRefreshBegin(Catalog* aCat)
{
    d->refreshing.insert(aCat);
}

void Root::catalogRefreshComplete(Catalog* aCat, Delegate::FailureCode aReason)
{
    CatalogDict::iterator catIt = d->catalogs.find(aCat->id());
    if (aReason != Delegate::FAIL_SUCCESS) {
        if (d->delegate) {
            d->delegate->failedRefresh(aCat, aReason);
        }
        
        // if the failure is permanent, delete the catalog from our
        // list (don't touch it on disk)
        bool isPermanentFailure = (aReason == Delegate::FAIL_VERSION);
        if (isPermanentFailure) {
            SG_LOG(SG_GENERAL, SG_WARN, "permanent failure for catalog:" << aCat->id());
            d->catalogs.erase(catIt);
        }
    } else if (catIt == d->catalogs.end()) {
        // first fresh, add to our storage now
        d->catalogs.insert(catIt, CatalogDict::value_type(aCat->id(), aCat));
    }
    
    d->refreshing.erase(aCat);
    if (d->refreshing.empty()) {
        if (d->delegate) {
            d->delegate->refreshComplete();
        }
    }
}

} // of namespace pkg

} // of namespace simgear
