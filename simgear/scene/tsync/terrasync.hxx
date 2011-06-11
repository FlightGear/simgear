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

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <OpenThreads/Thread>

class SGPath;

namespace simgear
{
const int NOWHERE = -9999;

class TileCache;

class SGTerraSync : public SGSubsystem
{
public:

    SGTerraSync(SGPropertyNode_ptr root);
    virtual ~SGTerraSync();
    virtual void init();
    virtual void reinit();
    virtual void bind();
    virtual void unbind();
    virtual void update(double);

    bool isIdle();
    bool schedulePosition(int lat, int lon);
    void setTileCache(TileCache* tile_cache);

protected:
    void syncAirportsModels();
    void syncArea(int lat, int lon);
    void syncAreas(int lat, int lon, int lat_dir, int lon_dir);
    void refreshScenery(SGPath path,const string& relativeDir);

    class SvnThread;

private:
    SvnThread* _svnThread;
    int last_lat;
    int last_lon;
    SGPropertyNode_ptr _terraRoot;
    SGPropertyNode_ptr _refresh_display;
    SGPropertyNode_ptr _stalled_node;
    TileCache* _tile_cache;
    simgear::TiedPropertyList _tiedProperties;
};

}

#endif /* TERRASYNC_HXX_ */
