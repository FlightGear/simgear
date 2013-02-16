// SGPerfMon.hxx -- Performance Tracing
//
// Written by Thorsten Brehm, started November 2011.
//
// Copyright (C) 2011 Thorsten Brehm - brehmt (at) gmail com
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef __SGPERFMON_HXX
#define __SGPERFMON_HXX

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/timing/timestamp.hxx>

class SampleStatistic;

///////////////////////////////////////////////////////////////////////////////
// SG Performance Monitor  ////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class SGPerformanceMonitor : public SGSubsystem
{

public:
    SGPerformanceMonitor(SGSubsystemMgr* subSysMgr, SGPropertyNode_ptr root);

    virtual void bind   (void);
    virtual void unbind (void);
    virtual void init   (void);
    virtual void update (double dt);

private:
    static void subSystemMgrHook(void* userData, const std::string& name, SampleStatistic* timeStat);

    void reportTiming(const std::string& name, SampleStatistic* timeStat);

    SGTimeStamp _lastUpdate;
    SGSubsystemMgr* _subSysMgr;
    SGPropertyNode_ptr _root;
    SGPropertyNode_ptr _statiticsSubsystems;
    SGPropertyNode_ptr _statisticsFlag;
    SGPropertyNode_ptr _statisticsInterval;

    bool _isEnabled;
    int _count;
};

#endif // __SGPERFMON_HXX
