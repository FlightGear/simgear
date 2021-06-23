// Written by David Megginson, started 2000-12
//
// Copyright (C) 2000  David Megginson, david@megginson.com
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

#include <algorithm>
#include <cassert>

#include <simgear/debug/logstream.hxx>
#include <simgear/timing/timestamp.hxx>

#include "exception.hxx"
#include "subsystem_mgr.hxx"
#include "commands.hxx"

#include "SGSmplstat.hxx"
#include <simgear/debug/ErrorReportingCallback.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/props/props.hxx>

const int SG_MAX_SUBSYSTEM_EXCEPTIONS = 4;
const char SUBSYSTEM_NAME_SEPARATOR = '.';

using std::string;
using State = SGSubsystem::State;

////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystem
////////////////////////////////////////////////////////////////////////

SGSubsystemTimingCb SGSubsystem::reportTimingCb = NULL;
void* SGSubsystem::reportTimingUserData = NULL;
bool SGSubsystem::reportTimingStatsRequest = false;
int SGSubsystem::maxTimePerFrame_ms = 7;

SGSubsystem::SGSubsystem () : _executionTime(0), _lastExecutionTime(0)
{
}

SGSubsystem::~SGSubsystem ()
{
}

void
SGSubsystem::init ()
{
}

SGSubsystem::InitStatus
SGSubsystem::incrementalInit ()
{
  init();
  return INIT_DONE;
}

void
SGSubsystem::postinit ()
{
}

void
SGSubsystem::reinit ()
{
}

void
SGSubsystem::shutdown ()
{
}

void
SGSubsystem::bind ()
{
}

void
SGSubsystem::unbind ()
{
}

void
SGSubsystem::suspend ()
{
    suspend(true);
}

void
SGSubsystem::suspend (bool suspended)
{
    // important we don't use is_suspended() here since SGSubsystemGroup
    // overries. We record the actual state correctly here, even for groups,
    // so this works out, whereas is_suspended() is always false for groups
    if (_suspended == suspended)
        return;
    
    const auto newState = suspended ? State::SUSPEND : State::RESUME;
    const auto manager = get_manager();
    
    if (manager)
        manager->notifyDelegatesWillChange(this, newState);
    _suspended = suspended;
    if (manager)
        manager->notifyDelegatesDidChange(this, newState);
}

void
SGSubsystem::resume ()
{
    suspend(false);
}

bool
SGSubsystem::is_suspended () const
{
  return _suspended;
}

void SGSubsystem::stamp(const string& name)
{
    timingInfo.push_back(TimingInfo(name, SGTimeStamp::now()));
}

void SGSubsystem::set_name(const std::string &n)
{
    assert(_subsystemId.empty());
    _subsystemId = n;
}

std::string SGSubsystem::subsystemClassId() const
{
    auto pos = _subsystemId.find(SUBSYSTEM_NAME_SEPARATOR);
    if (pos == std::string::npos) {
        return _subsystemId;
    }
    
    return _subsystemId.substr(0, pos);
}

std::string SGSubsystem::subsystemInstanceId() const
{
    auto pos = _subsystemId.find(SUBSYSTEM_NAME_SEPARATOR);
    if (pos == std::string::npos) {
        return {};
    }
    
    return _subsystemId.substr(pos+1);
}

void SGSubsystem::set_group(SGSubsystemGroup* group)
{
    _group = group;
}

SGSubsystemGroup* SGSubsystem::get_group() const
{
    return _group;
}

SGSubsystemMgr* SGSubsystem::get_manager() const
{
    if (!get_group())
        throw sg_exception("SGSubsystem::get_manager: subsystem " + subsystemId() + " has no group");
    return get_group()->get_manager();
}

std::string SGSubsystem::nameForState(State s)
{
    switch (s) {
    case State::INVALID: return "invalid";
    case State::INIT: return "init";
    case State::REINIT: return "reinit";
    case State::POSTINIT: return "post-init";
    case State::SHUTDOWN: return "shutdown";
    case State::BIND: return "bind";
    case State::UNBIND: return "unbind";
    case State::ADD: return "add";
    case State::REMOVE: return "remove";
    case State::SUSPEND: return "suspend";
    case State::RESUME: return "resume";
    }

	return {};
}

////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystemGroup.
////////////////////////////////////////////////////////////////////////

class SGSubsystemGroup::Member
{    
private:
    Member(const Member &member);
public:
    Member();
    ~Member ();
    
    void update (double delta_time_sec);

    void reportTiming(void) { if (reportTimingCb) reportTimingCb(reportTimingUserData, name, &timeStat); }
    void reportTimingStats(TimerStats *_lastValues) {
        if (subsystem)
            subsystem->reportTimingStats(_lastValues);
    }

    void updateExecutionTime(double time) { timeStat += time;}
    SampleStatistic timeStat;
    std::string name;
    SGSubsystemRef subsystem;
    double min_step_sec;
    double elapsed_sec;
    bool collectTimeStats;
    int exceptionCount;
    int initTime;

    void mergeTimerStats(SGSubsystem::TimerStats &stats);
};



SGSubsystemGroup::SGSubsystemGroup() :
    _fixedUpdateTime(-1.0),
    _updateTimeRemainder(0.0),
    _initPosition(-1)
{
}

SGSubsystemGroup::~SGSubsystemGroup ()
{
    clearSubsystems();
}

void
SGSubsystemGroup::init ()
{
    assert(_state == State::BIND);
    forEach([this](SGSubsystem* s) {
        try {
            this->notifyWillChange(s, State::INIT);
            s->init();
            this->notifyDidChange(s, State::INIT);
        } catch (std::exception& e) {
            simgear::reportError("Caught exception init-ing subsystem " + s->subsystemId() + "\n\t" + e.what());
            throw;
        }
    });
    _state = State::INIT;
}

SGSubsystem::InitStatus
SGSubsystemGroup::incrementalInit()
{
    // special case this, simplifies the logic below
    if (_members.empty()) {
        _state = State::INIT;
        return INIT_DONE;
    }

    // termination test
    if (_initPosition >= static_cast<int>(_members.size())) {
        _state = State::INIT;
        return INIT_DONE;
    }
  
    if (_initPosition < 0) {
        // first call
        assert(_state == State::BIND);
        _initPosition = 0;
        notifyWillChange(_members.front()->subsystem, State::INIT);
    }
    
    const auto m = _members[_initPosition]; 
    SGTimeStamp st;
    st.stamp();
    InitStatus memberStatus;

    try {
        memberStatus = m->subsystem->incrementalInit();
    } catch (std::exception& e) {
        simgear::reportError("Caught exception init-ing subsystem " + m->subsystem->subsystemId() + "\n\t" + e.what());
        throw;
    }

    m->initTime += st.elapsedMSec();
  
    if (memberStatus == INIT_DONE) {
        // complete init of this one
        notifyDidChange(m->subsystem, State::INIT);
        ++_initPosition;
        
        if (_initPosition < static_cast<int>(_members.size())) {
            // start init of the next one
            notifyWillChange( _members[_initPosition]->subsystem, State::INIT);
        }
    }
  
  return INIT_CONTINUE;
}

void SGSubsystemGroup::forEach(std::function<void(SGSubsystem*)> f)
{
    for (auto m : _members) {
        f(m->subsystem);
    }
}

void SGSubsystemGroup::reverseForEach(std::function<void(SGSubsystem*)> f)
{
    for (auto it = _members.rbegin(); it != _members.rend(); ++it) {
        f((*it)->subsystem);
    }
}

void
SGSubsystemGroup::postinit ()
{
    assert(_state == State::INIT);
    forEach([this](SGSubsystem* s){
        this->notifyWillChange(s, State::POSTINIT);
        s->postinit();
        this->notifyDidChange(s, State::POSTINIT);
    });
    _state = State::POSTINIT;
}

void
SGSubsystemGroup::reinit ()
{
    forEach([this](SGSubsystem* s){
        this->notifyWillChange(s, State::REINIT);
        s->reinit();
        this->notifyDidChange(s, State::REINIT);
    });
}

void
SGSubsystemGroup::shutdown ()
{
    reverseForEach([this](SGSubsystem* s){
        this->notifyWillChange(s, State::SHUTDOWN);
        s->shutdown();
        this->notifyDidChange(s, State::SHUTDOWN);
    });
    _state = State::SHUTDOWN;
    _initPosition = -1;
}

void
SGSubsystemGroup::bind ()
{
    forEach([this](SGSubsystem* s) {
        try {
            this->notifyWillChange(s, State::BIND);
            s->bind();
            this->notifyDidChange(s, State::BIND);
        } catch (std::exception& e) {
            simgear::reportError("Caught exception binding subsystem " + s->subsystemId() + "\n\t" + e.what());
            throw;
        }
    });
    _state = State::BIND;
}

void
SGSubsystemGroup::unbind ()
{
    reverseForEach([this](SGSubsystem* s){
        this->notifyWillChange(s, State::UNBIND);
        s->unbind();
        this->notifyDidChange(s, State::UNBIND);
    });
    _state = State::UNBIND;
}

void
SGSubsystemGroup::update (double delta_time_sec)
{
    int loopCount = 1;
    // if dt == 0.0, we are paused, so we need to run one iteration
    // of our members; if we have a fixed update time, we compute a
    // loop count, and locally adjust dt
    if ((delta_time_sec > 0.0) && (_fixedUpdateTime > 0.0)) {
      double localDelta = delta_time_sec + _updateTimeRemainder;
      loopCount = SGMiscd::roundToInt(localDelta / _fixedUpdateTime);
      loopCount = std::max(0, loopCount);
      _updateTimeRemainder = delta_time_sec - (loopCount * _fixedUpdateTime);
      delta_time_sec = _fixedUpdateTime;
    }

    const bool recordTime = (reportTimingCb != nullptr);
    SGTimeStamp timeStamp;
    TimerStats lvTimerStats(_timerStats);
    TimerStats overrunItems;
    bool overrun = false;

    SGTimeStamp outerTimeStamp;
    outerTimeStamp.stamp();
    while (loopCount-- > 0) {
        for (auto member : _members) {

          timeStamp.stamp();
          if (member->subsystem->_timerStats.size()) {
              member->subsystem->_lastTimerStats.clear();
              member->subsystem->_lastTimerStats.insert(member->subsystem->_timerStats.begin(), member->subsystem->_timerStats.end());
          }
          member->update(delta_time_sec); // indirect call
          if (member->name.size())
              _timerStats[member->name] += timeStamp.elapsedMSec() / 1000.0;

          if (recordTime && reportTimingCb) {
              member->updateExecutionTime(timeStamp.elapsedMSec()*1000);
              if (timeStamp.elapsedMSec() > SGSubsystemMgr::maxTimePerFrame_ms) {
                  overrunItems[member->name] += timeStamp.elapsedMSec();
                  overrun = true;
              }
          }
      }
    } // of multiple update loop
    _lastExecutionTime = _executionTime;
    _executionTime += outerTimeStamp.elapsedMSec();
    if (overrun) {
        for (auto overrunItem : overrunItems) {
            SG_LOG(SG_EVENT, SG_ALERT, "Subsystem "
                << overrunItem.first
                << " total "
                << std::setw(6) << std::fixed << std::setprecision(2) << std::right << _timerStats[overrunItem.first]
                << "s overrun "
                << std::setw(6) << std::fixed << std::setprecision(2) << std::right << overrunItem.second
                << "ms");
            auto m = std::find_if(_members.begin(), _members.end(), [overrunItem](const Member* m) {
                if (m->name == overrunItem.first)
                    return true;
                return false;
            });
            if (m != _members.end()) {
                auto member = *m;
                if (overrunItems[member->name]) {
                    TimerStats sst;
                    member->reportTimingStats(&_lastTimerStats);
                    //if (lvTimerStats[member->name] != _timerStats[member->name]) {
                    //    SG_LOG(SG_EVENT, SG_ALERT,
                    //        " +" << std::setw(6) << std::left << (_timerStats[member->name] - lvTimerStats[member->name])
                    //        << " total " << std::setw(6) << std::left << _timerStats[member->name]
                    //        << " " << member->name
                    //    );
                    //}
                }
            }
        }
    }

    if (reportTimingStatsRequest) {
        reportTimingStats(nullptr);
        //for (auto member : _members) {
        //    member->mergeTimerStats(_timerStats);
        //    if (_timerStats.size()) {
        //        SG_LOG(SG_EVENT, SG_ALERT, "" << std::setw(6) << std::fixed << std::setprecision(2) << std::left << _timerStats[member->name]
        //            << ": " << member->name);
        //        for (auto item : _timerStats) {
        //            if (item.second > 0)
        //                SG_LOG(SG_EVENT, SG_ALERT, "  " << std::setw(6) << std::left << item.second << " " << item.first);
        //        }
        //    }
        //}
    }
    _lastTimerStats.clear();
    _lastTimerStats.insert(_timerStats.begin(), _timerStats.end());

}
void SGSubsystem::reportTimingStats(TimerStats *__lastValues) {
    std::string _name = "";

    bool reportDeltas = __lastValues != nullptr;
    __lastValues = &_lastTimerStats;
    std::ostringstream t;
    if (reportDeltas) {
        auto deltaT = _executionTime - _lastExecutionTime;
        if (deltaT != 0) {
            t << subsystemInstanceId() << "(+" << std::setprecision(2) << std::right << deltaT << "ms).";
            _name = t.str();
        }
    }
    else {
        SG_LOG(SG_EVENT, SG_ALERT, "SubSystem: " << _name << " " << std::setw(6) << std::setprecision(4) << std::right << _executionTime / 1000.0 << "s");
    }
    for (auto item : _timerStats) {
        std::ostringstream output;
        if (item.second > 0) {
            if (reportDeltas)
            {
                auto delta = item.second - (*__lastValues)[item.first];
                if (delta != 0) {
                    output 
                        << "  +" << std::setw(6) << std::setprecision(4) << std::left << (delta * 1000.0)
                        << _name << item.first
                        << "  total " << std::setw(6) << std::setprecision(4) << std::right << item.second << "s " 
                        ;
                }
            }
            else
                output << "  " << std::setw(6) << std::setprecision(4) << std::right << item.second << "s " << item.first;
            if (output.str().size())
                SG_LOG(SG_EVENT, SG_ALERT, output.str());
        }
    }
}

void SGSubsystemGroup::reportTimingStats(TimerStats *_lastValues) {
    SGSubsystem::reportTimingStats(_lastValues);

    std::string _name = subsystemInstanceId();
    if (!_name.size())
        _name = typeid(this).name();
    if (_lastValues) {
        auto deltaT = _executionTime - _lastExecutionTime;
        if (deltaT != 0) {
            SG_LOG(SG_EVENT, SG_ALERT, 
                " +" << std::setw(6) << std::setprecision(4) << std::right << deltaT << "ms "
                << subsystemInstanceId() );
        }
    }
    else
        SG_LOG(SG_EVENT, SG_ALERT, "SubSystemGroup: " << subsystemInstanceId() << " " << std::setw(6) << std::setprecision(4) << std::right << _executionTime / 1000.0 << "s");
    for (auto member : _members) {
        member->reportTimingStats(_lastValues);
    }
    _lastTimerStats.clear();
    _lastTimerStats.insert(_timerStats.begin(), _timerStats.end());

}
void
SGSubsystemGroup::reportTiming(void)
{
    for (auto member : _members) {
        member->reportTiming();
    }
}

void
SGSubsystemGroup::suspend ()
{
    SGSubsystem::suspend();
    forEach([](SGSubsystem* s) { s->suspend(); });
}

void
SGSubsystemGroup::resume ()
{
    forEach([](SGSubsystem* s) { s->resume(); });
    SGSubsystem::resume();
}

string_list
SGSubsystemGroup::member_names() const
{
	string_list result;
    for (auto member : _members) {
        result.push_back(member->name);
    }

	return result;
}

bool
SGSubsystemGroup::is_suspended () const
{
    // important so suspended groups still count dt for their members
    // but this does mean we need to be careful when notifying suspend
    // and resume on groups
    return false;
}

void
SGSubsystemGroup::set_subsystem (const string &name, SGSubsystem * subsystem,
                                 double min_step_sec)
{
    if (name.empty()) {
        SG_LOG(SG_GENERAL, SG_DEV_WARN, "adding subsystem to group without a name");
        // TODO, make this an exception in the future
    } else if (subsystem->subsystemId().empty()) {
        subsystem->set_name(name);
    } else if (name != subsystem->subsystemId()) {
        SG_LOG(SG_GENERAL, SG_DEV_WARN, "adding subsystem to group with name '" << name
               << "', but subsystemId() returns '" << subsystem->subsystemId() << "'");
    }
    
    notifyWillChange(subsystem, State::ADD);
    Member* member = get_member(name, true);
    member->name = name;
    member->subsystem = subsystem;
    member->min_step_sec = min_step_sec;
    subsystem->set_group(this);
    notifyDidChange(subsystem, State::ADD);
    
    if (_state != State::INVALID && (_state <= State::POSTINIT)) {
        // transition to the correct state
        if (_state >= State::BIND) {
            notifyWillChange(subsystem, State::BIND);
            subsystem->bind();
            notifyDidChange(subsystem, State::BIND);
        }
        
        if (_state >= State::INIT) {
            notifyWillChange(subsystem, State::INIT);
            subsystem->init();
            notifyDidChange(subsystem, State::INIT);
        }
        
        if (_state == State::POSTINIT) {
            notifyWillChange(subsystem, State::POSTINIT);
            subsystem->postinit();
            notifyDidChange(subsystem, State::POSTINIT);
        }
    }
}

void
SGSubsystemGroup::set_subsystem (SGSubsystem * subsystem, double min_step_sec)
{
    set_subsystem(subsystem->subsystemId(), subsystem, min_step_sec);
}

SGSubsystem *
SGSubsystemGroup::get_subsystem (const string &name)
{
    if (has_subsystem(name)) {
        const Member* member = get_member(name, false);
        return member->subsystem;
    }
    
    // recursive search
    for (const auto& m : _members) {
        if (m->subsystem->is_group()) {
            auto s = static_cast<SGSubsystemGroup*>(m->subsystem.ptr())->get_subsystem(name);
            if (s)
                return s;
        }
    }
    
    return {};
}

bool
SGSubsystemGroup::remove_subsystem(const string &name)
{
    // direct membership
    auto it = std::find_if(_members.begin(), _members.end(), [name](const Member* m)
                           { return m->name == name; });
    if (it != _members.end()) {
        // found it, great
        const auto sub = (*it)->subsystem;
        
        // ensure we clear the name during shutdown. This ensures that
        // a lookup during shutdown fails, which is the desired behaviour.
        // for an example see: https://sourceforge.net/p/flightgear/codetickets/2217/
        // where we looked up Nasal during Nasal shutdown
        // without this, the SubsystemMgr re-caches a dead pointer
        // after doing its clear()
        (*it)->name.clear();
        
        if (_state != State::INVALID) {
            // transition out correctly
            if ((_state >= State::INIT) && (_state < State::SHUTDOWN)) {
                notifyWillChange(sub, State::SHUTDOWN);
                sub->shutdown();
                notifyDidChange(sub, State::SHUTDOWN);
            }
            
            if ((_state >= State::BIND) && (_state < State::UNBIND)) {
                notifyWillChange(sub, State::UNBIND);
                sub->unbind();
                notifyDidChange(sub, State::UNBIND);
            }
        }
        
        notifyWillChange(sub, State::REMOVE);
        delete *it;
        _members.erase(it);
        notifyDidChange(sub, State::REMOVE);
        return true;
    }
    
    // recursive removal
    for (const auto& m : _members) {
        if (m->subsystem->is_group()) {
            auto group = static_cast<SGSubsystemGroup*>(m->subsystem.ptr());
            bool ok = group->remove_subsystem(name);
            if (ok) {
                return true;
            }
        }
    }
    
    return false;
}

void
SGSubsystemGroup::notifyWillChange(SGSubsystem* sub, SGSubsystemMgr::State s)
{
    auto manager = get_manager();
    if (manager) {
        manager->notifyDelegatesWillChange(sub, s);
    }
}

void
SGSubsystemGroup::notifyDidChange(SGSubsystem* sub, SGSubsystemMgr::State s)
{
    auto manager = get_manager();
    if (manager) {
        manager->notifyDelegatesDidChange(sub, s);
    }
}

//------------------------------------------------------------------------------
void SGSubsystemGroup::clearSubsystems()
{
    // reverse order to prevent order dependency problems
    for (auto it = _members.rbegin(); it != _members.rend(); ++it)
    {
        // hold a ref here in case (as is very likely) this is the
        // final ref to the subsystem, so that we can notify-did-change safely
        SGSubsystemRef sub = (*it)->subsystem;
        notifyWillChange(sub, State::REMOVE);
        delete *it;
        notifyDidChange(sub, State::REMOVE);
    }
    
    _members.clear();
}

void
SGSubsystemGroup::set_fixed_update_time(double dt)
{
  _fixedUpdateTime = dt;
}

bool
SGSubsystemGroup::has_subsystem (const string &name) const
{
    auto it = std::find_if(_members.begin(), _members.end(), [name](const Member* m)
                           { return m->name == name; });
    return it != _members.end();
}

auto SGSubsystemGroup::get_member(const string &name, bool create) -> Member*
{
    auto it = std::find_if(_members.begin(), _members.end(), [name](const Member* m)
                           { return m->name == name; });
    if (it == _members.end()) {
        if (!create)
            return nullptr;

        Member* m = new Member;
        m->name = name;
        _timerStats[name] = 0;
        _members.push_back(m);
        return _members.back();
    }
    
    return *it;
}

SGSubsystemMgr* SGSubsystemGroup::get_manager() const
{
    auto parentGroup = get_group();
    if (parentGroup) {
        return parentGroup->get_manager();
    }
    
    return _manager;
}

void SGSubsystemGroup::set_manager(SGSubsystemMgr *manager)
{
    _manager = manager;
}

////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystemGroup::Member
////////////////////////////////////////////////////////////////////////

SGSubsystemGroup::Member::Member ()
    : name(""),
      subsystem(0),
      min_step_sec(0),
      elapsed_sec(0),
      exceptionCount(0),
      initTime(0)
{
}

// This shouldn't be called due to subsystem pointer ownership issues.
SGSubsystemGroup::Member::Member (const Member &)
{
}

SGSubsystemGroup::Member::~Member ()
{
}
void SGSubsystemGroup::Member::mergeTimerStats(SGSubsystem::TimerStats &stats) {
    stats.insert(subsystem->_timerStats.begin(), subsystem->_timerStats.end());
    //for (auto ts : subsystem->_timerStats)
    //    ts.second = 0;
}

void
SGSubsystemGroup::Member::update (double delta_time_sec)
{
    elapsed_sec += delta_time_sec;
    if (elapsed_sec < min_step_sec) {
        return;
    }
    
    if (subsystem->is_suspended()) {
        return;
    }
    
    SGTimeStamp oTimer;
    try {
        oTimer.stamp();
        subsystem->update(elapsed_sec);
        subsystem->_lastExecutionTime = subsystem->_executionTime;
        subsystem->_executionTime += oTimer.elapsedMSec();
        elapsed_sec = 0;
    }
    catch (sg_exception& e) {
      SG_LOG(SG_GENERAL, SG_ALERT, "caught exception processing subsystem:" << name
        << "\nmessage:" << e.getMessage());
      
      if (++exceptionCount > SG_MAX_SUBSYSTEM_EXCEPTIONS) {
        SG_LOG(SG_GENERAL, SG_ALERT, "(exceptionCount=" << exceptionCount <<
          ", suspending)");
        simgear::reportError("suspending subsystem after too many errors:" + name);
        subsystem->suspend();
      }
    } catch (std::bad_alloc& ba) {
        // attempting to track down source of these on Sentry.io
        SG_LOG(SG_GENERAL, SG_ALERT, "caught bad_alloc processing subsystem:" << name);
        simgear::reportError("caught bad_alloc processing subsystem:" + name);

        if (++exceptionCount > SG_MAX_SUBSYSTEM_EXCEPTIONS) {
            SG_LOG(SG_GENERAL, SG_ALERT, "(exceptionCount=" << exceptionCount << ", suspending)");
            subsystem->suspend();
        }
    }
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystemMgr.
////////////////////////////////////////////////////////////////////////

namespace {
    SGSubsystemMgr* global_defaultSubsystemManager = nullptr;
    
    void registerSubsystemCommands();
    
} // end of anonymous namespace

SGSubsystemMgr::SGSubsystemMgr () :
  _groups(MAX_GROUPS)
{
    if (global_defaultSubsystemManager == nullptr) {
        // register ourselves as the default
        global_defaultSubsystemManager = this;
    }

    // disabled until subsystemfactory is removed from FlightGear
#if 0
    auto commandManager = SGCommandMgr::instance();
    if (commandManager && !commandManager->getCommand("add-subsystem")) {
        registerSubsystemCommands();
    }
#endif
    
    for (int i = 0; i < MAX_GROUPS; i++) {
        auto g = new SGSubsystemGroup();
        g->set_manager(this);
        _groups[i].reset(g);
    }
}

SGSubsystemMgr::~SGSubsystemMgr ()
{
    _destructorActive = true;
    _groups.clear();
    
    // if we were the global one, null out the pointer so it doesn't dangle.
    // don't do anything clever to track multiple subsystems for now; if we
    // a smarter scheme, let's wait until it's clearer what that might be.
    if (global_defaultSubsystemManager == this) {
        global_defaultSubsystemManager = nullptr;
    }
}

SGSubsystemMgr* SGSubsystemMgr::getManager(const std::string& id)
{
    if (id.empty()) {
        return global_defaultSubsystemManager;
    }
    
    // remove me when if/when we supprot multiple subsystem instances
    throw sg_exception("multiple subsystem instances not supported yet");
}

void
SGSubsystemMgr::init ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
            _groups[i]->init();
}


SGSubsystem::InitStatus
SGSubsystemMgr::incrementalInit()
{
  if (_initPosition >= MAX_GROUPS)
    return INIT_DONE;
  
  InitStatus memberStatus = _groups[_initPosition]->incrementalInit();  
  if (memberStatus == INIT_DONE)
    ++_initPosition;
  
  return INIT_CONTINUE;
}

void
SGSubsystemMgr::postinit ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
            _groups[i]->postinit();
}

void
SGSubsystemMgr::reinit ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
            _groups[i]->reinit();
}

void
SGSubsystemMgr::shutdown ()
{
    // reverse order to prevent order dependency problems
    for (int i = MAX_GROUPS-1; i >= 0; i--)
        _groups[i]->shutdown();
  
    _initPosition = 0;
}


void
SGSubsystemMgr::bind ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i]->bind();
}

void
SGSubsystemMgr::unbind ()
{
    // reverse order to prevent order dependency problems
    for (int i = MAX_GROUPS-1; i >= 0; i--)
        _groups[i]->unbind();
}

void
SGSubsystemMgr::update (double delta_time_sec)
{
    SGTimeStamp timeStamp;

    for (int i = 0; i < MAX_GROUPS; i++) {
        _groups[i]->update(delta_time_sec);
    }
    reportTimingStatsRequest = false;
}

void
SGSubsystemMgr::suspend ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i]->suspend();
}

void
SGSubsystemMgr::resume ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i]->resume();
}

bool
SGSubsystemMgr::is_suspended () const
{
    return false;
}

void
SGSubsystemMgr::add (const char * name, SGSubsystem * subsystem,
                     GroupType group, double min_time_sec)
{
    if (get_subsystem(name) != nullptr)
        throw sg_exception("Duplicate add of subsystem: " + std::string(name));
    
    SG_LOG(SG_GENERAL, SG_DEBUG, "Adding subsystem " << name);
    get_group(group)->set_subsystem(name, subsystem, min_time_sec);
}

bool
SGSubsystemMgr::remove(const char* name)
{
    // drop the cache
    _subsystemNameCache.clear();
  
// we don't know which group the subsystem belongs too
// fortunately this is a very infrequently used code path, so the slow
// search is not a problem
    for (auto group : _groups) {
        bool didRemove = group->remove_subsystem(name);
        if (didRemove) {
            return true;
        }
    } // of groups iteration
    
    SG_LOG(SG_GENERAL, SG_WARN, "SGSubsystemMgr::remove: not found: " << name);
    return false;
}


SGSubsystemGroup *
SGSubsystemMgr::get_group (GroupType group)
{
    return _groups[group];
}

SGSubsystem *
SGSubsystemMgr::get_subsystem (const string &name) const
{
    if (_destructorActive)
        return nullptr;
    
    auto s =_subsystemNameCache.find(name);
    if (s != _subsystemNameCache.end()) {
        // in the cache, excellent
        return s->second;
    }

    for (auto g : _groups) {
        auto sub = g->get_subsystem(name);
        if (sub) {
            // insert into the cache
            _subsystemNameCache[name] = sub;
            return sub;
        }
    }
    
    return nullptr;
}

SGSubsystem*
SGSubsystemMgr::get_subsystem(const std::string &name, const std::string& subsystemInstanceId) const
{
    return get_subsystem(name + SUBSYSTEM_NAME_SEPARATOR + subsystemInstanceId);
}


/** Trigger the timing callback to report data for all subsystems. */
void
SGSubsystemMgr::reportTiming()
{
    for (int i = 0; i < MAX_GROUPS; i++) {
        _groups[i]->reportTiming();
    } // of groups iteration
}

// anonymous namespace to hold registration informatipno

namespace  {
    struct RegisteredSubsystemData
    {
        RegisteredSubsystemData(const std::string& aName, bool aInstanced,
                                SGSubsystemMgr::SubsystemFactoryFunctor aFunctor,
                                SGSubsystemMgr::GroupType aGroup,
                                double aInterval) :
            name(aName),
            instanced(aInstanced),
            functor(aFunctor),
            defaultGroup(aGroup),
            defaultUpdateInterval(aInterval)
        {
        }
        
        std::string name;
        bool instanced = false;
        SGSubsystemMgr::SubsystemFactoryFunctor functor;
        SGSubsystemMgr::GroupType defaultGroup;
        double defaultUpdateInterval = 0.0;
        
        SGSubsystemMgr::DependencyVec depends;
    };
    
    using SybsystemRegistrationVec = std::vector<RegisteredSubsystemData>;
    
    SybsystemRegistrationVec &getGlobalRegistrations()
    {
        static SybsystemRegistrationVec global_registrations;
        return global_registrations;
    }

    SybsystemRegistrationVec::const_iterator findRegistration(const std::string& name)
    {
        auto &global_registrations = getGlobalRegistrations();
        auto it = std::find_if(global_registrations.begin(),
                               global_registrations.end(),
                               [name](const RegisteredSubsystemData& d)
                               { return name == d.name; });
        return it;
    }
} // of anonymous namespace

void SGSubsystemMgr::registerSubsystem(const std::string& name,
                                       SubsystemFactoryFunctor f,
                                       GroupType group,
                                       bool instanced,
                                       double updateInterval,
                                       std::initializer_list<Dependency> deps)
{
    auto &global_registrations = getGlobalRegistrations();
    if (findRegistration(name) != global_registrations.end()) {
        throw sg_exception("duplicate subsystem registration for: " + name);
    }
    
    global_registrations.push_back({name, instanced, f, group, updateInterval});
    if (deps.size() > 0) {
        global_registrations.back().depends = deps;
    }
}

auto SGSubsystemMgr::defaultGroupFor(const char* name) -> GroupType
{
    auto it = findRegistration(name);
    auto &global_registrations = getGlobalRegistrations();
    if (it == global_registrations.end()) {
        throw sg_exception("unknown subsystem registration for: " + std::string(name));
    }
    
    return it->defaultGroup;
}

double SGSubsystemMgr::defaultUpdateIntervalFor(const char* name)
{
    auto it = findRegistration(name);
    auto &global_registrations = getGlobalRegistrations();
    if (it == global_registrations.end()) {
        throw sg_exception("unknown subsystem registration for: " + std::string(name));
    }
    
    return it->defaultUpdateInterval;
}

const SGSubsystemMgr::DependencyVec&
SGSubsystemMgr::dependsFor(const char* name)
{
    auto it = findRegistration(name);
    auto &global_registrations = getGlobalRegistrations();
    if (it == global_registrations.end()) {
        throw sg_exception("unknown subsystem registration for: " + std::string(name));
    }
    
    return it->depends;
}

SGSubsystemRef
SGSubsystemMgr::create(const std::string& name)
{
    auto it = findRegistration(name);
    auto &global_registrations = getGlobalRegistrations();
    if (it == global_registrations.end()) {
        return {}; // or should this throw with a 'not registered'?
    }
    
    if (it->instanced) {
        throw sg_exception("SGSubsystemMgr::create: using non-instanced mode for instanced subsytem");
    }
    
    SGSubsystemRef ref = it->functor();
    if (!ref) {
        throw sg_exception("SGSubsystemMgr::create: functor failed to create subsystem implementation: " + name);
    }
    
    ref->set_name(name);
    return ref;
}

SGSubsystemRef
SGSubsystemMgr::createInstance(const std::string& name, const std::string& subsystemInstanceId)
{
    auto it = findRegistration(name);
    auto &global_registrations = getGlobalRegistrations();
    if (it == global_registrations.end()) {
        return {}; // or should this throw with a 'not registered'?
    }
    
    if (!it->instanced) {
        throw sg_exception("SGSubsystemMgr::create: using instanced mode for non-instanced subsytem");
    }
    
    SGSubsystemRef ref = it->functor();
    if (!ref) {
        throw sg_exception("SGSubsystemMgr::create: functor failed to create an instsance of " + name);
    }
    
    const auto combinedName = name + SUBSYSTEM_NAME_SEPARATOR + subsystemInstanceId;
    ref->set_name(combinedName);
    return ref;
}

void SGSubsystemMgr::Delegate::willChange(SGSubsystem*, State)
{
}

void SGSubsystemMgr::Delegate::didChange(SGSubsystem*, State)
{
}

void SGSubsystemMgr::addDelegate(Delegate * d)
{
    assert(d);
    _delegates.push_back(d);
}

void SGSubsystemMgr::removeDelegate(Delegate * d)
{
    assert(d);
    auto it = std::find(_delegates.begin(), _delegates.end(), d);
    if (it == _delegates.end()) {
        SG_LOG(SG_GENERAL, SG_DEV_ALERT, "removeDelegate: unknown delegate");
        return;
    }
    
    _delegates.erase(it);
}

void SGSubsystemMgr::notifyDelegatesWillChange(SGSubsystem* sub, State newState)
{
    std::for_each(_delegates.begin(), _delegates.end(), [sub, newState](Delegate* d)
                  { d->willChange(sub, newState); });
}

void SGSubsystemMgr::notifyDelegatesDidChange(SGSubsystem* sub, State state)
{
    std::for_each(_delegates.begin(), _delegates.end(), [sub, state](Delegate* d)
                  { d->didChange(sub, state); });
}

void SGSubsystemMgr::set_root_node(SGPropertyNode_ptr node)
{
    _rootNode = node;
}

SGPropertyNode_ptr
SGSubsystemMgr::root_node() const
{
    return _rootNode;
}

namespace {
    
    bool
    do_check_subsystem_running(const SGPropertyNode * arg, SGPropertyNode * root)
    {
        auto manager = SGSubsystemMgr::getManager({});
        return (manager->get_subsystem(arg->getStringValue("name")) != nullptr);
    }
    
    
    SGSubsystemMgr::GroupType mapGroupNameToType(const std::string& s)
    {
        if (s == "init")        return SGSubsystemMgr::INIT;
        if (s == "general")     return SGSubsystemMgr::GENERAL;
        if (s == "fdm")         return SGSubsystemMgr::FDM;
        if (s == "post-fdm")    return SGSubsystemMgr::POST_FDM;
        if (s == "display")     return SGSubsystemMgr::DISPLAY;
        if (s == "sound")       return SGSubsystemMgr::SOUND;
        
        SG_LOG(SG_GENERAL, SG_ALERT, "unrecognized subsystem group:" << s);
        return SGSubsystemMgr::GENERAL;
    }
    
    bool
    do_add_subsystem (const SGPropertyNode * arg, SGPropertyNode * root)
    {
        auto manager = SGSubsystemMgr::getManager({});
        std::string subsystem = arg->getStringValue("subsystem");
        
        // allow override of the name but defaultt o the subsystem name
        std::string name = arg->getStringValue("name");
        std::string subsystemInstanceId = arg->getStringValue("instance");

        if (name.empty()) {
            // default name to subsystem name, but before we parse any instance name
            name = subsystem;
        }
        
        auto separatorPos = subsystem.find(SUBSYSTEM_NAME_SEPARATOR);
        if (separatorPos != std::string::npos) {
            if (!subsystemInstanceId.empty()) {
                SG_LOG(SG_GENERAL, SG_WARN, "Specified a composite subsystem name and an instance name, please do one or the other: "
                        << subsystemInstanceId << " and " << subsystem);
                return false;
            }
            
            subsystemInstanceId = subsystem.substr(separatorPos + 1);
            subsystem = subsystem.substr(0, separatorPos);
        }
        
        SGSubsystem* sub = nullptr;
        if (!subsystemInstanceId.empty()) {
            sub = manager->createInstance(subsystem, subsystemInstanceId);
        } else {
            sub = manager->create(subsystem);
        }
        
        if (!sub)
            return false;
        
        std::string groupArg = arg->getStringValue("group");
        SGSubsystemMgr::GroupType group = SGSubsystemMgr::GENERAL;
        if (!groupArg.empty()) {
            group = mapGroupNameToType(groupArg);
        }
        
        double minTime = arg->getDoubleValue("min-time-sec", 0.0);
        
        const auto combinedName = subsystem + SUBSYSTEM_NAME_SEPARATOR + subsystemInstanceId;
        manager->add(combinedName.c_str(),
                     sub,
                     group,
                     minTime);
        
        // we no longer check for the do-bind-init flag here, since set_subsystem
        // tracks the group state and will transition the added subsystem
        // automatically

        return true;
    }
    
    bool do_remove_subsystem(const SGPropertyNode * arg, SGPropertyNode * root)
    {
        auto manager = SGSubsystemMgr::getManager({});
        std::string name = arg->getStringValue("subsystem");
        
        SGSubsystem* instance = manager->get_subsystem(name);
        if (!instance) {
            SG_LOG(SG_GENERAL, SG_ALERT, "do_remove_subsystem: unknown subsytem:" << name);
            return false;
        }
                
        // unplug from the manager (this also deletes the instance!)
        manager->remove(name.c_str());
        return true;
    }
    
    /**
     * Built-in command: reinitialize one or more subsystems.
     *
     * subsystem[*]: the name(s) of the subsystem(s) to reinitialize; if
     * none is specified, reinitialize all of them.
     */
    bool do_reinit (const SGPropertyNode * arg, SGPropertyNode * root)
    {
        bool result = true;
        auto manager = SGSubsystemMgr::getManager({});

        auto subsystems = arg->getChildren("subsystem");
        if (subsystems.empty()) {
            SG_LOG(SG_GENERAL, SG_INFO, "do_reinit: reinit-ing subsystem manager");
            manager->reinit();
        } else {
            for (auto sub : subsystems) {
                const char * name = sub->getStringValue();
                SGSubsystem* subsystem = manager->get_subsystem(name);
                if (subsystem == nullptr) {
                    result = false;
                    SG_LOG( SG_GENERAL, SG_ALERT, "Subsystem " << name << " not found" );
                } else {
                    subsystem->reinit();
                }
            }
        }

        return result;
    }
    
    /**
     * Built-in command: suspend one or more subsystems.
     *
     * subsystem[*] - the name(s) of the subsystem(s) to suspend.
     */
    bool do_suspend (const SGPropertyNode * arg, SGPropertyNode * root)
    {
        bool result = true;
        auto manager = SGSubsystemMgr::getManager({});

        for (auto subNode : arg->getChildren("subsystem")) {
            const char * name = subNode->getStringValue();
            SGSubsystem * subsystem = manager->get_subsystem(name);
            if (subsystem == nullptr) {
                result = false;
                SG_LOG(SG_GENERAL, SG_ALERT, "Subsystem " << name << " not found");
            } else {
                subsystem->suspend();
            }
        }
        return result;
    }
    
    /**
     * Built-in command: suspend one or more subsystems.
     *
     * subsystem[*] - the name(s) of the subsystem(s) to suspend.
     */
    bool do_resume (const SGPropertyNode * arg, SGPropertyNode * root)
    {
        bool result = true;
        auto manager = SGSubsystemMgr::getManager({});

        for (auto subNode : arg->getChildren("subsystem")) {
            const char * name = subNode->getStringValue();
            SGSubsystem * subsystem = manager->get_subsystem(name);
            if (subsystem == nullptr) {
                result = false;
                SG_LOG(SG_GENERAL, SG_ALERT, "Subsystem " << name << " not found");
            } else {
                subsystem->resume();
            }
        }
        return result;
    }
    
     struct CommandDef {
        const char * name;
        SGCommandMgr::command_t command;
     };
    
    CommandDef built_ins[] = {
        { "add-subsystem", do_add_subsystem },
        { "remove-subsystem", do_remove_subsystem },
        { "subsystem-running", do_check_subsystem_running },
        { "reinit", do_reinit },
        { "suspend", do_suspend },
        { "resume", do_resume },
    };

    void registerSubsystemCommands()
    {
        auto commandManager = SGCommandMgr::instance();
        for (auto b : built_ins) {
            commandManager->addCommand(b.name, b.command);
        }
    }
    
} // anonymous namespace implementing subsystem commands

// end of subsystem_mgr.cxx
