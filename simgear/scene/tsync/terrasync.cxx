// terrasync.cxx -- scenery fetcher
//
// Started by Curtis Olson, November 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2008  Alexander R. Perry <alex.perry@ieee.org>
// Copyright (C) 2011  Thorsten Brehm <brehmt@gmail.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#include <simgear_config.h>
#include <simgear/compiler.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif

#ifdef __MINGW32__
#  include <time.h>
#  include <unistd.h>
#elif defined(_MSC_VER)
#   include <io.h>
#   include <time.h>
#   include <process.h>
#endif

#include <stdlib.h>             // atoi() atof() abs() system()
#include <signal.h>             // signal()
#include <string.h>

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>

#include <simgear/version.h>

#include "terrasync.hxx"

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/threads/SGQueue.hxx>
#include <simgear/threads/SGThread.hxx>

#include <simgear/misc/sg_dir.hxx>
#include <simgear/debug/BufferedLogCallback.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/HTTPRepository.hxx>
#include <simgear/io/DNSClient.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/math/sg_random.h>

using namespace simgear;
using namespace std;

namespace UpdateInterval
{
    // interval in seconds to allow an update to repeat after a successful update (=daily)
    static const double SuccessfulAttempt = 24*60*60;
    // interval in seconds to allow another update after a failed attempt (10 minutes)
    static const double FailedAttempt     = 10*60;
}

typedef map<string,time_t> TileAgeCache;

///////////////////////////////////////////////////////////////////////////////
// helper functions ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

string stripPath(string path)
{
    // svn doesn't like trailing white-spaces or path separators - strip them!
    path = simgear::strutils::strip(path);
    size_t slen = path.length();
    while ((slen>0)&&
            ((path[slen-1]=='/')||(path[slen-1]=='\\')))
    {
        slen--;
    }
    return path.substr(0,slen);
}

bool hasWhitespace(string path)
{
    return path.find(' ')!=string::npos;
}

///////////////////////////////////////////////////////////////////////////////
// SyncItem ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SyncItem
{
public:
    enum Type {
        Stop = 0, ///< special item indicating to stop the SVNThread
        Tile,
        AirportData,
        SharedModels,
        AIData,
        OSMTile ///< OSm2City per-Tile data
    };

    enum Status
    {
        Invalid = 0,
        Waiting,
        Cached, ///< using already cached result
        Updated,
        NotFound,
        Failed
    };

    SyncItem() :
        _dir(),
        _type(Stop),
        _status(Invalid)
    {
    }

    SyncItem(string dir, Type ty) :
        _dir(dir),
        _type(ty),
        _status(Waiting)
    {}

    string _dir;
    Type _type;
    Status _status;
};

///////////////////////////////////////////////////////////////////////////////

/**
 * @brief SyncSlot encapsulates a queue of sync items we will fetch
 * serially. Multiple slots exist to sync different types of item in
 * parallel.
 */
class SyncSlot
{
public:
    SyncSlot() = default;

    SyncItem currentItem;
    bool isNewDirectory = false;
    std::deque<SyncItem> queue;
    std::unique_ptr<HTTPRepository> repository;
    SGTimeStamp stamp;
    bool busy = false; ///< is the slot working or idle
    unsigned int pendingKBytes = 0;
    unsigned int pendingExtractKBytes = 0;
    unsigned int nextWarnTimeout = 0;
};

static const int SYNC_SLOT_TILES = 0; ///< Terrain and Objects sync
static const int SYNC_SLOT_SHARED_DATA = 1; /// shared Models and Airport data
static const int SYNC_SLOT_AI_DATA = 2; /// AI traffic and models
static const int SYNC_SLOT_OSM_TILE_DATA = 3;

static const unsigned int NUM_SYNC_SLOTS = 4;

/**
 * @brief translate a sync item type into one of the available slots.
 * This provides the scheduling / balancing / prioritising between slots.
 */
static unsigned int syncSlotForType(SyncItem::Type ty)
{
    switch (ty) {
    case SyncItem::Tile: return SYNC_SLOT_TILES;
    case SyncItem::SharedModels:
    case SyncItem::AirportData:
        return SYNC_SLOT_SHARED_DATA;
    case SyncItem::AIData:
        return SYNC_SLOT_AI_DATA;
    case SyncItem::OSMTile:
        return SYNC_SLOT_OSM_TILE_DATA;
    default:
        return SYNC_SLOT_SHARED_DATA;
    }
}

struct TerrasyncThreadState
{
    TerrasyncThreadState() : _busy(false),
                             _stalled(false),
                             _hasServer(false),
                             _fail_count(0),
                             _updated_tile_count(0),
                             _success_count(0),
                             _consecutive_errors(0),
                             _cache_hits(0),
                             _transfer_rate(0),
                             _total_kb_downloaded(0),
                             _totalKbPending(0),
                             _extractTotalKbPending(0)
    {}

    bool _busy;
    bool _stalled;
    bool _hasServer;
    int  _fail_count;
    int  _updated_tile_count;
    int  _success_count;
    int  _consecutive_errors;
    int  _cache_hits;
    int _transfer_rate;
    // kbytes, not bytes, because bytes might overflow 2^31
    int _total_kb_downloaded;
    unsigned int _totalKbPending;
    unsigned int _extractTotalKbPending;
};

///////////////////////////////////////////////////////////////////////////////
// SGTerraSync::WorkerThread //////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SGTerraSync::WorkerThread : public SGThread
{
public:
   WorkerThread();
   virtual ~WorkerThread( ) { stop(); }

   void stop();
   bool start();

    bool isIdle()
    {
        std::lock_guard<std::mutex> g(_stateLock);
        return !_state._busy;
    }

    bool isRunning()
    {
         std::lock_guard<std::mutex> g(_stateLock);
        return _running;
    }

    bool isStalled()
    {
        std::lock_guard<std::mutex> g(_stateLock);
        return _state._stalled;
    }

    bool hasServer()
    {
        std::lock_guard<std::mutex> g(_stateLock);
        return _state._hasServer;
    }

    bool hasServer( bool flag )
    {
        std::lock_guard<std::mutex> g(_stateLock);
        return (_state._hasServer = flag);
    }

    bool findServer();

    void request(const SyncItem& dir) {waitingTiles.push_front(dir);}

    bool hasNewTiles()
    {
        return !_freshTiles.empty();
    }

   SyncItem getNewTile() { return _freshTiles.pop_front();}

   void setHTTPServer(const std::string& server, const std::string& osmServer)
   {
      _httpServer = stripPath(server);
      _osmCityServer = stripPath(osmServer);
      _isAutomaticServer = (server == "automatic");
   }

   void setDNSServer(const std::string& nameserver)
   {
       _dnsNameserver = nameserver;
   }

   void setDNSDN( const std::string & dn )
   {
     _dnsdn = simgear::strutils::strip(dn);
   }

   void setProtocol( const std::string & protocol )
   {
     _protocol = simgear::strutils::strip(protocol);
   }

   void setSceneryVersion( const std::string & sceneryVersion )
   {
     _sceneryVersion = simgear::strutils::strip(sceneryVersion);
   }

   void setOSMCityVersion(const std::string& osmCityVersion)
   {
       _osmCityService = osmCityVersion;
   }

   void   setLocalDir(string dir)           { _local_dir    = stripPath(dir);}
   string getLocalDir()                     { return _local_dir;}

    void setInstalledDir(const SGPath& p)  { _installRoot = p; }

   void   setCacheHits(unsigned int hits)
    {
        std::lock_guard<std::mutex> g(_stateLock);
        _state._cache_hits = hits;
    }

    TerrasyncThreadState threadsafeCopyState()
    {
        TerrasyncThreadState st;
        {
            std::lock_guard<std::mutex> g(_stateLock);
            st = _state;
        }
        return st;
    }

    bool isDirActive(const std::string& path) const;

    void setCachePath(const SGPath &p) { _persistentCachePath = p; }

  private:
      std::string dnsSelectServerForService(const std::string& service);

      void incrementCacheHits()
      {
          std::lock_guard<std::mutex> g(_stateLock);
          _state._cache_hits++;
    }

   virtual void run();

    // internal mode run and helpers
    void runInternal();
    void updateSyncSlot(SyncSlot& slot);

    bool beginSyncAirports(SyncSlot& slot);
    bool beginSyncTile(SyncSlot& slot);
    bool beginNormalSync(SyncSlot& slot);

    void drainWaitingTiles();

    // commond helpers between both internal and external models

    SyncItem::Status isPathCached(const SyncItem& next) const;
    void updated(SyncItem item, bool isNewDirectory);
    void fail(SyncItem failedItem);
    void notFound(SyncItem notFoundItem);

    void initCompletedTilesPersistentCache();
    void writeCompletedTilesPersistentCache() const;

    HTTP::Client _http;
    SyncSlot _syncSlots[NUM_SYNC_SLOTS];

    bool _stop, _running;
    SGBlockingDeque <SyncItem> waitingTiles;

    TileAgeCache _completedTiles;
    TileAgeCache _notFoundItems;

    SGBlockingDeque <SyncItem> _freshTiles;
    string _local_dir;
    SGPath _persistentCachePath;
    string _httpServer;
    string _osmCityServer;
    string _osmCityService = "o2c";

    bool _isAutomaticServer;
    SGPath _installRoot;
    string _sceneryVersion;
    string _protocol;
    string _dnsdn;
    string _dnsNameserver;

    TerrasyncThreadState _state;
    mutable std::mutex _stateLock;
};

SGTerraSync::WorkerThread::WorkerThread() :
    _stop(false),
    _running(false),
    _isAutomaticServer(true)
{
    _http.setUserAgent("terrascenery-" SG_STRINGIZE(SIMGEAR_VERSION));
}

void SGTerraSync::WorkerThread::stop()
{
    // drop any pending requests
    waitingTiles.clear();

    if (!isRunning())
        return;

    // set stop flag and wake up the thread with an empty request
    {
        std::lock_guard<std::mutex> g(_stateLock);
        _stop = true;
    }

    SyncItem w(string(), SyncItem::Stop);
    request(w);
    join();

    // clear the sync slots, in case we restart
    for (unsigned int slot = 0; slot < NUM_SYNC_SLOTS; ++slot) {
        _syncSlots[slot] = {};
    }

    // clear these so if re-init-ing, we check again
    _completedTiles.clear();
    _notFoundItems.clear();

    _http.reset();
    _http.setUserAgent("terrascenery-" SG_STRINGIZE(SIMGEAR_VERSION));
}

bool SGTerraSync::WorkerThread::start()
{
    if (isRunning())
        return false;

    if (_local_dir=="")
    {
        SG_LOG(SG_TERRASYNC,SG_ALERT,
               "Cannot start scenery download. Local cache directory is undefined.");
        _state._fail_count++;
        _state._stalled = true;
        return false;
    }

    SGPath path(_local_dir);
    if (!path.exists())
    {
        const SGPath parentDir = path.dirPath();
        if (parentDir.exists()) {
            // attempt to create terraSync dir ourselves
            bool ok = path.create_dir(0755);
            if (!ok) {
                SG_LOG(SG_TERRASYNC, SG_ALERT,
                       "Cannot start scenery download. Directory '" << _local_dir << "' does not exist. Set correct directory path or create directory folder.");
                _state._fail_count++;
                _state._stalled = true;
                return false;
            }
        }
    }

    path.append("version");
    if (path.exists())
    {
        SG_LOG(SG_TERRASYNC,SG_ALERT,
               "Cannot start scenery download. Directory '" << _local_dir <<
               "' contains the base package. Use a separate directory.");
        _state._fail_count++;
        _state._stalled = true;
        return false;
    }

    _stop = false;
    _state = TerrasyncThreadState(); // clean state

    SG_LOG(SG_TERRASYNC, SG_MANDATORY_INFO,
           "Starting automatic scenery download/synchronization to '" << _local_dir << "'.");

    SGThread::start();
    return true;
}

static inline string MakeQService(string & protocol, string & version )
{
    if( protocol.empty() ) return version;
    return protocol + "+" + version;
}

bool SGTerraSync::WorkerThread::findServer()
{
    if ( false == _isAutomaticServer ) return true;

    _httpServer = dnsSelectServerForService(MakeQService(_protocol, _sceneryVersion));

    if (!_osmCityService.empty()) {
        _osmCityServer = dnsSelectServerForService(_osmCityService);
    }

    return !_httpServer.empty();
}

std::string SGTerraSync::WorkerThread::dnsSelectServerForService(const std::string& service)
{
    DNS::NAPTRRequest * naptrRequest = new DNS::NAPTRRequest(_dnsdn);
    naptrRequest->qservice = service;

    naptrRequest->qflags = "U";
    DNS::Request_ptr r(naptrRequest);

    DNS::Client dnsClient{_dnsNameserver};
    dnsClient.makeRequest(r);
    SG_LOG(SG_TERRASYNC,SG_DEBUG,"DNS NAPTR query for '" << _dnsdn << "' '" << naptrRequest->qservice << "'" );
    while (!r->isComplete() && !r->isTimeout()) {
        dnsClient.update(0);
    }

    if( naptrRequest->entries.empty() ) {
        SG_LOG(SG_TERRASYNC, SG_ALERT, "Warning: no DNS entry found for '" << _dnsdn << "' '" << naptrRequest->qservice << "'" );
        return {};
    }

    // walk through responses, they are ordered by 1. order and 2. preference
    // For now, only take entries with lowest order
    // TODO: try all available servers in the order given by preferenc and order
    int order = naptrRequest->entries[0]->order;

    // get all servers with this order and the same (for now only lowest preference)
    DNS::NAPTRRequest::NAPTR_list availableServers;
    for (const auto& entry : naptrRequest->entries) {
        if (entry->order != order)
            continue;

        const string regex = entry->regexp;
        if (false == simgear::strutils::starts_with(regex, "!^.*$!")) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "ignoring unsupported regexp: " << regex);
            continue;
        }

        if (false == simgear::strutils::ends_with(regex, "!")) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "ignoring unsupported regexp: " << regex);
            continue;
        }

        // always use first entry
        if (availableServers.empty() || entry->preference == availableServers[0]->preference) {
            SG_LOG(SG_TERRASYNC, SG_DEBUG, "available server regexp: " << regex);
            availableServers.push_back(entry);
        }
    }

    // now pick a random entry from the available servers
    auto idx = static_cast<int>(sg_random() * availableServers.size());
    const auto server = availableServers.at(idx)->regexp;
    SG_LOG(SG_TERRASYNC, SG_INFO, "picking entry # " << idx << ", server is " << server.substr(6, server.length() - 7););
    return server.substr(6, server.length() - 7);
}

void SGTerraSync::WorkerThread::run()
{
    {
         std::lock_guard<std::mutex> g(_stateLock);
        _running = true;
    }

    initCompletedTilesPersistentCache();
    runInternal();

    {
        std::lock_guard<std::mutex> g(_stateLock);
        _running = false;
    }
}

void SGTerraSync::WorkerThread::updateSyncSlot(SyncSlot &slot)
{
    if (slot.repository.get()) {
        slot.repository->process();
        if (slot.repository->isDoingSync()) {
#if 1
            if (slot.stamp.elapsedMSec() > (int)slot.nextWarnTimeout) {
                SG_LOG(SG_TERRASYNC, SG_INFO, "sync taking a long time:" << slot.currentItem._dir << " taken " << slot.stamp.elapsedMSec());
                SG_LOG(SG_TERRASYNC, SG_INFO, "HTTP request count:" << _http.hasActiveRequests());
                slot.nextWarnTimeout += 30 * 1000;
            }
#endif
            // convert bytes to kbytes here
            slot.pendingKBytes = (slot.repository->bytesToDownload() >> 10);
            slot.pendingExtractKBytes = (slot.repository->bytesToExtract() >> 10);
            return; // easy, still working
        }

        // check result
        HTTPRepository::ResultCode res = slot.repository->failure();

        if (res == HTTPRepository::REPO_ERROR_NOT_FOUND) {
            // not founds should never happen any more (unless the server-
            // side data is incorrect), since we now check top-down that
            // a 1x1 dir exists or not.
            notFound(slot.currentItem);
        } else if (res != HTTPRepository::REPO_NO_ERROR) {
            fail(slot.currentItem);

            // in case the Airports_archive download fails, create the
            // directory, so that next sync, we do a manual sync
            if ((slot.currentItem._type == SyncItem::AirportData) && slot.isNewDirectory) {
                SG_LOG(SG_TERRASYNC, SG_ALERT, "Failed to download Airports_archive, will download discrete files next time");
                simgear::Dir d(_local_dir + "/Airports");
                d.create(0755);
                _completedTiles.erase(slot.currentItem._dir);
            }
        } else {
            updated(slot.currentItem, slot.isNewDirectory);
            SG_LOG(SG_TERRASYNC, SG_DEBUG, "sync of " << slot.repository->baseUrl() << " finished ("
                   << slot.stamp.elapsedMSec() << " msec");
        }

        // whatever happened, we're done with this repository instance
        slot.busy = false;
        slot.repository.reset();
        slot.pendingKBytes = 0;
        slot.pendingExtractKBytes = 0;
        slot.currentItem = {};
    }

    // init and start sync of the next repository
    if (!slot.queue.empty()) {
        slot.currentItem = slot.queue.front();
        slot.queue.pop_front();

        SGPath path(_local_dir);
        path.append(slot.currentItem._dir);
        slot.isNewDirectory = !path.exists();
        const auto type = slot.currentItem._type;

        bool ok = false;
        if (type == SyncItem::AirportData) {
            ok = beginSyncAirports(slot);
        } else if (type == SyncItem::OSMTile) {
            ok = beginSyncTile(slot);
        } else if (type == SyncItem::Tile) {
            ok = beginSyncTile(slot);
        } else {
            ok = beginNormalSync(slot);
        }

        if (!ok) {
            SG_LOG(SG_TERRASYNC, SG_INFO, "sync of " << slot.currentItem._dir << " failed to start");
            fail(slot.currentItem);
            slot.busy = false;
            slot.repository.reset();
            return;
        }

        try {
            slot.repository->update();
        } catch (sg_exception& e) {
            SG_LOG(SG_TERRASYNC, SG_INFO, "sync of " << slot.repository->baseUrl() << " failed to start with error:"
                   << e.getFormattedMessage());
            fail(slot.currentItem);
            slot.busy = false;
            slot.repository.reset();
            return;
        }

        slot.nextWarnTimeout = 30 * 1000;
        slot.stamp.stamp();
        slot.busy = true;
        slot.pendingKBytes = slot.repository->bytesToDownload() >> 10;
        slot.pendingExtractKBytes = slot.repository->bytesToExtract() >> 10;

        SG_LOG(SG_TERRASYNC, SG_INFO, "sync of " << slot.repository->baseUrl() << ":" << slot.currentItem._dir << " started, queue size is " << slot.queue.size());
    }
}

bool SGTerraSync::WorkerThread::beginSyncAirports(SyncSlot& slot)
{
    if (!slot.isNewDirectory) {
        return beginNormalSync(slot);
    }

    SG_LOG(SG_TERRASYNC, SG_INFO, "doing Airports download via tarball");

    // we want to sync the 'root' TerraSync dir, but not all of it, just
    // the Airports_archive.tar.gz file so we use our TerraSync local root
    // as the path (since the archive will add Airports/)
    slot.repository.reset(new HTTPRepository(_local_dir, &_http));
    slot.repository->setBaseUrl(_httpServer);

    // filter callback to *only* sync the Airport_archive tarball,
    // and ensure no other contents are touched
    auto f = [](const HTTPRepository::SyncItem& item) {
        if (!item.directory.empty())
            return false;
        return (item.filename.find("Airports_archive.") == 0);
    };

    slot.repository->setFilter(f);
    return true;
}

bool SGTerraSync::WorkerThread::beginSyncTile(SyncSlot& slot)
{
    // avoid 404 requests by doing a sync which excludes all paths
    // except our tile path. In the case of a missing 1x1 tile, we will
    // stop becuase all directories are filtered out, which is what we want

    auto comps = strutils::split(slot.currentItem._dir, "/");
    if (comps.size() != 3) {
        SG_LOG(SG_TERRASYNC, SG_ALERT, "Bad tile path:" << slot.currentItem._dir);
        return false;
    }

    const auto tileCategory = comps.front();
    const auto tenByTenDir = comps.at(1);
    const auto oneByOneDir = comps.at(2);

    const auto path = SGPath::fromUtf8(_local_dir) / tileCategory;
    slot.repository.reset(new HTTPRepository(path, &_http));

    if (slot.currentItem._type == SyncItem::OSMTile) {
        if (_osmCityServer.empty()) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "No OSM2City server defined for:" << slot.currentItem._dir);
            return false;
        }

        slot.repository->setBaseUrl(_osmCityServer + "/" + tileCategory);
    } else {
        slot.repository->setBaseUrl(_httpServer + "/" + tileCategory);
    }

    if (_installRoot.exists()) {
      SGPath p = _installRoot / tileCategory;
      slot.repository->setInstalledCopyPath(p);
    }

    const auto dirPrefix = tenByTenDir + "/" + oneByOneDir;
    using simgear::strutils::starts_with;

    // filter callback to *only* sync the 1x1 dir we want, if it exists
    // if doesn't, we'll simply stop, which is what we want
    auto f = [tenByTenDir, oneByOneDir, dirPrefix](const HTTPRepository::SyncItem& item) {
        // only allow the specific 10x10 and 1x1 dirs we want
        if (item.directory.empty()) {
            return item.filename == tenByTenDir;
        } else if (item.directory == tenByTenDir) {
            // allow 10x10 dir to contain 1x1_dir.tgz/.zip and still be accepted
            // this does mean we'd also download oneByOneDir_foobar but that
            // doesn't seem unreasonable either
            return starts_with(item.filename, oneByOneDir);
        }

        // allow arbitrary children below dirPrefix, including sub-dirs
        if (item.directory.find(dirPrefix) == 0) {
            return true;
        }

        SG_LOG(SG_TERRASYNC, SG_ALERT, "Tile sync: saw weird path:" << item.directory << " file " << item.filename);
        return false;
    };

    slot.repository->setFilter(f);
    return true;
}

bool SGTerraSync::WorkerThread::beginNormalSync(SyncSlot& slot)
{
    SGPath path(_local_dir);
    path.append(slot.currentItem._dir);
    slot.repository.reset(new HTTPRepository(path, &_http));
    slot.repository->setBaseUrl(_httpServer + "/" + slot.currentItem._dir);

    if (_installRoot.exists()) {
      SGPath p = _installRoot;
      p.append(slot.currentItem._dir);
      slot.repository->setInstalledCopyPath(p);
    }

    return true;
}

void SGTerraSync::WorkerThread::runInternal()
{
    while (!_stop) {
        // try to find a terrasync server
        if( !hasServer() ) {
            const auto haveServer = findServer();
            if (haveServer) {
                hasServer(true);

                std::lock_guard<std::mutex> g(_stateLock);
                _state._consecutive_errors = 0;

                SG_LOG(SG_TERRASYNC, SG_INFO, "terrasync scenery provider of the day is '" << _httpServer << "'");
            } else {
                std::lock_guard<std::mutex> g(_stateLock);
                _state._consecutive_errors++;
            }
            continue;
        }

        try {
            _http.update(10);
        } catch (sg_exception& e) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "failure doing HTTP update" << e.getFormattedMessage());
        }

        {
            std::lock_guard<std::mutex> g(_stateLock);
            _state._transfer_rate = _http.transferRateBytesPerSec();
            // convert from bytes to kbytes
            _state._total_kb_downloaded = static_cast<int>(_http.totalBytesDownloaded() / 1024);
        }

        if (_stop)
            break;

        drainWaitingTiles();

        bool anySlotBusy = false;
        unsigned int newPendingCount = 0;
        unsigned int newExtractCount = 0; // how much is left to extract

        // update each sync slot in turn
        for (unsigned int slot=0; slot < NUM_SYNC_SLOTS; ++slot) {
            updateSyncSlot(_syncSlots[slot]);
            newPendingCount += _syncSlots[slot].pendingKBytes;
            newExtractCount += _syncSlots[slot].pendingExtractKBytes;
            anySlotBusy |= _syncSlots[slot].busy;
        }

        {
            std::lock_guard<std::mutex> g(_stateLock);
            _state._totalKbPending = newPendingCount; // approximately atomic update
            _state._extractTotalKbPending = newExtractCount;
            _state._busy = anySlotBusy;
        }


        if (!anySlotBusy) {
            // wait on the blocking deque here, otherwise we spin
            // the loop very fast, since _http::update with no connections
            // active returns immediately.
            waitingTiles.waitOnNotEmpty();
        }
    } // of thread running loop
}

SyncItem::Status SGTerraSync::WorkerThread::isPathCached(const SyncItem& next) const
{
    auto ii = _completedTiles.find(next._dir);
    if (ii == _completedTiles.end()) {
        ii = _notFoundItems.find( next._dir );
        // Invalid means 'not cached', otherwise we want to return to
        // higher levels the cache status
        return (ii == _notFoundItems.end()) ? SyncItem::Invalid : SyncItem::NotFound;
    }

    // check if the path still physically exists. This is needed to
    // cope with the user manipulating our cache dir
    SGPath p(_local_dir);
    p.append(next._dir);
    if (!p.exists()) {
        return SyncItem::Invalid;
    }

    time_t now = time(0);
    return (ii->second > now) ? SyncItem::Cached : SyncItem::Invalid;
}

void SGTerraSync::WorkerThread::fail(SyncItem failedItem)
{
    std::lock_guard<std::mutex> g(_stateLock);
    time_t now = time(0);

    if (_osmCityServer.empty() && (failedItem._type == SyncItem::OSMTile)) {
        // don't count these as errors, otherwise normla sync will keep
        // being abandoned
    } else {
        _state._consecutive_errors++;
        _state._fail_count++;
    }

    failedItem._status = SyncItem::Failed;
    _freshTiles.push_back(failedItem);
    // not we also end up here for partial syncs
    SG_LOG(SG_TERRASYNC, SG_WARN,
           "Failed to sync'" << failedItem._dir << "'");
    _completedTiles[ failedItem._dir ] = now + UpdateInterval::FailedAttempt;
}

void SGTerraSync::WorkerThread::notFound(SyncItem item)
{
    // treat not found as authorative, so use the same cache expiry
    // as succesful download. Important for MP models and similar so
    // we don't spam the server with lookups for models that don't
    // exist

    SG_LOG(SG_TERRASYNC, SG_WARN, "Not found for: '" << item._dir << "'");

    time_t now = time(0);
    item._status = SyncItem::NotFound;
    _freshTiles.push_back(item);
    _notFoundItems[ item._dir ] = now + UpdateInterval::SuccessfulAttempt;
    writeCompletedTilesPersistentCache();
}

void SGTerraSync::WorkerThread::updated(SyncItem item, bool isNewDirectory)
{
    {
        std::lock_guard<std::mutex> g(_stateLock);
        time_t now = time(0);
        _state._consecutive_errors = 0;
        _state._success_count++;
        SG_LOG(SG_TERRASYNC,SG_INFO,
               "Successfully synchronized directory '" << item._dir << "'");

        item._status = SyncItem::Updated;
        if (item._type == SyncItem::Tile) {
            _state._updated_tile_count++;
        }

        _freshTiles.push_back(item);
        _completedTiles[ item._dir ] = now + UpdateInterval::SuccessfulAttempt;
    }

    writeCompletedTilesPersistentCache();
}

void SGTerraSync::WorkerThread::drainWaitingTiles()
{
    // drain the waiting tiles queue into the sync slot queues.
    while (!waitingTiles.empty()) {
        SyncItem next = waitingTiles.pop_front();
        SyncItem::Status cacheStatus = isPathCached(next);
        if (cacheStatus != SyncItem::Invalid) {
            incrementCacheHits();
            SG_LOG(SG_TERRASYNC, SG_BULK, "\nTerraSync Cache hit for: '" << next._dir << "'");
            next._status = cacheStatus;
            _freshTiles.push_back(next);
            continue;
        }

        const auto slot = syncSlotForType(next._type);
        _syncSlots[slot].queue.push_back(next);
    }
}

bool SGTerraSync::WorkerThread::isDirActive(const std::string& path) const
{
    // check waiting tiles first. we have to copy it to check safely,
    // but since it's normally empty, this is not a big deal.
    const auto copyOfWaiting = waitingTiles.copy();
    auto it = std::find_if(copyOfWaiting.begin(), copyOfWaiting.end(), [&path](const SyncItem& i) {
        return i._dir == path;
    });

    if (it != copyOfWaiting.end()) {
        return true;
    }

    // check each sync slot in turn
    std::lock_guard<std::mutex> g(_stateLock);
    for (unsigned int slot = 0; slot < NUM_SYNC_SLOTS; ++slot) {
        const auto& syncSlot = _syncSlots[slot];
        if (syncSlot.currentItem._dir == path)
            return true;

        auto it = std::find_if(syncSlot.queue.begin(), syncSlot.queue.end(), [&path](const SyncItem& i) {
            return i._dir == path;
        });

        if (it != syncSlot.queue.end()) {
            return true;
        }
    } // of sync slots iteration

    return false;
}

void SGTerraSync::WorkerThread::initCompletedTilesPersistentCache() {
  if (!_persistentCachePath.exists()) {
    return;
  }

  SGPropertyNode_ptr cacheRoot(new SGPropertyNode);
  time_t now = time(0);

  try {
    readProperties(_persistentCachePath, cacheRoot);
  } catch (sg_exception &e) {
    SG_LOG(SG_TERRASYNC, SG_INFO, "corrupted persistent cache, discarding");
    return;
  }

  for (int i = 0; i < cacheRoot->nChildren(); ++i) {
    SGPropertyNode *entry = cacheRoot->getChild(i);
    bool isNotFound = (strcmp(entry->getName(), "not-found") == 0);
    string tileName = entry->getStringValue("path");
    time_t stamp = entry->getIntValue("stamp");
    if (stamp < now) {
      continue;
    }

    if (isNotFound) {
      _notFoundItems[tileName] = stamp;
    } else {
      _completedTiles[tileName] = stamp;
    }
  }
}

void SGTerraSync::WorkerThread::writeCompletedTilesPersistentCache() const {
  // cache is disabled
  if (_persistentCachePath.isNull()) {
    return;
  }

  sg_ofstream f(_persistentCachePath, std::ios::trunc);
  if (!f.is_open()) {
    return;
  }

  SGPropertyNode_ptr cacheRoot(new SGPropertyNode);
  TileAgeCache::const_iterator it = _completedTiles.begin();
  for (; it != _completedTiles.end(); ++it) {
    SGPropertyNode *entry = cacheRoot->addChild("entry");
    entry->setStringValue("path", it->first);
    entry->setIntValue("stamp", it->second);
  }

  it = _notFoundItems.begin();
  for (; it != _notFoundItems.end(); ++it) {
    SGPropertyNode *entry = cacheRoot->addChild("not-found");
    entry->setStringValue("path", it->first);
    entry->setIntValue("stamp", it->second);
  }

  writeProperties(f, cacheRoot, true /* write_all */);
  f.close();
}

///////////////////////////////////////////////////////////////////////////////
// SGTerraSync ////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SGTerraSync::SGTerraSync() :
    _workerThread(NULL),
    _bound(false),
    _inited(false)
{
    _workerThread = new WorkerThread();
    _log = new BufferedLogCallback(SG_TERRASYNC, SG_INFO);
    _log->truncateAt(255);

    sglog().addCallback(_log);
}

SGTerraSync::~SGTerraSync()
{
    delete _workerThread;
    _workerThread = NULL;
    sglog().removeCallback(_log);
    delete _log;
     _tiedProperties.Untie();
}

void SGTerraSync::setRoot(SGPropertyNode_ptr root)
{
    if (!root) {
        _terraRoot.clear();
        _renderingRoot.clear();
        return;
    }

    _terraRoot = root->getNode("/sim/terrasync",true);
    _renderingRoot = root->getNode("/sim/rendering", true);
}

void SGTerraSync::init()
{
    if (_inited) {
        return;
    }

    _inited = true;

    assert(_terraRoot);

    reinit();
}

void SGTerraSync::shutdown()
{
    SG_LOG(SG_TERRASYNC, SG_INFO, "Shutdown");
     _workerThread->stop();
}

void SGTerraSync::reinit()
{
    auto enabled = _enabledNode->getBoolValue();
    // do not reinit when enabled and we're already up and running
    if (enabled  && _workerThread->isRunning())
    {
        _availableNode->setBoolValue(true);
        return;
    }
    _stalledNode->setBoolValue(false);
    _workerThread->stop();

    if (enabled)
    {
        _availableNode->setBoolValue(true);
        _workerThread->setHTTPServer(
            _terraRoot->getStringValue("http-server", "automatic"),
            _terraRoot->getStringValue("osm2city-server", ""));
        _workerThread->setSceneryVersion( _terraRoot->getStringValue("scenery-version","ws20") );
        _workerThread->setOSMCityVersion(_terraRoot->getStringValue("osm2city-version", "o2c"));
        _workerThread->setProtocol( _terraRoot->getStringValue("protocol","") );

        if (_terraRoot->hasChild("dns-server")) {
            auto ns = _terraRoot->getStringValue("dns-server");
            if (ns == "google") {
                ns = "8.8.8.8";
            }
            
            SG_LOG(SG_TERRASYNC,SG_INFO,"DNS server override:" << ns);
            _workerThread->setDNSServer(ns);
        }
#if 1
        // leave it hardcoded for now, not sure about the security implications for now
        _workerThread->setDNSDN( "terrasync.flightgear.org");
#else
        _workerThread->setDNSDN( _terraRoot->getStringValue("dnsdn","terrasync.flightgear.org") );
#endif

        SGPath sceneryRoot{_terraRoot->getStringValue("scenery-dir", "")};
        _workerThread->setLocalDir(sceneryRoot.utf8Str());
        if (sceneryRoot.exists()) {
            writeWarningFile(sceneryRoot);
        }

        SGPath installPath(_terraRoot->getStringValue("installation-dir"));
        _workerThread->setInstalledDir(installPath);

        if (_terraRoot->getBoolValue("enable-persistent-cache", true)) {
          _workerThread->setCachePath(sceneryRoot / "RecheckCache");
        }

        _workerThread->setCacheHits(_terraRoot->getIntValue("cache-hit", 0));

        if (_workerThread->start())
        {
            syncAirportsModels();
        }
    }
    else
        _availableNode->setBoolValue(false);

    _stalledNode->setBoolValue(_workerThread->isStalled());
}

void SGTerraSync::bind()
{
    if (_bound) {
        return;
    }

    _bound = true;

    //_terraRoot->getNode("use-built-in-svn", true)->setAttribute(SGPropertyNode::USERARCHIVE,false);
    //_terraRoot->getNode("use-svn", true)->setAttribute(SGPropertyNode::USERARCHIVE,false);
    _terraRoot->getNode("intialized", true)->setBoolValue(true);

    // stalled is used as a signal handler (to connect listeners triggering GUI pop-ups)
    _stalledNode = _terraRoot->getNode("stalled", true);
    _stalledNode->setBoolValue(_workerThread->isStalled());
//    _stalledNode->setAttribute(SGPropertyNode::PRESERVE,true);

    _activeNode = _terraRoot->getNode("active", true);

    _busyNode = _terraRoot->getNode("busy", true);
    _updateCountNode = _terraRoot->getNode("update-count", true);
    _errorCountNode = _terraRoot->getNode("error-count", true);
    _tileCountNode = _terraRoot->getNode("tile-count", true);
    _cacheHitsNode = _terraRoot->getNode("cache-hits", true);
    _transferRateBytesSecNode = _terraRoot->getNode("transfer-rate-bytes-sec", true);
    _pendingKbytesNode = _terraRoot->getNode("pending-kbytes", true);
    _downloadedKBtesNode = _terraRoot->getNode("downloaded-kbytes", true);
    _extractPendingKbytesNode = _terraRoot->getNode("extract-pending-kbytes", true);
    _enabledNode = _terraRoot->getNode("enabled", true);
    _availableNode = _terraRoot->getNode("available", true);
    _maxErrorsNode = _terraRoot->getNode("max-errors", true);
}

void SGTerraSync::unbind()
{
    _workerThread->stop();
    _tiedProperties.Untie();
    _bound = false;
    _inited = false;

    _terraRoot.clear();
    _stalledNode.clear();
    _activeNode.clear();
    _cacheHits.clear();
    _renderingRoot.clear();
    _busyNode.clear();
    _updateCountNode.clear();
    _errorCountNode.clear();
    _tileCountNode.clear();
    _cacheHitsNode.clear();
    _transferRateBytesSecNode.clear();
    _pendingKbytesNode.clear();
    _downloadedKBtesNode.clear();
    _extractPendingKbytesNode.clear();
    _enabledNode.clear();
    _availableNode.clear();
    _maxErrorsNode.clear();
}

void SGTerraSync::update(double)
{
    auto enabled = _enabledNode->getBoolValue();
    auto worker_running = _workerThread->isRunning();

    // hold enabled false until retry time passes
    if (enabled && (_retryTime > SGTimeStamp::now())) {
        enabled = false;
    }

    // see if the enabled status has changed; and if so take the appropriate action.
    if (enabled && !worker_running)
    {
        reinit();
        SG_LOG(SG_TERRASYNC, SG_MANDATORY_INFO, "Terrasync started");
    }
    else if (!enabled && worker_running)
    {
        reinit();
        SG_LOG(SG_TERRASYNC, SG_MANDATORY_INFO, "Terrasync stopped");
    }
    TerrasyncThreadState copiedState(_workerThread->threadsafeCopyState());

    _busyNode->setIntValue(copiedState._busy);
    _updateCountNode->setIntValue(copiedState._success_count);
    _errorCountNode->setIntValue(copiedState._fail_count);
    _tileCountNode->setIntValue(copiedState._updated_tile_count);
    _cacheHitsNode->setIntValue(copiedState._cache_hits);
    _transferRateBytesSecNode->setIntValue(copiedState._transfer_rate);
    _pendingKbytesNode->setIntValue(copiedState._totalKbPending);
    _downloadedKBtesNode->setIntValue(copiedState._total_kb_downloaded);
    _extractPendingKbytesNode->setIntValue(copiedState._extractTotalKbPending);

    _stalledNode->setBoolValue(_workerThread->isStalled());
    _activeNode->setBoolValue(worker_running);

    int allowedErrors = _maxErrorsNode->getIntValue();
    if (worker_running && (copiedState._consecutive_errors >= allowedErrors)) {
        _workerThread->stop();

        _retryBackOffSeconds = std::min(_retryBackOffSeconds + 60, 60u * 15);
        const int seconds = static_cast<int>(sg_random() * _retryBackOffSeconds);
        _retryTime = SGTimeStamp::now() + SGTimeStamp::fromSec(seconds);
        SG_LOG(SG_TERRASYNC, SG_ALERT, "Terrasync paused due to " << copiedState._consecutive_errors << " consecutive errors during sync; will retry in " << seconds << " seconds.");
    }

    while (_workerThread->hasNewTiles())
    {
        // ensure they are popped
        _workerThread->getNewTile();
    }
}

bool SGTerraSync::isIdle() {return _workerThread->isIdle();}

void SGTerraSync::syncAirportsModels()
{
  SyncItem w("Airports", SyncItem::AirportData);
  SyncItem a("Models", SyncItem::SharedModels);
  _workerThread->request(w);
  _workerThread->request(a);
}

string_list SGTerraSync::getSceneryPathSuffixes() const
{
    string_list scenerySuffixes;

    for (auto node : _renderingRoot->getChildren("scenery-path-suffix")) {
        if (node->getBoolValue("enabled", true) && node->hasValue("name")) {
            scenerySuffixes.push_back(node->getStringValue("name"));
        }
    }

    if (scenerySuffixes.empty()) {
        // if preferences didn't load, use some default
        scenerySuffixes = {"Objects", "Terrain"}; // default values
    }

    return scenerySuffixes;
}

bool isOSMSuffix(const std::string& suffix)
{
    return (suffix == "Buildings") || (suffix == "Roads") || (suffix == "Pylons") || (suffix == "Details");
}

void SGTerraSync::syncAreaByPath(const std::string& aPath)
{
    if (!_workerThread->isRunning()) {
        return;
    }

    for (const auto& suffix : getSceneryPathSuffixes()) {
        const auto dir = suffix + "/" + aPath;
        if (_workerThread->isDirActive(dir)) {
            continue;
        }
        SyncItem w(dir, isOSMSuffix(suffix) ? SyncItem::OSMTile : SyncItem::Tile);
        _workerThread->request( w );
    }
}

bool SGTerraSync::scheduleTile(const SGBucket& bucket)
{
    std::string basePath = bucket.gen_base_path();
    syncAreaByPath(basePath);
    return true;
}

bool SGTerraSync::isTileDirPending(const std::string& sceneryDir) const
{
    if (!_workerThread->isRunning()) {
        return false;
    }

    for (const auto& suffix : getSceneryPathSuffixes()) {
        // don't wait on OSM dirs, even if enabled
        if (isOSMSuffix(suffix)) {
            continue;
        }

        const auto s = suffix + "/" + sceneryDir;
        if (_workerThread->isDirActive(s)) {
            return true;
        }
    }

    return false;
}

void SGTerraSync::scheduleDataDir(const std::string& dataDir)
{
    if (!_workerThread->isRunning()) {
        return;
    }

    if (_workerThread->isDirActive(dataDir)) {
        return;
    }

    SyncItem w(dataDir, SyncItem::AIData);
    _workerThread->request( w );
}

bool SGTerraSync::isDataDirPending(const std::string& dataDir) const
{
    if (!_workerThread->isRunning()) {
        return false;
    }

    return _workerThread->isDirActive(dataDir);
}

void SGTerraSync::reposition()
{
    // stub, remove
}

void SGTerraSync::writeWarningFile(const SGPath& sceneryDir)
{
    SGPath p = sceneryDir / "TerraSync-WARNING.txt";
    if (p.exists())
        return;

    sg_ofstream os(p, std::ios::out | std::ios::trunc);
    os << "This folder is managed by FlightGear's download system.\n";
    os << "Any changes you make here or in sub-folders will be overwritten when TerraSync\n";
    os << "downloads updates.\n";
    os << "\n";
    os << "To use custom scenery or data with FlightGear, put it in a differnet location\n";
    os << "on your computer, then add the location using either the launcher 'Add-ons' page, or by\n";
    os << "passing '--fg-scenery=<location>' on the command line.";
    os << endl;
}


// Register the subsystem.
SGSubsystemMgr::Registrant<SGTerraSync> registrantSGTerraSync(
    SGSubsystemMgr::GENERAL,
    {{"FGRenderer", SGSubsystemMgr::Dependency::NONSUBSYSTEM_HARD}});
