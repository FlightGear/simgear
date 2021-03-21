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

#include <simgear_config.h>

#include <simgear/package/Root.hxx>

#include <algorithm>
#include <cassert>
#include <cstring>
#include <deque>
#include <map>
#include <set>

#include <simgear/debug/logstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/sg_hash.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Catalog.hxx>

const int SECONDS_PER_DAY = 24 * 60 * 60;

namespace simgear {

namespace {
    std::string hashForUrl(const std::string& d)
    {
        sha1nfo info;
        sha1_init(&info);
        sha1_write(&info, d.data(), d.size());
        return strutils::encodeHex(sha1_result(&info), HASH_LENGTH);
    }
} // of anonymous namespace

namespace pkg {

typedef std::map<std::string, CatalogRef> CatalogDict;
typedef std::vector<Delegate*> DelegateVec;
typedef std::deque<std::string> StringDeque;

class Root::ThumbnailDownloader : public HTTP::Request
{
public:
    ThumbnailDownloader(Root::RootPrivate* aOwner,
                        const std::string& aUrl, const std::string& aRealUrl = std::string()) :
        HTTP::Request(aUrl),
        m_owner(aOwner),
        m_realUrl(aRealUrl)
    {
        if (m_realUrl.empty()) {
            m_realUrl = aUrl;
        }
    }

    std::string realUrl() const
    {
        return m_realUrl;
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
    std::string m_realUrl;
};

class Root::RootPrivate
{
public:
    RootPrivate() :
        http(NULL),
        maxAgeSeconds(SECONDS_PER_DAY)
    {
    }

    void fireStatusChange(InstallRef install, Delegate::StatusCode status)
    {
        for (auto d : delegates) {
            d->installStatusChanged(install, status);
        }
    }

    void fireStartInstall(InstallRef install)
    {
        for (auto d : delegates) {
            d->startInstall(install);
            d->installStatusChanged(install, Delegate::STATUS_IN_PROGRESS);
        }
    }

    void fireInstallProgress(InstallRef install,
                             unsigned int aBytes, unsigned int aTotal)
    {
        for (auto d : delegates) {
            d->installProgress(install, aBytes, aTotal);
        }
    }

    void fireFinishInstall(InstallRef install, Delegate::StatusCode status)
    {
        for (auto d : delegates) {
            d->finishInstall(install, status);
            d->installStatusChanged(install, status);
        }
    }

    void fireRefreshStatus(CatalogRef catalog, Delegate::StatusCode status)
    {
        // take a copy of delegates since firing this can modify the data
        const auto currentDelegates = delegates;
        for (auto d : currentDelegates) {
            d->catalogRefreshed(catalog, status);
        }
    }

    void firePackagesChanged()
    {
        for (auto d : delegates) {
            d->availablePackagesChanged();
        }
    }

    void thumbnailDownloadComplete(HTTP::Request_ptr request,
                                   Delegate::StatusCode status, const std::string& bytes)
    {
        auto dl = static_cast<Root::ThumbnailDownloader*>(request.get());
        std::string u = dl->realUrl();
        if (status == Delegate::STATUS_SUCCESS) {
            thumbnailCache[u].requestPending = false;

            // if this was a network load, rather than a re-load from the disk cache,
            // then persist to disk now.
            if (strutils::starts_with(request->url(), "http")) {
                addToPersistentCache(u, bytes);
            }

            fireDataForThumbnail(u, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
        } else if (status == Delegate::FAIL_HTTP_FORBIDDEN) {
            // treat this as rate-limiting failure, at least from some mirrors
            // (eg Ibiblio) and retry up to the max count
            const int retries = (thumbnailCache[u].retryCount++);
            if (retries < 3) {
                SG_LOG(SG_IO, SG_DEBUG, "Download failed for: " << u << ", will retry");
                thumbnailCache[u].requestPending = true;
                pendingThumbnails.push_back(u);
            }
        } else {
            // any other failure.
            thumbnailCache[u].requestPending = false;

            // if this was a cache refresh, let's report the cached data instead
            SGPath cachePath = pathInCache(u);
            if (cachePath.exists()) {
                SG_LOG(SG_IO, SG_WARN, "Download failed for: " << u << ", will use old cached data");
                cachePath.touch(); // touch the file so we don't repeat this danxce
                // kick a load from the cache
                loadFromPersistentCache(u, cachePath);
            }
        }

        downloadNextPendingThumbnail();
    }

    void fireDataForThumbnail(const std::string& aUrl, const uint8_t* bytes, size_t size)
    {
        for (auto d : delegates) {
            d->dataForThumbnail(aUrl, size, bytes);
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

    void fireFinishUninstall(PackageRef pkg)
    {
        for (auto d : delegates) {
            d->finishUninstall(pkg);
        }
    }

    SGPath pathInCache(const std::string& url) const
    {
        const auto hash = hashForUrl(url);
        // append the correct file suffix
        auto pos = url.rfind('.');
        if (pos == std::string::npos) {
            return SGPath();
        }

        return path / "ThumbnailCache" / (hash + url.substr(pos));
    }

    void addToPersistentCache(const std::string& url, const std::string& imageBytes)
    {
       // this will over-write the existing file if we are refreshing,
        // since we use 'truncatr' to open the new file
        SGPath cachePath = pathInCache(url);
        sg_ofstream fstream(cachePath, std::ios::out | std::ios::trunc | std::ios::binary);
        fstream.write(imageBytes.data(), imageBytes.size());
        fstream.close();

        auto it = thumbnailCache.find(url);
        assert(it != thumbnailCache.end());
        it->second.pathOnDisk = cachePath;
    }

    bool checkPersistentCache(const std::string& url)
    {
        SGPath cachePath = pathInCache(url);
        if (!cachePath.exists()) {
            return false;
        }

        // check age, if it's too old, expire and download again
        int age = time(nullptr) - cachePath.modTime();
        const int cacheMaxAge = SECONDS_PER_DAY * 7;
        if (age > cacheMaxAge) { // cache for seven days
            // note we do *not* remove the file data here, since the
            // cache refresh might fail
            return false;
        }

        loadFromPersistentCache(url, cachePath);
        return true;
    }

    void loadFromPersistentCache(const std::string& url, const SGPath& path)
    {
        assert(path.exists());

        auto it = thumbnailCache.find(url);
        if (it == thumbnailCache.end()) {
            ThumbnailCacheEntry entry;
            entry.pathOnDisk = path;
            it = thumbnailCache.insert(it, std::make_pair(url, entry));
        } else {
            assert(it->second.pathOnDisk.isNull() || (it->second.pathOnDisk == path));
        }

        sg_ifstream thumbnailStream(path, std::ios::in | std::ios::binary);
        string bytes = thumbnailStream.read_all();
        fireDataForThumbnail(url, reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size());
    }

    void writeRemovedCatalogsFile() const {
      SGPath p = path / "RemovedCatalogs";
      sg_ofstream stream(p, std::ios::out | std::ios::trunc | std::ios::binary);
      for (const auto &cid : manuallyRemovedCatalogs) {
        stream << cid << "\n";
      }
      stream.close();
    }

    void loadRemovedCatalogsFile() {
      manuallyRemovedCatalogs.clear();
      SGPath p = path / "RemovedCatalogs";
      if (!p.exists())
        return;

      sg_ifstream stream(p, std::ios::in);
      while (!stream.eof()) {
        std::string line;
        std::getline(stream, line);
        const auto trimmed = strutils::strip(line);
        if (!trimmed.empty()) {
          manuallyRemovedCatalogs.push_back(trimmed);
        }
      } // of lines iteration
    }

    DelegateVec delegates;

    SGPath path;
    std::string locale;
    HTTP::Client* http;
    CatalogDict catalogs;
    CatalogList disabledCatalogs;
    unsigned int maxAgeSeconds;
    std::string version;

    std::set<CatalogRef> refreshing;
    typedef std::deque<InstallRef> UpdateDeque;
    UpdateDeque updateDeque;
    std::deque<HTTP::Request_ptr> httpPendingRequests;

    HTTP::Request_ptr thumbnailDownloadRequest;
    StringDeque pendingThumbnails;

    struct ThumbnailCacheEntry
    {
        int retryCount = 0;
        bool requestPending = false;
        SGPath pathOnDisk;
    };

    std::map<std::string, ThumbnailCacheEntry> thumbnailCache;

    typedef std::map<PackageRef, InstallRef> InstallCache;
    InstallCache m_installs;

    /// persistent list of catalogs the user has manually removed
    string_list manuallyRemovedCatalogs;
};


void Root::ThumbnailDownloader::onDone()
{
    if (simgear::strutils::starts_with(url(), "file://")) {
        m_owner->thumbnailDownloadComplete(this, Delegate::STATUS_SUCCESS, m_buffer);
        return;
    }

    if (responseCode() != 200) {
        auto status = (responseCode() == 403) ? Delegate::FAIL_HTTP_FORBIDDEN : Delegate::FAIL_DOWNLOAD;
        SG_LOG(SG_NETWORK, SG_INFO, "thumbnail download failure: " << url() << " with reason " << responseCode());
        m_owner->thumbnailDownloadComplete(this, status, std::string());
        return;
    }

    m_owner->thumbnailDownloadComplete(this, Delegate::STATUS_SUCCESS, m_buffer);
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
    for (auto req : d->httpPendingRequests) {
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

void Root::cancelHTTPRequest(HTTP::Request *req, const std::string &reason)
{
    if (d->http) {
        d->http->cancelRequest(req, reason);
    }

    std::deque<HTTP::Request_ptr>::iterator it = std::find(d->httpPendingRequests.begin(),
                                                           d->httpPendingRequests.end(),
                                                           req);
    if (it != d->httpPendingRequests.end()) {
        d->httpPendingRequests.erase(it);
    }
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
    }

    Dir thumbsCacheDir(aPath / "ThumbnailCache");
    if (!thumbsCacheDir.exists()) {
        thumbsCacheDir.create(0755);
    }

    d->loadRemovedCatalogsFile();

    for (SGPath c : dir.children(Dir::TYPE_DIR | Dir::NO_DOT_OR_DOTDOT)) {
        // note this will set the catalog status, which will insert into
        // disabled catalogs automatically if necesary
        auto cat = Catalog::createFromPath(this, c);
        if (cat && cat->isEnabled()) {
            d->catalogs.insert({cat->id(), cat});
        } else if (cat) {
            SG_LOG(SG_GENERAL, SG_DEBUG, "Package-Root init: catalog is disabled: " << cat->id());
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
    auto it = d->catalogs.find(aId);
    if (it == d->catalogs.end()) {
        // check disabled catalog list
        auto j = std::find_if(d->disabledCatalogs.begin(), d->disabledCatalogs.end(),
                              [aId](const CatalogRef& cat) { return cat->id() == aId; });
        if (j != d->disabledCatalogs.end()) {
            return *j;
        }
        return nullptr;
    }

    return it->second;
}

CatalogRef Root::getCatalogByUrl(const std::string& aUrl) const
{
    auto it = std::find_if(d->catalogs.begin(), d->catalogs.end(),
                           [aUrl](const CatalogDict::value_type& v)
                           { return (v.second->url() == aUrl); });
    if (it == d->catalogs.end())
        return {};

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

CatalogList Root::allCatalogs() const
{
    CatalogList r = catalogs();
    r.insert(r.end(), d->disabledCatalogs.begin(), d->disabledCatalogs.end());
    return r;
}

PackageList
Root::allPackages(Type ty) const
{
    PackageList r;

    CatalogDict::const_iterator it = d->catalogs.begin();
    for (; it != d->catalogs.end(); ++it) {
        const PackageList& r2(it->second->packages(ty));
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

    // copy all candidate catalogs to a seperate list, since refreshing
    // can modify both the main collection and/or the disabled list
    CatalogList toRefresh;
    CatalogDict::iterator it = d->catalogs.begin();
    for (; it != d->catalogs.end(); ++it) {
        unsigned int age = it->second->ageInSeconds();
        if (aForce || (age > maxAgeSeconds())) {
            toRefresh.push_back(it->second);
        }
    }

    toRefresh.insert(toRefresh.end(), d->disabledCatalogs.begin(),
                     d->disabledCatalogs.end());
    for (auto cat : toRefresh) {
        cat->refresh();
        didStartAny = true;
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
    auto it = std::find(d->delegates.begin(), d->delegates.end(), aDelegate);
    if (it != d->delegates.end()) {
        d->delegates.erase(it);
    }
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

    auto it = std::find(d->updateDeque.begin(), d->updateDeque.end(), aInstall);
    if (it != d->updateDeque.end()) {
        // already scheduled to update
        return;
    }

    PackageList deps = aInstall->package()->dependencies();
    for (Package* dep : deps) {
        // will internally schedule for update if required
        // hence be careful, this method is re-entered in here!
        dep->install();
    }

    bool wasEmpty = d->updateDeque.empty();
    d->updateDeque.push_back(aInstall);

    d->fireStatusChange(aInstall, Delegate::STATUS_IN_PROGRESS);
    if (wasEmpty) {
        aInstall->startUpdate();
    }
}

void Root::scheduleAllUpdates() {
  auto toBeUpdated = packagesNeedingUpdate(); // make a copy
  for (const auto &u : toBeUpdated) {
    scheduleToUpdate(u->existingInstall());
  }
}

bool Root::isInstallQueued(InstallRef aInstall) const
{
    auto it = std::find(d->updateDeque.begin(), d->updateDeque.end(), aInstall);
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
	if (d->updateDeque.empty() || (d->updateDeque.front() != aCurrent)) {
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
        SG_LOG(SG_GENERAL, SG_WARN, "failed to install package:"
               << aInstall->package()->id() << ":" << aReason);
    }

    // order matters here, so a call to 'isQueued' from a finish-install
    // callback returns false, not true
    startNext(aInstall);
    d->fireFinishInstall(aInstall, aReason);
}

void Root::cancelDownload(InstallRef aInstall)
{
    auto it = std::find(d->updateDeque.begin(), d->updateDeque.end(), aInstall);
    if (it != d->updateDeque.end()) {
        bool startNext = (aInstall == d->updateDeque.front());
        d->updateDeque.erase(it);
        d->fireStatusChange(aInstall, Delegate::USER_CANCELLED);

        if (startNext) {
            if (!d->updateDeque.empty()) {
                d->updateDeque.front()->startUpdate();
            }
        } // of install was front item
    } // of found install in queue
}

void Root::catalogRefreshStatus(CatalogRef aCat, Delegate::StatusCode aReason)
{
    auto catIt = d->catalogs.find(aCat->id());
    d->fireRefreshStatus(aCat, aReason);

    if (aCat->isUserEnabled() &&
        (aReason == Delegate::STATUS_REFRESHED) && 
        (catIt == d->catalogs.end())) 
    {
        assert(!aCat->id().empty());
        d->catalogs.insert(catIt, CatalogDict::value_type(aCat->id(), aCat));

    // catalog might have been previously disabled, let's remove in that case
        auto j = std::find(d->disabledCatalogs.begin(),
                        d->disabledCatalogs.end(),
                        aCat);
        if (j != d->disabledCatalogs.end()) {
            SG_LOG(SG_GENERAL, SG_INFO, "re-enabling disabled catalog:" << aCat->id());
            d->disabledCatalogs.erase(j);
        }
    }

    if (!aCat->isEnabled()) {
        // catalog has errors or was disabled by user, disable it
        auto j = std::find(d->disabledCatalogs.begin(),
                           d->disabledCatalogs.end(),
                           aCat);
        if (j == d->disabledCatalogs.end()) {
            SG_LOG(SG_GENERAL, SG_INFO, "disabling catalog:" << aCat->id());
            d->disabledCatalogs.push_back(aCat);
        }

        // and remove it from the active collection
        if (catIt != d->catalogs.end()) {
            d->catalogs.erase(catIt);
        }
    } // of catalog is disabled

    // remove from refreshing /after/ checking for enable / disabled, since for
    // new catalogs, the reference in d->refreshing might be our /only/
    // reference to the catalog. Once the refresh is done (either failed or
    // succeeded) the Catalog will be in either d->catalogs or
    // d->disabledCatalogs
    if (aReason == Delegate::STATUS_IN_PROGRESS) {
      d->refreshing.insert(aCat);
    } else {
      d->refreshing.erase(aCat);
    }

    if (d->refreshing.empty()) {
        d->fireRefreshStatus(CatalogRef(), Delegate::STATUS_REFRESHED);
        d->firePackagesChanged();
    }
}

bool Root::removeCatalog(CatalogRef cat)
{
    if (!cat)
        return false;

    // normal remove path
    if (!cat->id().empty()) {
        return removeCatalogById(cat->id());
    }

    if (!cat->removeDirectory()) {
        SG_LOG(SG_GENERAL, SG_WARN, "removeCatalog: failed to remove directory " << cat->installRoot());
    }
    auto it = std::find(d->disabledCatalogs.begin(),
                       d->disabledCatalogs.end(),
                       cat);
    if (it != d->disabledCatalogs.end()) {
        d->disabledCatalogs.erase(it);
    }

    // notify that a catalog is being removed
    d->firePackagesChanged();

    return true;
}

bool Root::removeCatalogById(const std::string& aId)
{
    CatalogRef cat;

    CatalogDict::iterator catIt = d->catalogs.find(aId);
    if (catIt == d->catalogs.end()) {
        // check the disabled list
        auto j = std::find_if(d->disabledCatalogs.begin(), d->disabledCatalogs.end(),
                              [aId](const CatalogRef& cat) { return cat->id() == aId; });
        if (j == d->disabledCatalogs.end()) {
            SG_LOG(SG_GENERAL, SG_WARN, "removeCatalogById: no catalog with id:" << aId);
            return false;
        }

        cat = *j;
        d->disabledCatalogs.erase(j);
    } else {
        cat = catIt->second;
        // drop the reference
        d->catalogs.erase(catIt);
    }

    bool ok = cat->removeDirectory();
    if (!ok) {
        SG_LOG(SG_GENERAL, SG_WARN, "removeCatalogById: catalog :" << aId
            << "failed to remove directory");
    }

    d->manuallyRemovedCatalogs.push_back(aId);
    d->writeRemovedCatalogsFile();

    // notify that a catalog is being removed
    d->firePackagesChanged();

    return ok;
}

void Root::requestThumbnailData(const std::string& aUrl)
{
    if (aUrl.empty()) {
        SG_LOG(SG_GENERAL, SG_DEV_WARN, "requestThumbnailData: empty URL requested");
        return;
    }

    auto it = d->thumbnailCache.find(aUrl);
    if (it == d->thumbnailCache.end()) {
        bool cachedOnDisk = d->checkPersistentCache(aUrl);
        if (cachedOnDisk) {
            // checkPersistentCache will insert the entry and schedule
        } else {
            d->pendingThumbnails.push_front(aUrl);
            d->thumbnailCache[aUrl] = RootPrivate::ThumbnailCacheEntry();
            d->thumbnailCache[aUrl].requestPending = true;
            d->downloadNextPendingThumbnail();
        }
    } else {
        if (!it->second.requestPending && it->second.pathOnDisk.exists()) {
            d->loadFromPersistentCache(aUrl, it->second.pathOnDisk);
        }
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
    d->fireFinishUninstall(ins->package());
}

string_list Root::explicitlyRemovedCatalogs() const {
  return d->manuallyRemovedCatalogs;
}


PackageList Root::packagesProviding(const std::string& path, bool onlyInstalled) const
{
    string modPath = path;
    auto inferredType = AnyPackageType;
    if (strutils::starts_with(path, "Aircraft/")) {
        inferredType = AircraftPackage;
        modPath = path.substr(9);
    } else if (strutils::starts_with(path, "AI/Aircraft/")) {
        inferredType = AIModelPackage;
        modPath = path.substr(12);
    }

    string subPath;
    const auto firstSeperatorPos = modPath.find('/');
    if (firstSeperatorPos != std::string::npos) {
        subPath = modPath.substr(firstSeperatorPos + 1);
        modPath.resize(firstSeperatorPos);
    }

    PackageList r;
    for (auto cat : d->catalogs) {
        const auto p = cat.second->packagesProviding(inferredType, modPath, subPath);
        r.insert(r.end(), p.begin(), p.end());
    } // catalog iteratrion

    if (onlyInstalled) {
        auto it = std::remove_if(r.begin(), r.end(), [](const PackageRef& p) {
            return p->isInstalled();
        });
        r.erase(it, r.end());
    }

    return r;
}

} // of namespace pkg

} // of namespace simgear
