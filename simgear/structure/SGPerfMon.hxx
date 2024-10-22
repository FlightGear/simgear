// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2011 Thorsten Brehm <brehmt@gmail.com>

/**
 * @file
 * @brief Performance Monitoring
 */

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

    // Subsystem API.
    void bind() override;
    void init() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "performance-mon"; }

private:
    static void subSystemMgrHook(void* userData, const std::string& name, SampleStatistic* timeStat);

    void reportTiming(const std::string& name, SampleStatistic* timeStat);

    SGTimeStamp _lastUpdate;
    SGSubsystemMgr* _subSysMgr;
    SGPropertyNode_ptr _root;
    SGPropertyNode_ptr _statiticsSubsystems;
    SGPropertyNode_ptr _timingDetailsFlag;
    SGPropertyNode_ptr _statisticsFlag;
    SGPropertyNode_ptr _statisticsInterval;
    SGPropertyNode_ptr _maxTimePerFrame_ms;

    bool _isEnabled;
    int _count;
};

#endif // __SGPERFMON_HXX
