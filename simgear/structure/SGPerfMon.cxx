// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2011 Thorsten Brehm <brehmt@gmail.com>

/**
 * @file
 * @brief Performance Monitoring
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGPerfMon.hxx"
#include <simgear/structure/SGSmplstat.hxx>

#include <stdio.h>
#include <string.h>
#include <string>

#include <simgear/constants.h>
#include <simgear/sg_inlines.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/structure/exception.hxx>

using std::string;

SGPerformanceMonitor::SGPerformanceMonitor(SGSubsystemMgr* subSysMgr, SGPropertyNode_ptr root) :
    _isEnabled(false),
    _count(0)
{
    _root = root;
    _subSysMgr = subSysMgr;
}

void
SGPerformanceMonitor::bind(void)
{
    _statiticsSubsystems = _root->getChild("subsystems",    0, true);
    _statisticsFlag      = _root->getChild("enabled",       0, true);
    _timingDetailsFlag   = _root->getChild("dump-stats",    0, true);
    _timingDetailsFlag->setBoolValue(false);
    _statisticsInterval  = _root->getChild("interval-s",    0, true);
    _maxTimePerFrame_ms = _root->getChild("max-time-per-frame-ms", 0, true);
}

void
SGPerformanceMonitor::unbind(void)
{
    _statiticsSubsystems = 0;
    _statisticsFlag = 0;
    _statisticsInterval = 0;
    _maxTimePerFrame_ms = 0;
}

void
SGPerformanceMonitor::init(void)
{

}

void
SGPerformanceMonitor::update(double dt)
{
    if (_isEnabled != _statisticsFlag->getBoolValue())
    {
        // flag has changed, update subsystem manager
        _isEnabled = _statisticsFlag->getBoolValue();
        if (_isEnabled)
        {
            _subSysMgr->setReportTimingCb(this,&subSystemMgrHook);
            _lastUpdate.stamp();
        }
        else
            _subSysMgr->setReportTimingCb(this,0);
    }
    if (_timingDetailsFlag->getBoolValue()) {
        _subSysMgr->setReportTimingStats(true);
        _timingDetailsFlag->setBoolValue(false);
    }
    if (!_isEnabled)
        return;

    if (_lastUpdate.elapsedMSec() > 1000 * _statisticsInterval->getDoubleValue())
    {
        _count = 0;
        // grab timing statistics
        _subSysMgr->reportTiming();
        _lastUpdate.stamp();
    }
    if (_maxTimePerFrame_ms) {
        SGSubsystem::maxTimePerFrame_ms = _maxTimePerFrame_ms->getIntValue();
    }
}

/** Callback hooked into the subsystem manager. */
void
SGPerformanceMonitor::subSystemMgrHook(void* userData, const std::string& name, SampleStatistic* timeStat)
{
    ((SGPerformanceMonitor*) userData)->reportTiming(name, timeStat);
}

/** Grabs and exposes timing information to properties */
void
SGPerformanceMonitor::reportTiming(const string& name, SampleStatistic* timeStat)
{
    SGPropertyNode* node = _statiticsSubsystems->getChild("subsystem",_count++,true);

    double minMs        = timeStat->min()    / 1000;
    double maxMs        = timeStat->max()    / 1000;
    double meanMs       = timeStat->mean()   / 1000;
    double stdDevMs     = timeStat->stdDev() / 1000;
    double totalMs      = timeStat->total()  / 1000;
    double cumulativeMs = timeStat->cumulative() / 1000;
    int    samples      = timeStat->samples();

    node->setStringValue("name", name);
    node->setDoubleValue("min-ms", minMs);
    node->setDoubleValue("max-ms", maxMs);
    node->setDoubleValue("mean-ms", meanMs);
    node->setDoubleValue("stddev-ms", stdDevMs);
    node->setDoubleValue("total-ms", totalMs);
    node->setDoubleValue("cumulative-ms", cumulativeMs);
    node->setDoubleValue("count",samples);

    timeStat->reset();
}


// Register the subsystem.
//SGSubsystemMgr::Registrant<SGPerformanceMonitor> registrantSGPerformanceMonitor;
