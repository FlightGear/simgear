// terrasync.hxx -- scenery fetcher
//
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

#ifndef TERRASYNC_HXX_
#define TERRASYNC_HXX_

#include <set>

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>

class SGPath;
class SGBucket;

namespace simgear
{
const int NOWHERE = -9999;

class BufferedLogCallback;

class SGTerraSync : public SGSubsystem
{
public:

    SGTerraSync();
    virtual ~SGTerraSync();
    
    virtual void init();    
    virtual void shutdown();
    virtual void reinit();
    virtual void bind();
    virtual void unbind();
    virtual void update(double);

    /// notify terrasync that the sim was repositioned, as opposed to
    /// us travelling in a direction. Avoid last_lat / last_lon blocking
    /// certain tiles when we reposition.
    void reposition();
    
    bool isIdle();
    
    bool scheduleTile(const SGBucket& bucket);
    
    void syncAreaByPath(const std::string& aPath);
    
    void setRoot(SGPropertyNode_ptr root);

    /// retrive the associated log object, for displaying log
    /// output somewhere (a UI, presumably)
    BufferedLogCallback* log() const
        { return _log; }
    
    /**
     * Test if a scenery directory is queued or actively syncing.
     * File path is the tile name, eg 'e001n52' or 'w003n56'. Will return true
     * if either the Terrain or Objects variant is being synced.
     * 
     */
    bool isTileDirPending(const std::string& sceneryDir) const;
    
    
    void scheduleDataDir(const std::string& dataDir);
    
    bool isDataDirPending(const std::string& dataDir) const;
protected:
    void syncAirportsModels();
 
    
    class SvnThread;

private:
    SvnThread* _svnThread;
    SGPropertyNode_ptr _terraRoot;
    SGPropertyNode_ptr _stalledNode;
    SGPropertyNode_ptr _cacheHits;
    
    // we manually bind+init TerraSync during early startup
    // to get better overlap of slow operations (Shared Models sync
    // and nav-cache rebuild). As a result we need to track the bind/init
    // state explicitly to avoid duplicate calls.
    bool _bound, _inited;
    
    simgear::TiedPropertyList _tiedProperties;
    BufferedLogCallback* _log;
    
    typedef std::set<std::string> string_set;
    string_set _activeTileDirs;
};

}

#endif /* TERRASYNC_HXX_ */
