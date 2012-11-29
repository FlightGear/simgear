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
#include <time.h>
#include <unistd.h>
#elif defined(_MSC_VER)
#   include <io.h>
#   ifndef HAVE_SVN_CLIENT_H
#       include <time.h>
#       include <process.h>
#   endif
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

#ifdef HAVE_SVN_CLIENT_H
#  ifdef HAVE_LIBSVN_CLIENT_1
#    include <svn_version.h>
#    include <svn_auth.h>
#    include <svn_client.h>
#    include <svn_cmdline.h>
#    include <svn_pools.h>
#  else
#    undef HAVE_SVN_CLIENT_H
#  endif
#endif

#ifdef HAVE_SVN_CLIENT_H
    static const svn_version_checklist_t mysvn_checklist[] = {
        { "svn_subr",   svn_subr_version },
        { "svn_client", svn_client_version },
        { NULL, NULL }
    };
    static const bool svn_built_in_available = true;
#else
    static const bool svn_built_in_available = false;
#endif

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
// WaitingTile ////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class  WaitingTile
{
public:
    WaitingTile(string dir,bool refresh) :
        _dir(dir), _refreshScenery(refresh) {}
    string _dir;
    bool _refreshScenery;
};

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
   void request(const WaitingTile& dir) {waitingTiles.push_front(dir);}
   bool isDirty() { bool r = _is_dirty;_is_dirty = false;return r;}
   bool hasNewTiles() { return !_freshTiles.empty();}
   WaitingTile getNewTile() { return _freshTiles.pop_front();}

   void   setSvnServer(string server)       { _svn_server   = stripPath(server);}
   void   setExtSvnUtility(string svn_util) { _svn_command  = simgear::strutils::strip(svn_util);}
   void   setRsyncServer(string server)     { _rsync_server = simgear::strutils::strip(server);}
   void   setLocalDir(string dir)           { _local_dir    = stripPath(dir);}
   string getLocalDir()                     { return _local_dir;}
   void   setUseSvn(bool use_svn)           { _use_svn = use_svn;}
   void   setAllowedErrorCount(int errors)  {_allowed_errors = errors;}

#ifdef HAVE_SVN_CLIENT_H
   void setUseBuiltin(bool built_in) { _use_built_in = built_in;}
#endif

   volatile bool _active;
   volatile bool _running;
   volatile bool _busy;
   volatile bool _stalled;
   volatile int  _fail_count;
   volatile int  _updated_tile_count;
   volatile int  _success_count;
   volatile int  _consecutive_errors;
   volatile int  _allowed_errors;

private:
   virtual void run();
   bool syncTree(const char* dir, bool& isNewDirectory);
   bool syncTreeExternal(const char* dir);

#ifdef HAVE_SVN_CLIENT_H
   static int svnClientSetup(void);
   bool syncTreeInternal(const char* dir);

   bool _use_built_in;

   // Things we need for doing subversion checkout - often
   static apr_pool_t *_svn_pool;
   static svn_client_ctx_t *_svn_ctx;
   static svn_opt_revision_t *_svn_rev;
   static svn_opt_revision_t *_svn_rev_peg;
#endif

   volatile bool _is_dirty;
   volatile bool _stop;
   SGBlockingDeque <WaitingTile> waitingTiles;
   CompletedTiles _completedTiles;
   SGBlockingDeque <WaitingTile> _freshTiles;
   bool _use_svn;
   string _svn_server;
   string _svn_command;
   string _rsync_server;
   string _local_dir;
};

#ifdef HAVE_SVN_CLIENT_H
    apr_pool_t* SGTerraSync::SvnThread::_svn_pool = NULL;
    svn_client_ctx_t* SGTerraSync::SvnThread::_svn_ctx = NULL;
    svn_opt_revision_t* SGTerraSync::SvnThread::_svn_rev = NULL;
    svn_opt_revision_t* SGTerraSync::SvnThread::_svn_rev_peg = NULL;
#endif

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
#ifdef HAVE_SVN_CLIENT_H
    _use_built_in(true),
#endif
    _is_dirty(false),
    _stop(false),
    _use_svn(true)
{
#ifdef HAVE_SVN_CLIENT_H
    int errCode = SGTerraSync::SvnThread::svnClientSetup();
    if (errCode != EXIT_SUCCESS)
    {
        SG_LOG(SG_TERRAIN,SG_ALERT,
               "Failed to initialize built-in SVN client, error = " << errCode);
    }
#endif
}

void SGTerraSync::SvnThread::stop()
{
    // drop any pending requests
    waitingTiles.clear();

    if (!_running)
        return;

    // set stop flag and wake up the thread with an empty request
    _stop = true;
    WaitingTile w("",false);
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

#ifdef HAVE_SVN_CLIENT_H
    _use_svn |= _use_built_in;
#endif

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
#ifdef HAVE_SVN_CLIENT_H
    if (_use_svn && _use_built_in)
        status = "Using built-in SVN support. ";
    else
#endif
    if (_use_svn)
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

// sync one directory tree
bool SGTerraSync::SvnThread::syncTree(const char* dir, bool& isNewDirectory)
{
    int rc;
    SGPath path( _local_dir );

    path.append( dir );
    isNewDirectory = !path.exists();
    if (isNewDirectory)
    {
        rc = path.create_dir( 0755 );
        if (rc)
        {
            SG_LOG(SG_TERRAIN,SG_ALERT,
                   "Cannot create directory '" << dir << "', return code = " << rc );
            return false;
        }
    }

#ifdef HAVE_SVN_CLIENT_H
    if (_use_built_in)
        return syncTreeInternal(dir);
    else
#endif
    {
        return syncTreeExternal(dir);
    }
}


#ifdef HAVE_SVN_CLIENT_H
bool SGTerraSync::SvnThread::syncTreeInternal(const char* dir)
{
    SG_LOG(SG_TERRAIN,SG_DEBUG, "Synchronizing scenery directory " << dir);
    if (!_svn_pool)
    {
        SG_LOG(SG_TERRAIN,SG_ALERT,
               "Built-in SVN client failed to initialize.");
        return false;
    }

    ostringstream command;
    command << _svn_server << "/" << dir;

    ostringstream dest_base_dir;
    dest_base_dir << _local_dir << "/" << dir;

    apr_pool_t *subpool = svn_pool_create(_svn_pool);

    svn_error_t *err = NULL;
#if (SVN_VER_MINOR >= 5)
    err = svn_client_checkout3(NULL,
            command.str().c_str(),
            dest_base_dir.str().c_str(),
            _svn_rev_peg,
            _svn_rev,
            svn_depth_infinity,
            0, // ignore-externals = false
            0, // allow unver obstructions = false
            _svn_ctx,
            subpool);
#else
    // version 1.4 API
    err = svn_client_checkout2(NULL,
            command.str().c_str(),
            dest_base_dir.str().c_str(),
            _svn_rev_peg,
            _svn_rev,
            1, // recurse=true - same as svn_depth_infinity for checkout3 above
            0, // ignore externals = false
            _svn_ctx,
            subpool);
#endif

    bool ReturnValue = true;
    if (err)
    {
        // Report errors from the checkout attempt
        if (err->apr_err == SVN_ERR_RA_ILLEGAL_URL)
        {
            // ignore errors when remote path doesn't exist (no scenery data for ocean areas)
        }
        else
        {
            SG_LOG(SG_TERRAIN,SG_ALERT,
                    "Failed to synchronize directory '" << dir << "', " <<
                    err->message << " (code " << err->apr_err << ").");
            svn_error_clear(err);
            // try to clean up
            err = svn_client_cleanup(dest_base_dir.str().c_str(),
                    _svn_ctx,subpool);
            if (!err)
            {
                SG_LOG(SG_TERRAIN,SG_ALERT,
                       "SVN repository cleanup successful for '" << dir << "'.");
            }
            ReturnValue = false;
        }
    } else
    {
        SG_LOG(SG_TERRAIN,SG_DEBUG, "Done with scenery directory " << dir);
    }
    svn_pool_destroy(subpool);
    return ReturnValue;
}
#endif

bool SGTerraSync::SvnThread::syncTreeExternal(const char* dir)
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
    while (!_stop)
    {
        WaitingTile next = waitingTiles.pop_front();
        if (_stop)
           break;

        CompletedTiles::iterator ii =
            _completedTiles.find( next._dir );
        time_t now = time(0);
        if ((ii == _completedTiles.end())||
            (ii->second < now ))
        {
            bool isNewDirectory = false;

            _busy = true;
            if (!syncTree(next._dir.c_str(),isNewDirectory))
            {
                _consecutive_errors++;
                _fail_count++;
                _completedTiles[ next._dir ] = now + UpdateInterval::FailedAttempt;
            }
            else
            {
                _consecutive_errors = 0;
                _success_count++;
                SG_LOG(SG_TERRAIN,SG_INFO,
                       "Successfully synchronized directory '" << next._dir << "'");
                if (next._refreshScenery)
                {
                    // updated a tile
                    _updated_tile_count++;
                    if (isNewDirectory)
                    {
                        // for now only report new directories to refresh display
                        // (i.e. only when ocean needs to be replaced with actual data)
                        _freshTiles.push_back(next);
                        _is_dirty = true;
                    }
                }
                _completedTiles[ next._dir ] = now + UpdateInterval::SuccessfulAttempt;
            }
            _busy = false;
        }

        if ((_allowed_errors >= 0)&&
            (_consecutive_errors >= _allowed_errors))
        {
            _stalled = true;
            _stop = true;
        }
    }

    _active = false;
    _running = false;
    _is_dirty = true;
}

#ifdef HAVE_SVN_CLIENT_H
// Configure our subversion session
int SGTerraSync::SvnThread::svnClientSetup(void)
{
    // Are we already prepared?
    if (_svn_pool) return EXIT_SUCCESS;
    // No, so initialize svn internals generally

#ifdef _MSC_VER
    // there is a segfault when providing an error stream.
    //  Apparently, calling setvbuf with a nul buffer is
    //  not supported under msvc 7.1 ( code inside svn_cmdline_init )
    if (svn_cmdline_init("terrasync", 0) != EXIT_SUCCESS)
        return EXIT_FAILURE;

    // revert locale setting
    setlocale(LC_ALL,"C");
#else
    /* svn_cmdline_init configures the locale. Setup environment to ensure the
     * default "C" locale remains active, since fgfs isn't locale aware - especially
     * requires "." as decimal point in strings containing floating point varibales. */
    setenv("LC_ALL", "C", 1);

    if (svn_cmdline_init("terrasync", stderr) != EXIT_SUCCESS)
        return EXIT_FAILURE;
#endif

    apr_pool_t *pool;
    apr_pool_create(&pool, NULL);
    svn_error_t *err = NULL;
    SVN_VERSION_DEFINE(_svn_version);
    err = svn_ver_check_list(&_svn_version, mysvn_checklist);
    if (err)
        return svn_cmdline_handle_exit_error(err, pool, "fgfs: ");
    err = svn_ra_initialize(pool);
    if (err)
        return svn_cmdline_handle_exit_error(err, pool, "fgfs: ");
    char *config_dir = NULL;
    err = svn_config_ensure(config_dir, pool);
    if (err)
        return svn_cmdline_handle_exit_error(err, pool, "fgfs: ");
    err = svn_client_create_context(&_svn_ctx, pool);
    if (err)
        return svn_cmdline_handle_exit_error(err, pool, "fgfs: ");
    err = svn_config_get_config(&(_svn_ctx->config),
        config_dir, pool);
    if (err)
        return svn_cmdline_handle_exit_error(err, pool, "fgfs: ");
    svn_config_t *cfg;
    cfg = ( svn_config_t*) apr_hash_get(
        _svn_ctx->config,
        SVN_CONFIG_CATEGORY_CONFIG,
        APR_HASH_KEY_STRING);
    if (err)
        return svn_cmdline_handle_exit_error(err, pool, "fgfs: ");

    svn_auth_baton_t *ab=NULL;

#if (SVN_VER_MINOR >= 6)
    err = svn_cmdline_create_auth_baton  (&ab,
            TRUE, NULL, NULL, config_dir, TRUE, FALSE, cfg,
            _svn_ctx->cancel_func, _svn_ctx->cancel_baton, pool);
#else
    err = svn_cmdline_setup_auth_baton(&ab,
        TRUE, NULL, NULL, config_dir, TRUE, cfg,
        _svn_ctx->cancel_func, _svn_ctx->cancel_baton, pool);
#endif

    if (err)
        return svn_cmdline_handle_exit_error(err, pool, "fgfs: ");
    
    _svn_ctx->auth_baton = ab;
#if (SVN_VER_MINOR >= 5)
    _svn_ctx->conflict_func = NULL;
    _svn_ctx->conflict_baton = NULL;
#endif

    // Now our magic revisions
    _svn_rev = (svn_opt_revision_t*) apr_palloc(pool, 
        sizeof(svn_opt_revision_t));
    if (!_svn_rev)
        return EXIT_FAILURE;
    _svn_rev_peg = (svn_opt_revision_t*) apr_palloc(pool, 
        sizeof(svn_opt_revision_t));
    if (!_svn_rev_peg)
        return EXIT_FAILURE;
    _svn_rev->kind = svn_opt_revision_head;
    _svn_rev_peg->kind = svn_opt_revision_unspecified;
    // Success if we got this far
    _svn_pool = pool;
    return EXIT_SUCCESS;
}
#endif

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
}

SGTerraSync::~SGTerraSync()
{
    _tiedProperties.Untie();
    delete _svnThread;
    _svnThread = NULL;
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

    #ifdef HAVE_SVN_CLIENT_H
        _svnThread->setUseBuiltin(_terraRoot->getBoolValue("use-built-in-svn",true));
    #else
        _terraRoot->setBoolValue("use-built-in-svn",false);
    #endif
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
            WaitingTile next = _svnThread->getNewTile();
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
            WaitingTile w(dir.str(),false);
            _svnThread->request( w );
        }
    }
    WaitingTile w("Models",false);
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
        WaitingTile w(dir.str(),refresh);
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
