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

#include <simgear/version.h>

#include "terrasync.hxx"

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/threads/SGQueue.hxx>
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
    enum Type
    {
        Stop = 0,   ///< special item indicating to stop the SVNThread
        Tile,
        AirportData,
        SharedModels,
        AIData
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
    SyncSlot() :
        isNewDirectory(false),
        busy(false)
    {}

    SyncItem currentItem;
    bool isNewDirectory;
    std::queue<SyncItem> queue;
    std::auto_ptr<HTTPRepository> repository;
    SGTimeStamp stamp;
    bool busy; ///< is the slot working or idle

    unsigned int nextWarnTimeout;
};

static const int SYNC_SLOT_TILES = 0; ///< Terrain and Objects sync
static const int SYNC_SLOT_SHARED_DATA = 1; /// shared Models and Airport data
static const int SYNC_SLOT_AI_DATA = 2; /// AI traffic and models
static const unsigned int NUM_SYNC_SLOTS = 3;

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

    default:
        return SYNC_SLOT_SHARED_DATA;
    }
}


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

    bool isIdle() {return !_busy; }
   void request(const SyncItem& dir) {waitingTiles.push_front(dir);}
   bool isDirty() { bool r = _is_dirty;_is_dirty = false;return r;}
   bool hasNewTiles() { return !_freshTiles.empty();}
   SyncItem getNewTile() { return _freshTiles.pop_front();}

   void   setHTTPServer(const std::string& server)
   {
      _httpServer = stripPath(server);
   }

   void   setLocalDir(string dir)           { _local_dir    = stripPath(dir);}
   string getLocalDir()                     { return _local_dir;}
   void   setAllowedErrorCount(int errors)  {_allowed_errors = errors;}

   void   setCachePath(const SGPath& p)     {_persistentCachePath = p;}
   void   setCacheHits(unsigned int hits)   {_cache_hits = hits;}

   volatile bool _active;
   volatile bool _running;
   volatile bool _busy;
   volatile bool _stalled;
   volatile int  _fail_count;
   volatile int  _updated_tile_count;
   volatile int  _success_count;
   volatile int  _consecutive_errors;
   volatile int  _allowed_errors;
   volatile int  _cache_hits;
   volatile int _transfer_rate;
   // kbytes, not bytes, because bytes might overflow 2^31
   volatile int _total_kb_downloaded;

private:
   virtual void run();

    // internal mode run and helpers
    void runInternal();
    void updateSyncSlot(SyncSlot& slot);

    // commond helpers between both internal and external models

    SyncItem::Status isPathCached(const SyncItem& next) const;
    void initCompletedTilesPersistentCache();
    void writeCompletedTilesPersistentCache() const;
    void updated(SyncItem item, bool isNewDirectory);
    void fail(SyncItem failedItem);
    void notFound(SyncItem notFoundItem);

    HTTP::Client _http;
    SyncSlot _syncSlots[NUM_SYNC_SLOTS];

   volatile bool _is_dirty;
   volatile bool _stop;
   SGBlockingDeque <SyncItem> waitingTiles;

   TileAgeCache _completedTiles;
   TileAgeCache _notFoundItems;

   SGBlockingDeque <SyncItem> _freshTiles;
   string _local_dir;
   SGPath _persistentCachePath;
   string _httpServer;
};

SGTerraSync::WorkerThread::WorkerThread() :
    _active(false),
    _running(false),
    _busy(false),
    _stalled(false),
    _fail_count(0),
    _updated_tile_count(0),
    _success_count(0),
    _consecutive_errors(0),
    _allowed_errors(6),
    _cache_hits(0),
    _transfer_rate(0),
    _total_kb_downloaded(0),
    _is_dirty(false),
    _stop(false)
{
    _http.setUserAgent("terrascenery-" SG_STRINGIZE(SIMGEAR_VERSION));
}

void SGTerraSync::WorkerThread::stop()
{
    // drop any pending requests
    waitingTiles.clear();

    if (!_running)
        return;

    // set stop flag and wake up the thread with an empty request
    _stop = true;
    SyncItem w(string(), SyncItem::Stop);
    request(w);
    join();
    _running = false;
}

bool SGTerraSync::WorkerThread::start()
{
    if (_running)
        return false;

    if (_local_dir=="")
    {
        SG_LOG(SG_TERRASYNC,SG_ALERT,
               "Cannot start scenery download. Local cache directory is undefined.");
        _fail_count++;
        _stalled = true;
        return false;
    }

    SGPath path(_local_dir);
    if (!path.exists())
    {
        SG_LOG(SG_TERRASYNC,SG_ALERT,
               "Cannot start scenery download. Directory '" << _local_dir <<
               "' does not exist. Set correct directory path or create directory folder.");
        _fail_count++;
        _stalled = true;
        return false;
    }

    path.append("version");
    if (path.exists())
    {
        SG_LOG(SG_TERRASYNC,SG_ALERT,
               "Cannot start scenery download. Directory '" << _local_dir <<
               "' contains the base package. Use a separate directory.");
        _fail_count++;
        _stalled = true;
        return false;
    }

    _fail_count = 0;
    _updated_tile_count = 0;
    _success_count = 0;
    _consecutive_errors = 0;
    _stop = false;
    _stalled = false;
    _running = true;

    string status = "Using built-in HTTP support.";

    // not really an alert - but we want to (always) see this message, so user is
    // aware we're downloading scenery (and using bandwidth).
    SG_LOG(SG_TERRASYNC,SG_ALERT,
           "Starting automatic scenery download/synchronization. "
           << status
           << "Directory: '" << _local_dir << "'.");

    SGThread::start();
    return true;
}

void SGTerraSync::WorkerThread::run()
{
    _active = true;
    initCompletedTilesPersistentCache();

    if (_httpServer == "automatic" ) {

        //TODO: make DN and service settable from properties
        DNS::NAPTRRequest * naptrRequest = new DNS::NAPTRRequest("terrasync.flightgear.org");
        naptrRequest->qservice = "ws20";

        naptrRequest->qflags = "U";
        DNS::Request_ptr r(naptrRequest);

        DNS::Client dnsClient;
        dnsClient.makeRequest(r);
        while( !r->isComplete() && !r->isTimeout() )
          dnsClient.update(0);

        if( naptrRequest->entries.empty() ) {
            SG_LOG(SG_TERRASYNC, SG_ALERT, "ERROR: automatic terrasync http-server requested, but no DNS entry found.");
            _httpServer = "";
        } else {
            // walk through responses, they are ordered by 1. order and 2. preference
            // For now, only take entries with lowest order
            // TODO: try all available servers in the order given by preferenc and order
            int order = naptrRequest->entries[0]->order;

            // get all servers with this order and the same (for now only lowest preference)
            DNS::NAPTRRequest::NAPTR_list availableServers;
                for( DNS::NAPTRRequest::NAPTR_list::const_iterator it = naptrRequest->entries.begin();
                  it != naptrRequest->entries.end();
                  ++it ) {

                if( (*it)->order != order )
                    continue;

                string regex = (*it)->regexp;
                if( false == simgear::strutils::starts_with( (*it)->regexp, "!^.*$!" ) ) {
                    SG_LOG(SG_TERRASYNC,SG_WARN, "ignoring unsupported regexp: " << (*it)->regexp );
                    continue;
                }

                if( false == simgear::strutils::ends_with( (*it)->regexp, "!" ) ) {
                    SG_LOG(SG_TERRASYNC,SG_WARN, "ignoring unsupported regexp: " << (*it)->regexp );
                    continue;
                }

                // always use first entry
                if( availableServers.empty() || (*it)->preference == availableServers[0]->preference) {
                    SG_LOG(SG_TERRASYNC,SG_DEBUG, "available server regexp: " << (*it)->regexp );
                    availableServers.push_back( *it );
                }
          }

          // now pick a random entry from the available servers
          DNS::NAPTRRequest::NAPTR_list::size_type idx = sg_random() * availableServers.size();
          _httpServer = availableServers[idx]->regexp;
          _httpServer = _httpServer.substr( 6, _httpServer.length()-7 ); // strip search pattern and separators

          SG_LOG(SG_TERRASYNC,SG_INFO, "picking entry # " << idx << ", server is " << _httpServer );
        }
    }

    runInternal();

    _active = false;
    _running = false;
    _is_dirty = true;
}

void SGTerraSync::WorkerThread::updateSyncSlot(SyncSlot &slot)
{
    if (slot.repository.get()) {
        if (slot.repository->isDoingSync()) {
#if 1
            if (slot.stamp.elapsedMSec() > (int)slot.nextWarnTimeout) {
                SG_LOG(SG_TERRASYNC, SG_INFO, "sync taking a long time:" << slot.currentItem._dir << " taken " << slot.stamp.elapsedMSec());
                SG_LOG(SG_TERRASYNC, SG_INFO, "HTTP request count:" << _http.hasActiveRequests());
                slot.nextWarnTimeout += 10000;
            }
#endif
            return; // easy, still working
        }

        // check result
        HTTPRepository::ResultCode res = slot.repository->failure();
        if (res == HTTPRepository::REPO_ERROR_NOT_FOUND) {
            notFound(slot.currentItem);
        } else if (res != HTTPRepository::REPO_NO_ERROR) {
            fail(slot.currentItem);
        } else {
            updated(slot.currentItem, slot.isNewDirectory);
            SG_LOG(SG_TERRASYNC, SG_DEBUG, "sync of " << slot.repository->baseUrl() << " finished ("
                   << slot.stamp.elapsedMSec() << " msec");
        }

        // whatever happened, we're done with this repository instance
        slot.busy = false;
        slot.repository.reset();
    }

    // init and start sync of the next repository
    if (!slot.queue.empty()) {
        slot.currentItem = slot.queue.front();
        slot.queue.pop();

        SGPath path(_local_dir);
        path.append(slot.currentItem._dir);
        slot.isNewDirectory = !path.exists();
        if (slot.isNewDirectory) {
            int rc = path.create_dir( 0755 );
            if (rc) {
                SG_LOG(SG_TERRASYNC,SG_ALERT,
                       "Cannot create directory '" << path << "', return code = " << rc );
                fail(slot.currentItem);
                return;
            }
        } // of creating directory step

        slot.repository.reset(new HTTPRepository(path, &_http));
        slot.repository->setBaseUrl(_httpServer + "/" + slot.currentItem._dir);

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

        slot.nextWarnTimeout = 20000;
        slot.stamp.stamp();
        slot.busy = true;
        SG_LOG(SG_TERRASYNC, SG_INFO, "sync of " << slot.repository->baseUrl() << " started, queue size is " << slot.queue.size());
    }
}

void SGTerraSync::WorkerThread::runInternal()
{
    while (!_stop) {
        try {
            _http.update(100);
        } catch (sg_exception& e) {
            SG_LOG(SG_TERRASYNC, SG_WARN, "failure doing HTTP update" << e.getFormattedMessage());
        }

        _transfer_rate = _http.transferRateBytesPerSec();
        // convert from bytes to kbytes
        _total_kb_downloaded = static_cast<int>(_http.totalBytesDownloaded() / 1024);

        if (_stop)
            break;

        // drain the waiting tiles queue into the sync slot queues.
        while (!waitingTiles.empty()) {
            SyncItem next = waitingTiles.pop_front();
            SyncItem::Status cacheStatus = isPathCached(next);
            if (cacheStatus != SyncItem::Invalid) {
                _cache_hits++;
                SG_LOG(SG_TERRASYNC, SG_DEBUG, "\nTerraSync Cache hit for: '" << next._dir << "'");
                next._status = cacheStatus;
                _freshTiles.push_back(next);
                _is_dirty = true;
                continue;
            }

            unsigned int slot = syncSlotForType(next._type);
            _syncSlots[slot].queue.push(next);
        }

        bool anySlotBusy = false;
        // update each sync slot in turn
        for (unsigned int slot=0; slot < NUM_SYNC_SLOTS; ++slot) {
            updateSyncSlot(_syncSlots[slot]);
            anySlotBusy |= _syncSlots[slot].busy;
        }

        _busy = anySlotBusy;
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
    TileAgeCache::const_iterator ii = _completedTiles.find( next._dir );
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
    time_t now = time(0);
    _consecutive_errors++;
    _fail_count++;
    failedItem._status = SyncItem::Failed;
    _freshTiles.push_back(failedItem);
    SG_LOG(SG_TERRASYNC,SG_INFO,
           "Failed to sync'" << failedItem._dir << "'");
    _completedTiles[ failedItem._dir ] = now + UpdateInterval::FailedAttempt;
    _is_dirty = true;
}

void SGTerraSync::WorkerThread::notFound(SyncItem item)
{
    // treat not found as authorative, so use the same cache expiry
    // as succesful download. Important for MP models and similar so
    // we don't spam the server with lookups for models that don't
    // exist

    time_t now = time(0);
    item._status = SyncItem::NotFound;
    _freshTiles.push_back(item);
    _is_dirty = true;
    _notFoundItems[ item._dir ] = now + UpdateInterval::SuccessfulAttempt;
    writeCompletedTilesPersistentCache();
}

void SGTerraSync::WorkerThread::updated(SyncItem item, bool isNewDirectory)
{
    time_t now = time(0);
    _consecutive_errors = 0;
    _success_count++;
    SG_LOG(SG_TERRASYNC,SG_INFO,
           "Successfully synchronized directory '" << item._dir << "'");

    item._status = SyncItem::Updated;
    if (item._type == SyncItem::Tile) {
        _updated_tile_count++;
    }

    _freshTiles.push_back(item);
    _completedTiles[ item._dir ] = now + UpdateInterval::SuccessfulAttempt;
    _is_dirty = true;
    writeCompletedTilesPersistentCache();
}

void SGTerraSync::WorkerThread::initCompletedTilesPersistentCache()
{
    if (!_persistentCachePath.exists()) {
        return;
    }

    SGPropertyNode_ptr cacheRoot(new SGPropertyNode);
    time_t now = time(0);

    try {
        readProperties(_persistentCachePath.str(), cacheRoot);
    } catch (sg_exception& e) {
        SG_LOG(SG_TERRASYNC, SG_INFO, "corrupted persistent cache, discarding");
        return;
    }

    for (int i=0; i<cacheRoot->nChildren(); ++i) {
        SGPropertyNode* entry = cacheRoot->getChild(i);
        bool isNotFound = (strcmp(entry->getName(), "not-found") == 0);
        string tileName = entry->getStringValue("path");
        time_t stamp = entry->getIntValue("stamp");
        if (stamp < now) {
            continue;
        }

        if (isNotFound) {
            _completedTiles[tileName] = stamp;
        } else {
            _notFoundItems[tileName] = stamp;
        }
    }
}

void SGTerraSync::WorkerThread::writeCompletedTilesPersistentCache() const
{
    // cache is disabled
    if (_persistentCachePath.isNull()) {
        return;
    }

    std::ofstream f(_persistentCachePath.c_str(), std::ios::trunc);
    if (!f.is_open()) {
        return;
    }

    SGPropertyNode_ptr cacheRoot(new SGPropertyNode);
    TileAgeCache::const_iterator it = _completedTiles.begin();
    for (; it != _completedTiles.end(); ++it) {
        SGPropertyNode* entry = cacheRoot->addChild("entry");
        entry->setStringValue("path", it->first);
        entry->setIntValue("stamp", it->second);
    }

    it = _notFoundItems.begin();
    for (; it != _notFoundItems.end(); ++it) {
        SGPropertyNode* entry = cacheRoot->addChild("not-found");
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
    _terraRoot = root->getNode("/sim/terrasync",true);
}

void SGTerraSync::init()
{
    if (_inited) {
        return;
    }

    _inited = true;

    assert(_terraRoot);
    _terraRoot->setBoolValue("built-in-svn-available", true);

    reinit();
}

void SGTerraSync::shutdown()
{
     _workerThread->stop();
}

void SGTerraSync::reinit()
{
    // do not reinit when enabled and we're already up and running
    if ((_terraRoot->getBoolValue("enabled",false))&&
         (_workerThread->_active && _workerThread->_running))
    {
        return;
    }

    _workerThread->stop();

    if (_terraRoot->getBoolValue("enabled",false))
    {
        std::string httpServer(_terraRoot->getStringValue("http-server",""));
        _workerThread->setHTTPServer(httpServer);
        _workerThread->setLocalDir(_terraRoot->getStringValue("scenery-dir",""));
        _workerThread->setAllowedErrorCount(_terraRoot->getIntValue("max-errors",5));
        _workerThread->setCacheHits(_terraRoot->getIntValue("cache-hit", 0));

        if (_workerThread->start())
        {
            syncAirportsModels();
        }
    }

    _stalledNode->setBoolValue(_workerThread->_stalled);
}

void SGTerraSync::bind()
{
    if (_bound) {
        return;
    }

    _bound = true;
    _tiedProperties.Tie( _terraRoot->getNode("busy", true), (bool*) &_workerThread->_busy );
    _tiedProperties.Tie( _terraRoot->getNode("active", true), (bool*) &_workerThread->_active );
    _tiedProperties.Tie( _terraRoot->getNode("update-count", true), (int*) &_workerThread->_success_count );
    _tiedProperties.Tie( _terraRoot->getNode("error-count", true), (int*) &_workerThread->_fail_count );
    _tiedProperties.Tie( _terraRoot->getNode("tile-count", true), (int*) &_workerThread->_updated_tile_count );
    _tiedProperties.Tie( _terraRoot->getNode("cache-hits", true), (int*) &_workerThread->_cache_hits );
    _tiedProperties.Tie( _terraRoot->getNode("transfer-rate-bytes-sec", true), (int*) &_workerThread->_transfer_rate );

    // use kbytes here because propety doesn't support 64-bit and we might conceivably
    // download more than 2G in a single session
    _tiedProperties.Tie( _terraRoot->getNode("downloaded-kbytes", true), (int*) &_workerThread->_total_kb_downloaded );

    _terraRoot->getNode("busy", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("active", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("update-count", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("error-count", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("tile-count", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("use-built-in-svn", true)->setAttribute(SGPropertyNode::USERARCHIVE,false);
    _terraRoot->getNode("use-svn", true)->setAttribute(SGPropertyNode::USERARCHIVE,false);
    // stalled is used as a signal handler (to connect listeners triggering GUI pop-ups)
    _stalledNode = _terraRoot->getNode("stalled", true);
    _stalledNode->setBoolValue(_workerThread->_stalled);
    _stalledNode->setAttribute(SGPropertyNode::PRESERVE,true);
}

void SGTerraSync::unbind()
{
    _workerThread->stop();
    _tiedProperties.Untie();
    _bound = false;
    _inited = false;

    _terraRoot.clear();
    _stalledNode.clear();
    _cacheHits.clear();
}

void SGTerraSync::update(double)
{
    static SGBucket bucket;
    if (_workerThread->isDirty())
    {
        if (!_workerThread->_active)
        {
            if (_workerThread->_stalled)
            {
                SG_LOG(SG_TERRASYNC,SG_ALERT,
                       "Automatic scenery download/synchronization stalled. Too many errors.");
            }
            else
            {
                // not really an alert - just always show this message
                SG_LOG(SG_TERRASYNC,SG_ALERT,
                        "Automatic scenery download/synchronization has stopped.");
            }
            _stalledNode->setBoolValue(_workerThread->_stalled);
        }

        while (_workerThread->hasNewTiles())
        {
            SyncItem next = _workerThread->getNewTile();

            if ((next._type == SyncItem::Tile) || (next._type == SyncItem::AIData)) {
                _activeTileDirs.erase(next._dir);
            }
        } // of freshly synced items
    }
}

bool SGTerraSync::isIdle() {return _workerThread->isIdle();}

void SGTerraSync::syncAirportsModels()
{
    static const char* bounds = "MZAJKL"; // airport sync order: K-L, A-J, M-Z
    // note "request" method uses LIFO order, i.e. processes most recent request first
    for( unsigned i = 0; i < strlen(bounds)/2; i++ )
    {
        for ( char synced_other = bounds[2*i]; synced_other <= bounds[2*i+1]; synced_other++ )
        {
            ostringstream dir;
            dir << "Airports/" << synced_other;
            SyncItem w(dir.str(), SyncItem::AirportData);
            _workerThread->request( w );
        }
    }

    SyncItem w("Models", SyncItem::SharedModels);
    _workerThread->request( w );
}

void SGTerraSync::syncAreaByPath(const std::string& aPath)
{
    const char* terrainobjects[3] = { "Terrain/", "Objects/",  0 };
    for (const char** tree = &terrainobjects[0]; *tree; tree++)
    {
        std::string dir = string(*tree) + aPath;
        if (_activeTileDirs.find(dir) != _activeTileDirs.end()) {
            continue;
        }

        _activeTileDirs.insert(dir);
        SyncItem w(dir, SyncItem::Tile);
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
    if (!_workerThread->_running) {
        return false;
    }

    const char* terrainobjects[3] = { "Terrain/", "Objects/",  0 };
    for (const char** tree = &terrainobjects[0]; *tree; tree++) {
        string s = *tree + sceneryDir;
        if (_activeTileDirs.find(s) != _activeTileDirs.end()) {
            return true;
        }
    }

    return false;
}

void SGTerraSync::scheduleDataDir(const std::string& dataDir)
{
    if (_activeTileDirs.find(dataDir) != _activeTileDirs.end()) {
        return;
    }

    _activeTileDirs.insert(dataDir);
    SyncItem w(dataDir, SyncItem::AIData);
    _workerThread->request( w );

}

bool SGTerraSync::isDataDirPending(const std::string& dataDir) const
{
    if (!_workerThread->_running) {
        return false;
    }

    return (_activeTileDirs.find(dataDir) != _activeTileDirs.end());
}

void SGTerraSync::reposition()
{
    // stub, remove
}
