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

typedef std::map<std::string, CatalogRef> CatalogDict;
typedef std::vector<Delegate*> DelegateVec;
typedef std::map<std::string, std::string> MemThumbnailCache;
typedef std::deque<std::string> StringDeque;

class Root::ThumbnailDownloader : public HTTP::Request
{
public:
    ThumbnailDownloader(Root::RootPrivate* aOwner, const std::string& aUrl) :
    HTTP::Request(aUrl),
    m_owner(aOwner)
    {
    }

protected:
    virtual void gotBodyData(const char* s, int n)
    {
        m_buffer += std::string(s, n);
    }

    virtual void onDone();

private:
    Root::RootPrivate* m_owner;
    std::string m_buffer;
};

class Root::RootPrivate
{
public:
    RootPrivate() :
        http(NULL),
        maxAgeSeconds(60 * 60 * 24)
    {
    }

    void fireStartInstall(InstallRef install)
    {
        DelegateVec::const_iterator it;
        for (it = delegates.begin(); it != delegates.end(); ++it) {
            (*it)->startInstall(install);
        }
    }

    void fireInstallProgress(InstallRef install,
                             unsigned int aBytes, unsigned int aTotal)
    {
        DelegateVec::const_iterator it;
        for (it = delegates.begin(); it != delegates.end(); ++it) {
            (*it)->installProgress(install, aBytes, aTotal);
        }
    }

    void fireFinishInstall(InstallRef install, Delegate::StatusCode status)
    {
        DelegateVec::const_iterator it;
        for (it = delegates.begin(); it != delegates.end(); ++it) {
            (*it)->finishInstall(install, status);
        }
    }

    void fireRefreshStatus(CatalogRef catalog, Delegate::StatusCode status)
    {
        DelegateVec::const_iterator it;
        for (it = delegates.begin(); it != delegates.end(); ++it) {
            (*it)->catalogRefreshed(catalog, status);
        }
    }

    void firePackagesChanged()
    {
      DelegateVec::const_iterator it;
      for (it = delegates.begin(); it != delegates.end(); ++it) {
          (*it)->availablePackagesChanged();
      }
    }

    void thumbnailDownloadComplete(HTTP::Request_ptr request,
                                   Delegate::StatusCode status, const std::string& bytes)
    {
        std::string u(request->url());
        if (status == Delegate::STATUS_SUCCESS) {
            thumbnailCache[u] = bytes;
            fireDataForThumbnail(u, bytes);
        }

        downloadNextPendingThumbnail();
    }

    void fireDataForThumbnail(const std::string& aUrl, const std::string& bytes)
    {
        DelegateVec::const_iterator it;
        const uint8_t* data = reinterpret_cast<const uint8_t*>(bytes.data());
        for (it = delegates.begin(); it != delegates.end(); ++it) {
            (*it)->dataForThumbnail(aUrl, bytes.size(), data);
        }
    }

    void downloadNextPendingThumbnail()
    {
        thumbnailDownloadRequest.clear();
        if (pendingThumbnails.empty()) {
            return;
        }

        std::string u = pendingThumbnails.front();
        pendingThumbnails.pop_front();
        thumbnailDownloadRequest = new Root::ThumbnailDownloader(this, u);

        if (http) {
            http->makeRequest(thumbnailDownloadRequest);
        } else {
            httpPendingRequests.push_back(thumbnailDownloadRequest);
        }
    }

    DelegateVec delegates;

    SGPath path;
    std::string locale;
    HTTP::Client* http;
    CatalogDict catalogs;
    unsigned int maxAgeSeconds;
    std::string version;

    std::set<CatalogRef> refreshing;
    typedef std::deque<InstallRef> UpdateDeque;
    UpdateDeque updateDeque;
    std::deque<HTTP::Request_ptr> httpPendingRequests;

    HTTP::Request_ptr thumbnailDownloadRequest;
    StringDeque pendingThumbnails;
    MemThumbnailCache thumbnailCache;

    typedef std::map<PackageRef, InstallRef> InstallCache;
    InstallCache m_installs;
};


void Root::ThumbnailDownloader::onDone()
{
    if (responseCode() != 200) {
        SG_LOG(SG_GENERAL, SG_ALERT, "thumbnail download failure:" << url());
        m_owner->thumbnailDownloadComplete(this, Delegate::FAIL_DOWNLOAD, std::string());
        return;
    }

    m_owner->thumbnailDownloadComplete(this, Delegate::STATUS_SUCCESS, m_buffer);
    //time(&m_owner->m_retrievedTime);
    //m_owner->writeTimestamp();
    //m_owner->refreshComplete(Delegate::STATUS_REFRESHED);
}

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
        CatalogRef cat = Catalog::createFromPath(this, c);
        if (cat) {
           d->catalogs[cat->id()] = cat;
        }
    } // of child directories iteration
}

Root::~Root()
{

}

int Root::catalogVersion() const
{
    return 4;
}

std::string Root::applicationVersion() const
{
    return d->version;
}

CatalogRef Root::getCatalogById(const std::string& aId) const
{
    CatalogDict::const_iterator it = d->catalogs.find(aId);
    if (it == d->catalogs.end()) {
        return NULL;
    }

    return it->second;
}

PackageRef Root::getPackageById(const std::string& aName) const
{
    size_t lastDot = aName.rfind('.');

    PackageRef pkg = NULL;
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
    CatalogRef catalog = getCatalogById(catalogId);
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
Root::allPackages() const
{
    PackageList r;

    CatalogDict::const_iterator it = d->catalogs.begin();
    for (; it != d->catalogs.end(); ++it) {
        const PackageList& r2(it->second->packages());
        r.insert(r.end(), r2.begin(), r2.end());
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
    bool didStartAny = false;
    CatalogDict::iterator it = d->catalogs.begin();
    for (; it != d->catalogs.end(); ++it) {
        if (aForce || it->second->needsRefresh()) {
            it->second->refresh();
            didStartAny = true;
        }
    }

    if (!didStartAny) {
        // signal refresh complete to the delegate already
        d->fireRefreshStatus(CatalogRef(), Delegate::STATUS_REFRESHED);
    }
}

void Root::addDelegate(simgear::pkg::Delegate *aDelegate)
{
    d->delegates.push_back(aDelegate);
}

void Root::removeDelegate(simgear::pkg::Delegate *aDelegate)
{
    DelegateVec::iterator it = std::find(d->delegates.begin(),
                                         d->delegates.end(), aDelegate);
    if (it == d->delegates.end()) {
        throw sg_exception("unknown delegate in removeDelegate");
    }
    d->delegates.erase(it);
}

void Root::setLocale(const std::string& aLocale)
{
    d->locale = aLocale;
}

std::string Root::getLocale() const
{
    return d->locale;
}

void Root::scheduleToUpdate(InstallRef aInstall)
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

bool Root::isInstallQueued(InstallRef aInstall) const
{
    RootPrivate::UpdateDeque::const_iterator it =
        std::find(d->updateDeque.begin(), d->updateDeque.end(), aInstall);
    return (it != d->updateDeque.end());
}

void Root::startInstall(InstallRef aInstall)
{
    d->fireStartInstall(aInstall);
}

void Root::installProgress(InstallRef aInstall, unsigned int aBytes, unsigned int aTotal)
{
    d->fireInstallProgress(aInstall, aBytes, aTotal);
}

void Root::startNext(InstallRef aCurrent)
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

void Root::finishInstall(InstallRef aInstall, Delegate::StatusCode aReason)
{
    if (aReason != Delegate::STATUS_SUCCESS) {
        SG_LOG(SG_GENERAL, SG_ALERT, "failed to install package:"
               << aInstall->package()->id() << ":" << aReason);
    }

    // order matters here, so a call to 'isQueued' from a finish-install
    // callback returns false, not true
    startNext(aInstall);
    d->fireFinishInstall(aInstall, aReason);
}

void Root::cancelDownload(InstallRef aInstall)
{
    RootPrivate::UpdateDeque::iterator it =
        std::find(d->updateDeque.begin(), d->updateDeque.end(), aInstall);
    if (it != d->updateDeque.end()) {
        bool startNext = (aInstall == d->updateDeque.front());
        d->updateDeque.erase(it);
        if (startNext) {
            if (!d->updateDeque.empty()) {
                d->updateDeque.front()->startUpdate();
            }
        } // of install was front item
    } // of found install in queue
}

void Root::catalogRefreshStatus(CatalogRef aCat, Delegate::StatusCode aReason)
{
    CatalogDict::iterator catIt = d->catalogs.find(aCat->id());
    d->fireRefreshStatus(aCat, aReason);

    if (aReason == Delegate::STATUS_IN_PROGRESS) {
        d->refreshing.insert(aCat);
    } else {
        d->refreshing.erase(aCat);
    }

    if ((aReason != Delegate::STATUS_REFRESHED) && (aReason != Delegate::STATUS_IN_PROGRESS)) {
        // if the failure is permanent, delete the catalog from our
        // list (don't touch it on disk)
        bool isPermanentFailure = (aReason == Delegate::FAIL_VERSION);
        if (isPermanentFailure) {
            SG_LOG(SG_GENERAL, SG_WARN, "permanent failure for catalog:" << aCat->id());
            if (catIt != d->catalogs.end()) {
                d->catalogs.erase(catIt);
            }
        }
    }

    if ((aReason == Delegate::STATUS_REFRESHED) && (catIt == d->catalogs.end())) {
        assert(!aCat->id().empty());
        d->catalogs.insert(catIt, CatalogDict::value_type(aCat->id(), aCat));
    }

    if (d->refreshing.empty()) {
        d->fireRefreshStatus(CatalogRef(), Delegate::STATUS_REFRESHED);
        d->firePackagesChanged();
    }
}

bool Root::removeCatalogById(const std::string& aId)
{
    CatalogDict::iterator catIt = d->catalogs.find(aId);
    if (catIt == d->catalogs.end()) {
        SG_LOG(SG_GENERAL, SG_WARN, "removeCatalogById: unknown ID:" << aId);
        return false;
    }

    CatalogRef cat = catIt->second;

    // drop the reference
    d->catalogs.erase(catIt);

    bool ok = cat->uninstall();
    if (!ok) {
        SG_LOG(SG_GENERAL, SG_WARN, "removeCatalogById: catalog :" << aId
            << "failed to uninstall");
    }

    // notify that a catalog is being removed
    d->firePackagesChanged();

    return ok;
}

void Root::requestThumbnailData(const std::string& aUrl)
{
    MemThumbnailCache::iterator it = d->thumbnailCache.find(aUrl);
    if (it == d->thumbnailCache.end()) {
        // insert into cache to mark as pending
        d->pendingThumbnails.push_front(aUrl);
        d->thumbnailCache[aUrl] = std::string();
        d->downloadNextPendingThumbnail();
    } else if (!it->second.empty()) {
        // already loaded, fire data synchronously
        d->fireDataForThumbnail(aUrl, it->second);
    } else {
        // in cache but empty data, still fetching
    }
}

InstallRef Root::existingInstallForPackage(PackageRef p) const
{
    RootPrivate::InstallCache::const_iterator it =
        d->m_installs.find(p);
    if (it == d->m_installs.end()) {
        // check if it exists on disk, create
        SGPath path(p->pathOnDisk());
        if (path.exists()) {
            // this will add to our cache, and hence, modify m_installs
            return Install::createFromPath(path, p->catalog());
        }

        // insert a null reference into the dictionary, so we don't call
        // the pathOnDisk -> exists codepath repeatedley
        d->m_installs[p] = InstallRef();
        return InstallRef();
    }

    return it->second;
}

void Root::registerInstall(InstallRef ins)
{
    if (!ins.valid()) {
        return;
    }

    d->m_installs[ins->package()] = ins;
}

void Root::unregisterInstall(InstallRef ins)
{
    if (!ins .valid()) {
        return;
    }

    d->m_installs.erase(ins->package());
}

} // of namespace pkg

} // of namespace simgear
