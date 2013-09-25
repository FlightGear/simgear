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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

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

#include <simgear/compiler.h>

#include "terrasync.hxx"

#include <simgear/bucket/newbucket.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/threads/SGQueue.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/debug/BufferedLogCallback.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/SVNRepository.hxx>
#include <simgear/structure/exception.hxx>

static const bool svn_built_in_available = true;

using namespace simgear;
using namespace std;

const char* rsync_cmd = 
        "rsync --verbose --archive --delete --perms --owner --group";

const char* svn_options =
        "checkout -q";

namespace UpdateInterval
{
    // interval in seconds to allow an update to repeat after a successful update (=daily)
    static const double SuccessfulAttempt = 24*60*60;
    // interval in seconds to allow another update after a failed attempt (10 minutes)
    static const double FailedAttempt     = 10*60;
}

typedef map<string,time_t> CompletedTiles;

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
// WaitingSyncItem ////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class WaitingSyncItem
{
public:
    enum Type
    {
        Stop = 0,   ///< special item indicating to stop the SVNThread
        Tile,
        AirportData,
        SharedModels
    };
    
    WaitingSyncItem() :
        _dir(),
        _type(Stop),
        _refreshScenery(false)
    {
    }
    
    WaitingSyncItem(string dir, Type ty) :
        _dir(dir),
        _type(ty),
        _refreshScenery(false)
    {}
        
    void setRefresh()
    { _refreshScenery = true; }
        
    string _dir;
    Type _type;
    bool _refreshScenery;
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
    WaitingSyncItem currentItem;
    bool isNewDirectory;
    std::queue<WaitingSyncItem> queue;
    std::auto_ptr<SVNRepository> repository;
    SGTimeStamp stamp;
};

static const int SYNC_SLOT_TILES = 0; ///< Terrain and Objects sync
static const int SYNC_SLOT_SHARED_DATA = 1; /// shared Models and Airport data
static const int NUM_SYNC_SLOTS = 2;

/**
 * @brief translate a sync item type into one of the available slots.
 * This provides the scheduling / balancing / prioritising between slots.
 */
static unsigned int syncSlotForType(WaitingSyncItem::Type ty)
{
    switch (ty) {
    case WaitingSyncItem::Tile: return SYNC_SLOT_TILES;
    case WaitingSyncItem::SharedModels:
    case WaitingSyncItem::AirportData:
        return SYNC_SLOT_SHARED_DATA;
    default:
        return SYNC_SLOT_SHARED_DATA;
    }
}


///////////////////////////////////////////////////////////////////////////////
// SGTerraSync::SvnThread /////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class SGTerraSync::SvnThread : public SGThread
{
public:
   SvnThread();
   virtual ~SvnThread( ) { stop(); }

   void stop();
   bool start();

   bool isIdle() {return waitingTiles.empty();}
   void request(const WaitingSyncItem& dir) {waitingTiles.push_front(dir);}
   bool isDirty() { bool r = _is_dirty;_is_dirty = false;return r;}
   bool hasNewTiles() { return !_freshTiles.empty();}
   WaitingSyncItem getNewTile() { return _freshTiles.pop_front();}

   void   setSvnServer(string server)       { _svn_server   = stripPath(server);}
   void   setExtSvnUtility(string svn_util) { _svn_command  = simgear::strutils::strip(svn_util);}
   void   setRsyncServer(string server)     { _rsync_server = simgear::strutils::strip(server);}
   void   setLocalDir(string dir)           { _local_dir    = stripPath(dir);}
   string getLocalDir()                     { return _local_dir;}
   void   setUseSvn(bool use_svn)           { _use_svn = use_svn;}
   void   setAllowedErrorCount(int errors)  {_allowed_errors = errors;}

   void   setCachePath(const SGPath& p)     {_persistentCachePath = p;}
   void   setCacheHits(unsigned int hits)   {_cache_hits = hits;}
   void   setUseBuiltin(bool built_in) { _use_built_in = built_in;}

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
private:
   virtual void run();
    
    // external model run and helpers
    void runExternal();
    void syncPathExternal(const WaitingSyncItem& next);
    bool runExternalSyncCommand(const char* dir);

    // internal mode run and helpers
    void runInternal();
    void updateSyncSlot(SyncSlot& slot);

    // commond helpers between both internal and external models
    
    bool isPathCached(const WaitingSyncItem& next) const;
    void initCompletedTilesPersistentCache();
    void writeCompletedTilesPersistentCache() const;
    void updated(const WaitingSyncItem& item, bool isNewDirectory);
    void fail(const WaitingSyncItem& failedItem);
    
    bool _use_built_in;
    HTTP::Client _http;
    SyncSlot _syncSlots[NUM_SYNC_SLOTS];

   volatile bool _is_dirty;
   volatile bool _stop;
   SGBlockingDeque <WaitingSyncItem> waitingTiles;
   CompletedTiles _completedTiles;
   SGBlockingDeque <WaitingSyncItem> _freshTiles;
   bool _use_svn;
   string _svn_server;
   string _svn_command;
   string _rsync_server;
   string _local_dir;
   SGPath _persistentCachePath;
};

SGTerraSync::SvnThread::SvnThread() :
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
    _use_built_in(true),
    _is_dirty(false),
    _stop(false),
    _use_svn(true)
{
}

void SGTerraSync::SvnThread::stop()
{
    // drop any pending requests
    waitingTiles.clear();

    if (!_running)
        return;

    // set stop flag and wake up the thread with an empty request
    _stop = true;
    WaitingSyncItem w(string(), WaitingSyncItem::Stop);
    request(w);
    join();
    _running = false;
}

bool SGTerraSync::SvnThread::start()
{
    if (_running)
        return false;

    if (_local_dir=="")
    {
        SG_LOG(SG_TERRAIN,SG_ALERT,
               "Cannot start scenery download. Local cache directory is undefined.");
        _fail_count++;
        _stalled = true;
        return false;
    }

    SGPath path(_local_dir);
    if (!path.exists())
    {
        SG_LOG(SG_TERRAIN,SG_ALERT,
               "Cannot start scenery download. Directory '" << _local_dir <<
               "' does not exist. Set correct directory path or create directory folder.");
        _fail_count++;
        _stalled = true;
        return false;
    }

    path.append("version");
    if (path.exists())
    {
        SG_LOG(SG_TERRAIN,SG_ALERT,
               "Cannot start scenery download. Directory '" << _local_dir <<
               "' contains the base package. Use a separate directory.");
        _fail_count++;
        _stalled = true;
        return false;
    }

    _use_svn |= _use_built_in;

    if ((_use_svn)&&(_svn_server==""))
    {
        SG_LOG(SG_TERRAIN,SG_ALERT,
               "Cannot start scenery download. Subversion scenery server is undefined.");
        _fail_count++;
        _stalled = true;
        return false;
    }
    if ((!_use_svn)&&(_rsync_server==""))
    {
        SG_LOG(SG_TERRAIN,SG_ALERT,
               "Cannot start scenery download. Rsync scenery server is undefined.");
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

    string status;

    if (_use_svn && _use_built_in)
        status = "Using built-in SVN support. ";
    else if (_use_svn)
    {
        status = "Using external SVN utility '";
        status += _svn_command;
        status += "'. ";
    }
    else
    {
        status = "Using RSYNC. ";
    }

    // not really an alert - but we want to (always) see this message, so user is
    // aware we're downloading scenery (and using bandwidth).
    SG_LOG(SG_TERRAIN,SG_ALERT,
           "Starting automatic scenery download/synchronization. "
           << status
           << "Directory: '" << _local_dir << "'.");

    SGThread::start();
    return true;
}

bool SGTerraSync::SvnThread::runExternalSyncCommand(const char* dir)
{
    ostringstream buf;
    SGPath localPath( _local_dir );
    localPath.append( dir );

    if (_use_svn)
    {
        buf << "\"" << _svn_command << "\" "
            << svn_options << " "
            << "\"" << _svn_server << "/" << dir << "\" "
            << "\"" << localPath.str_native() << "\"";
    } else {
        buf << rsync_cmd << " "
            << "\"" << _rsync_server << "/" << dir << "/\" "
            << "\"" << localPath.str_native() << "/\"";
    }

    string command;
#ifdef SG_WINDOWS
        // windows command line parsing is just lovely...
        // to allow white spaces, the system call needs this:
        // ""C:\Program Files\something.exe" somearg "some other arg""
        // Note: whitespace strings quoted by a pair of "" _and_ the 
        //       entire string needs to be wrapped by "" too.
        // The svn url needs forward slashes (/) as a path separator while
        // the local path needs windows-native backslash as a path separator.
    command = "\"" + buf.str() + "\"";
#else
    command = buf.str();
#endif
    SG_LOG(SG_TERRAIN,SG_DEBUG, "sync command '" << command << "'");

#ifdef SG_WINDOWS
    // tbd: does Windows support "popen"?
    int rc = system( command.c_str() );
#else
    FILE* pipe = popen( command.c_str(), "r");
    int rc=-1;
    // wait for external process to finish
    if (pipe)
        rc = pclose(pipe);
#endif

    if (rc)
    {
        SG_LOG(SG_TERRAIN,SG_ALERT,
               "Failed to synchronize directory '" << dir << "', " <<
               "error code= " << rc);
        return false;
    }
    return true;
}

void SGTerraSync::SvnThread::run()
{
    _active = true;
    initCompletedTilesPersistentCache();

    
    if (_use_built_in) {
        runInternal();
    } else {
        runExternal();
    }
    
    _active = false;
    _running = false;
    _is_dirty = true;
}

void SGTerraSync::SvnThread::runExternal()
{
    while (!_stop)
    {
        WaitingSyncItem next = waitingTiles.pop_front();
        if (_stop)
           break;

        if (isPathCached(next)) {
            _cache_hits++;
            SG_LOG(SG_TERRAIN, SG_DEBUG,
                   "Cache hit for: '" << next._dir << "'");
            continue;
        }
        
        syncPathExternal(next);

        if ((_allowed_errors >= 0)&&
            (_consecutive_errors >= _allowed_errors))
        {
            _stalled = true;
            _stop = true;
        }
    } // of thread running loop
}

void SGTerraSync::SvnThread::syncPathExternal(const WaitingSyncItem& next)
{
    _busy = true;
    SGPath path( _local_dir );
    path.append( next._dir );
    bool isNewDirectory = !path.exists();
    
    try {
        if (isNewDirectory) {
            int rc = path.create_dir( 0755 );
            if (rc) {
                SG_LOG(SG_TERRAIN,SG_ALERT,
                       "Cannot create directory '" << path << "', return code = " << rc );
                throw sg_exception("Cannot create directory for terrasync", path.str());
            }
        }
        
        if (!runExternalSyncCommand(next._dir.c_str())) {
            throw sg_exception("Running external sync command failed");
        }
    } catch (sg_exception& e) {
        fail(next);
        return;
    }
    
    updated(next, isNewDirectory);
}

void SGTerraSync::SvnThread::updateSyncSlot(SyncSlot &slot)
{
    if (slot.repository.get()) {
        if (slot.repository->isDoingSync()) {
            return; // easy, still working
        }
        
        // check result
        SVNRepository::ResultCode res = slot.repository->failure();
        if (res == SVNRepository::SVN_ERROR_NOT_FOUND) {
            // this is fine, but maybe we should use a different return code
            // in the future to higher layers can distinguish this case
        } else if (res != SVNRepository::SVN_NO_ERROR) {
            fail(slot.currentItem);
        } else {
            updated(slot.currentItem, slot.isNewDirectory);
            SG_LOG(SG_IO, SG_DEBUG, "sync of " << slot.repository->baseUrl() << " finished ("
                   << slot.stamp.elapsedMSec() << " msec");
        }

        // whatever happened, we're done with this repository instance
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
                SG_LOG(SG_TERRAIN,SG_ALERT,
                       "Cannot create directory '" << path << "', return code = " << rc );
                fail(slot.currentItem);
                return;
            }
        } // of creating directory step
        
        slot.repository.reset(new SVNRepository(path, &_http));
        slot.repository->setBaseUrl(_svn_server + "/" + slot.currentItem._dir);
        slot.repository->update();
        
        slot.stamp.stamp();
        SG_LOG(SG_IO, SG_DEBUG, "sync of " << slot.repository->baseUrl() << " started");
    }
}

void SGTerraSync::SvnThread::runInternal()
{
    while (!_stop) {
        _http.update(100);
        if (_stop)
            break;
 
        // drain the waiting tiles queue into the sync slot queues.
        while (!waitingTiles.empty()) {
            WaitingSyncItem next = waitingTiles.pop_front();
            if (isPathCached(next)) {
                _cache_hits++;
                SG_LOG(SG_TERRAIN, SG_DEBUG,
                       "Cache hit for: '" << next._dir << "'");
                continue;
            }

            unsigned int slot = syncSlotForType(next._type);
            _syncSlots[slot].queue.push(next);
        }
       
        // update each sync slot in turn
        for (unsigned int slot=0; slot < NUM_SYNC_SLOTS; ++slot) {
            updateSyncSlot(_syncSlots[slot]);
        }
    } // of thread running loop
}

bool SGTerraSync::SvnThread::isPathCached(const WaitingSyncItem& next) const
{
    CompletedTiles::const_iterator ii = _completedTiles.find( next._dir );
    if (ii == _completedTiles.end()) {
        return false;
    }
    
    // check if the path still physically exists
    SGPath p(_local_dir);
    p.append(next._dir);
    if (!p.exists()) {
        return false;
    }
    
    time_t now = time(0);
    return (ii->second > now);
}

void SGTerraSync::SvnThread::fail(const WaitingSyncItem& failedItem)
{
    time_t now = time(0);
    _consecutive_errors++;
    _fail_count++;
    _completedTiles[ failedItem._dir ] = now + UpdateInterval::FailedAttempt;
    _busy = false;
}

void SGTerraSync::SvnThread::updated(const WaitingSyncItem& item, bool isNewDirectory)
{
    time_t now = time(0);
    _consecutive_errors = 0;
    _success_count++;
    SG_LOG(SG_TERRAIN,SG_INFO,
           "Successfully synchronized directory '" << item._dir << "'");
    if (item._refreshScenery) {
        // updated a tile
        _updated_tile_count++;
        if (isNewDirectory)
        {
            // for now only report new directories to refresh display
            // (i.e. only when ocean needs to be replaced with actual data)
            _freshTiles.push_back(item);
            _is_dirty = true;
        }
    }
    
    _completedTiles[ item._dir ] = now + UpdateInterval::SuccessfulAttempt;
    writeCompletedTilesPersistentCache();
    _busy = false;
}

void SGTerraSync::SvnThread::initCompletedTilesPersistentCache()
{
    if (!_persistentCachePath.exists()) {
        return;
    }
    
    SGPropertyNode_ptr cacheRoot(new SGPropertyNode);
    time_t now = time(0);
    
    readProperties(_persistentCachePath.str(), cacheRoot);
    for (int i=0; i<cacheRoot->nChildren(); ++i) {
        SGPropertyNode* entry = cacheRoot->getChild(i);
        string tileName = entry->getStringValue("path");
        time_t stamp = entry->getIntValue("stamp");
        if (stamp < now) {
            continue;
        }
        
        _completedTiles[tileName] = stamp;
    }
}

void SGTerraSync::SvnThread::writeCompletedTilesPersistentCache() const
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
    CompletedTiles::const_iterator it = _completedTiles.begin();
    for (; it != _completedTiles.end(); ++it) {
        SGPropertyNode* entry = cacheRoot->addChild("entry");
        entry->setStringValue("path", it->first);
        entry->setIntValue("stamp", it->second);
    }
    
    writeProperties(f, cacheRoot, true /* write_all */);
    f.close();
}

///////////////////////////////////////////////////////////////////////////////
// SGTerraSync ////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SGTerraSync::SGTerraSync(SGPropertyNode_ptr root) :
    _svnThread(NULL),
    last_lat(NOWHERE),
    last_lon(NOWHERE),
    _terraRoot(root->getNode("/sim/terrasync",true)),
    _refreshCb(NULL),
    _userCbData(NULL)
{
    _svnThread = new SvnThread();
    _log = new BufferedLogCallback(SG_TERRAIN, SG_INFO);
    _log->truncateAt(255);
    
    sglog().addCallback(_log);
}

SGTerraSync::~SGTerraSync()
{
    _tiedProperties.Untie();
    delete _svnThread;
    _svnThread = NULL;
    sglog().removeCallback(_log);
    delete _log;
}

void SGTerraSync::init()
{
    _refreshDisplay = _terraRoot->getNode("refresh-display",true);
    _terraRoot->setBoolValue("built-in-svn-available",svn_built_in_available);
    reinit();
}

void SGTerraSync::reinit()
{
    // do not reinit when enabled and we're already up and running
    if ((_terraRoot->getBoolValue("enabled",false))&&
         (_svnThread->_active && _svnThread->_running))
        return;

    _svnThread->stop();

    if (_terraRoot->getBoolValue("enabled",false))
    {
        _svnThread->setSvnServer(_terraRoot->getStringValue("svn-server",""));
        _svnThread->setRsyncServer(_terraRoot->getStringValue("rsync-server",""));
        _svnThread->setLocalDir(_terraRoot->getStringValue("scenery-dir",""));
        _svnThread->setAllowedErrorCount(_terraRoot->getIntValue("max-errors",5));
        _svnThread->setUseBuiltin(_terraRoot->getBoolValue("use-built-in-svn",true));
        _svnThread->setCachePath(SGPath(_terraRoot->getStringValue("cache-path","")));
        _svnThread->setCacheHits(_terraRoot->getIntValue("cache-hit", 0));
        _svnThread->setUseSvn(_terraRoot->getBoolValue("use-svn",true));
        _svnThread->setExtSvnUtility(_terraRoot->getStringValue("ext-svn-utility","svn"));

        if (_svnThread->start())
        {
            syncAirportsModels();
            if (last_lat != NOWHERE && last_lon != NOWHERE)
            {
                // reschedule most recent position
                int lat = last_lat;
                int lon = last_lon;
                last_lat = NOWHERE;
                last_lon = NOWHERE;
                schedulePosition(lat, lon);
            }
        }
    }

    _stalledNode->setBoolValue(_svnThread->_stalled);
}

void SGTerraSync::bind()
{
    _tiedProperties.Tie( _terraRoot->getNode("busy", true), (bool*) &_svnThread->_busy );
    _tiedProperties.Tie( _terraRoot->getNode("active", true), (bool*) &_svnThread->_active );
    _tiedProperties.Tie( _terraRoot->getNode("update-count", true), (int*) &_svnThread->_success_count );
    _tiedProperties.Tie( _terraRoot->getNode("error-count", true), (int*) &_svnThread->_fail_count );
    _tiedProperties.Tie( _terraRoot->getNode("tile-count", true), (int*) &_svnThread->_updated_tile_count );
    _tiedProperties.Tie( _terraRoot->getNode("cache-hits", true), (int*) &_svnThread->_cache_hits );
    
    _terraRoot->getNode("busy", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("active", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("update-count", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("error-count", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("tile-count", true)->setAttribute(SGPropertyNode::WRITE,false);
    _terraRoot->getNode("use-built-in-svn", true)->setAttribute(SGPropertyNode::USERARCHIVE,false);
    _terraRoot->getNode("use-svn", true)->setAttribute(SGPropertyNode::USERARCHIVE,false);
    // stalled is used as a signal handler (to connect listeners triggering GUI pop-ups)
    _stalledNode = _terraRoot->getNode("stalled", true);
    _stalledNode->setBoolValue(_svnThread->_stalled);
    _stalledNode->setAttribute(SGPropertyNode::PRESERVE,true);
}

void SGTerraSync::unbind()
{
    _svnThread->stop();
    _tiedProperties.Untie();
}

void SGTerraSync::update(double)
{
    static SGBucket bucket;
    if (_svnThread->isDirty())
    {
        if (!_svnThread->_active)
        {
            if (_svnThread->_stalled)
            {
                SG_LOG(SG_TERRAIN,SG_ALERT,
                       "Automatic scenery download/synchronization stalled. Too many errors.");
            }
            else
            {
                // not really an alert - just always show this message
                SG_LOG(SG_TERRAIN,SG_ALERT,
                        "Automatic scenery download/synchronization has stopped.");
            }
            _stalledNode->setBoolValue(_svnThread->_stalled);
        }

        if (!_refreshDisplay->getBoolValue())
            return;

        while (_svnThread->hasNewTiles())
        {
            WaitingSyncItem next = _svnThread->getNewTile();
            if (next._refreshScenery)
            {
                refreshScenery(_svnThread->getLocalDir(),next._dir);
            }
        }
    }
}

void SGTerraSync::refreshScenery(SGPath path,const string& relativeDir)
{
    // find tiles to be refreshed
    if (_refreshCb)
    {
        path.append(relativeDir);
        if (path.exists())
        {
            simgear::Dir dir(path);
            //TODO need to be smarter here. only update tiles which actually
            // changed recently. May also be possible to use information from the
            // built-in SVN client directly (instead of checking directory contents).
            PathList tileList = dir.children(simgear::Dir::TYPE_FILE, ".stg");
            for (unsigned int i=0; i<tileList.size(); ++i)
            {
                // reload scenery tile
                long index = atoi(tileList[i].file().c_str());
                _refreshCb(_userCbData, index);
            }
        }
    }
}

bool SGTerraSync::isIdle() {return _svnThread->isIdle();}

void SGTerraSync::setTileRefreshCb(SGTerraSyncCallback refreshCb, void* userCbData)
{
    _refreshCb = refreshCb;
    _userCbData = userCbData;
}

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
            WaitingSyncItem w(dir.str(), WaitingSyncItem::AirportData);
            _svnThread->request( w );
        }
    }
    
    WaitingSyncItem w("Models", WaitingSyncItem::SharedModels);
    _svnThread->request( w );
}


void SGTerraSync::syncArea( int lat, int lon )
{
    if ( lat < -90 || lat > 90 || lon < -180 || lon > 180 )
        return;
    char NS, EW;
    int baselat, baselon;

    if ( lat < 0 ) {
        int base = (int)(lat / 10);
        if ( lat == base * 10 ) {
            baselat = base * 10;
        } else {
            baselat = (base - 1) * 10;
        }
        NS = 's';
    } else {
        baselat = (int)(lat / 10) * 10;
        NS = 'n';
    }
    if ( lon < 0 ) {
        int base = (int)(lon / 10);
        if ( lon == base * 10 ) {
            baselon = base * 10;
        } else {
            baselon = (base - 1) * 10;
        }
        EW = 'w';
    } else {
        baselon = (int)(lon / 10) * 10;
        EW = 'e';
    }

    const char* terrainobjects[3] = { "Terrain", "Objects",  0 };
    bool refresh=true;
    for (const char** tree = &terrainobjects[0]; *tree; tree++)
    {
        ostringstream dir;
        dir << *tree << "/" << setfill('0')
            << EW << setw(3) << abs(baselon) << NS << setw(2) << abs(baselat) << "/"
            << EW << setw(3) << abs(lon)     << NS << setw(2) << abs(lat);
        
        WaitingSyncItem w(dir.str(), WaitingSyncItem::Tile);
        if (refresh) {
            w.setRefresh();
        }
        
        _svnThread->request( w );
        refresh=false;
    }
}


void SGTerraSync::syncAreas( int lat, int lon, int lat_dir, int lon_dir )
{
    if ( lat_dir == 0 && lon_dir == 0 ) {
        // do surrounding 8 1x1 degree areas.
        for ( int i = lat - 1; i <= lat + 1; ++i ) {
            for ( int j = lon - 1; j <= lon + 1; ++j ) {
                if ( i != lat || j != lon ) {
                    syncArea( i, j );
                }
            }
        }
    } else {
        if ( lat_dir != 0 ) {
            syncArea( lat + lat_dir, lon - 1 );
            syncArea( lat + lat_dir, lon + 1 );
            syncArea( lat + lat_dir, lon );
        }
        if ( lon_dir != 0 ) {
            syncArea( lat - 1, lon + lon_dir );
            syncArea( lat + 1, lon + lon_dir );
            syncArea( lat, lon + lon_dir );
        }
    }

    // do current 1x1 degree area first
    syncArea( lat, lon );
}


bool SGTerraSync::schedulePosition(int lat, int lon)
{
    bool Ok = false;

    // Ignore messages where the location does not change
    if ( lat != last_lat || lon != last_lon )
    {
        if (_svnThread->_running)
        {
            SG_LOG(SG_TERRAIN,SG_DEBUG, "Requesting scenery update for position " <<
                                        lat << "," << lon);
            int lat_dir=0;
            int lon_dir=0;
            if ( last_lat != NOWHERE && last_lon != NOWHERE )
            {
                int dist = lat - last_lat;
                if ( dist != 0 )
                {
                    lat_dir = dist / abs(dist);
                }
                else
                {
                    lat_dir = 0;
                }
                dist = lon - last_lon;
                if ( dist != 0 )
                {
                    lon_dir = dist / abs(dist);
                } else
                {
                    lon_dir = 0;
                }
            }

            SG_LOG(SG_TERRAIN,SG_DEBUG, "Scenery update for " <<
                   "lat = " << lat << ", lon = " << lon <<
                   ", lat_dir = " << lat_dir << ",  " <<
                   "lon_dir = " << lon_dir);

            syncAreas( lat, lon, lat_dir, lon_dir );
            Ok = true;
        }
        last_lat = lat;
        last_lon = lon;
    }

    return Ok;
}

