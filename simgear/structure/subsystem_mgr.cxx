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

#include <simgear/debug/logstream.hxx>
#include <simgear/timing/timestamp.hxx>

#include "exception.hxx"
#include "subsystem_mgr.hxx"

#include <simgear/math/SGMath.hxx>
#include "SGSmplstat.hxx"

const int SG_MAX_SUBSYSTEM_EXCEPTIONS = 4;

using std::string;
using State = SGSubsystem::State;

////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystem
////////////////////////////////////////////////////////////////////////

SGSubsystemTimingCb SGSubsystem::reportTimingCb = NULL;
void* SGSubsystem::reportTimingUserData = NULL;

SGSubsystem::SGSubsystem ()
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
    assert(_name.empty());
    _name = n;
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
        throw sg_exception("SGSubsystem::get_manager: subsystem " + name() + " has no group");
    return get_group()->get_manager();
}

std::string SGSubsystem::nameForState(State s)
{
    switch (s) {
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
}

////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystemGroup.
////////////////////////////////////////////////////////////////////////

class SGSubsystemGroup::Member
{    
private:
    Member (const Member &member);
public:
    Member ();
    ~Member ();
    
    void update (double delta_time_sec);

    void reportTiming(void) { if (reportTimingCb) reportTimingCb(reportTimingUserData, name, &timeStat); }
    void updateExecutionTime(double time) { timeStat += time;}

    SampleStatistic timeStat;
    std::string name;
    SGSubsystemRef subsystem;
    double min_step_sec;
    double elapsed_sec;
    bool collectTimeStats;
    int exceptionCount;
    int initTime;
};



SGSubsystemGroup::SGSubsystemGroup () :
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
    forEach([this](SGSubsystem* s){
        this->notifyWillChange(s, State::INIT);
        s->init();
        this->notifyDidChange(s, State::INIT);
    });
}

SGSubsystem::InitStatus
SGSubsystemGroup::incrementalInit()
{
    // special case this, simplifies the logic below
    if (_members.empty()) {
        return INIT_DONE;
    }

    // termination test
    if (_initPosition >= static_cast<int>(_members.size())) {
        return INIT_DONE;
    }
  
    if (_initPosition < 0) {
        // first call
        _initPosition = 0;
        notifyWillChange(_members.front()->subsystem, State::INIT);
    }
    
    const auto m = _members[_initPosition]; 
    SGTimeStamp st;
    st.stamp();
    const InitStatus memberStatus = m->subsystem->incrementalInit();
    m->initTime += st.elapsedMSec();
  
    if (memberStatus == INIT_DONE) {
        // complete init of this one
        notifyDidChange(m->subsystem, State::INIT);
        ++_initPosition;
        
        if (_initPosition < _members.size()) {
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
    forEach([this](SGSubsystem* s){
        this->notifyWillChange(s, State::POSTINIT);
        s->postinit();
        this->notifyDidChange(s, State::POSTINIT);
    });
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
    _initPosition = -1;
}

void
SGSubsystemGroup::bind ()
{
    forEach([this](SGSubsystem* s){
        this->notifyWillChange(s, State::BIND);
        s->bind();
        this->notifyDidChange(s, State::BIND);
    });
}

void
SGSubsystemGroup::unbind ()
{
    reverseForEach([this](SGSubsystem* s){
        this->notifyWillChange(s, State::UNBIND);
        s->unbind();
        this->notifyDidChange(s, State::UNBIND);
    });
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
    while (loopCount-- > 0) {
        for (auto member : _members) {
          if (recordTime)
              timeStamp = SGTimeStamp::now();

          member->update(delta_time_sec); // indirect call

          if (recordTime && reportTimingCb) {
              timeStamp = SGTimeStamp::now() - timeStamp;
              member->updateExecutionTime(timeStamp.toUSecs());
          }
      }
    } // of multiple update loop
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
    } else if (name != subsystem->name()) {
        SG_LOG(SG_GENERAL, SG_DEV_WARN, "adding subsystem to group with name '" << name
               << "', but name() returns '" << subsystem->name() << "'");
    }
    
    notifyWillChange(subsystem, State::ADD);
    Member* member = get_member(name, true);
    member->name = name;
    member->subsystem = subsystem;
    member->min_step_sec = min_step_sec;
    subsystem->set_group(this);
    notifyDidChange(subsystem, State::ADD);
}

void
SGSubsystemGroup::set_subsystem (SGSubsystem * subsystem, double min_step_sec)
{
    set_subsystem(subsystem->name(), subsystem, min_step_sec);
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
    
    try {
      subsystem->update(elapsed_sec);
      elapsed_sec = 0;
    } catch (sg_exception& e) {
      SG_LOG(SG_GENERAL, SG_ALERT, "caught exception processing subsystem:" << name
        << "\nmessage:" << e.getMessage());
      
      if (++exceptionCount > SG_MAX_SUBSYSTEM_EXCEPTIONS) {
        SG_LOG(SG_GENERAL, SG_ALERT, "(exceptionCount=" << exceptionCount <<
          ", suspending)");
        subsystem->suspend();
      }
    }
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystemMgr.
////////////////////////////////////////////////////////////////////////


SGSubsystemMgr::SGSubsystemMgr () :
  _groups(MAX_GROUPS)
{
    for (int i = 0; i < MAX_GROUPS; i++) {
        auto g = new SGSubsystemGroup;
        g->set_manager(this);
        _groups[i].reset(g);
    }
}

SGSubsystemMgr::~SGSubsystemMgr ()
{
    _destructorActive = true;
  _groups.clear();
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
    for (int i = 0; i < MAX_GROUPS; i++) {
        _groups[i]->update(delta_time_sec);
    }
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
SGSubsystemMgr::get_subsystem(const std::string &name, const std::string& instanceName) const
{
    return get_subsystem(name + "-" + instanceName);
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
    
    SybsystemRegistrationVec global_registrations;
    
    SybsystemRegistrationVec::const_iterator findRegistration(const std::string& name)
    {
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
    if (it == global_registrations.end()) {
        throw sg_exception("unknown subsystem registration for: " + std::string(name));
    }
    
    return it->defaultGroup;
}

double SGSubsystemMgr::defaultUpdateIntervalFor(const char* name)
{
    auto it = findRegistration(name);
    if (it == global_registrations.end()) {
        throw sg_exception("unknown subsystem registration for: " + std::string(name));
    }
    
    return it->defaultUpdateInterval;
}

const SGSubsystemMgr::DependencyVec&
SGSubsystemMgr::dependsFor(const char* name)
{
    auto it = findRegistration(name);
    if (it == global_registrations.end()) {
        throw sg_exception("unknown subsystem registration for: " + std::string(name));
    }
    
    return it->depends;
}

SGSubsystemRef
SGSubsystemMgr::create(const std::string& name)
{
    auto it = findRegistration(name);
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
SGSubsystemMgr::createInstance(const std::string& name, const std::string& instanceName)
{
    auto it = findRegistration(name);
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
    
    const auto combinedName = name + "-" + instanceName;
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


// end of subsystem_mgr.cxx
