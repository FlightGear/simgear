
#include <simgear/debug/logstream.hxx>
#include <simgear/timing/timestamp.hxx>

#include "exception.hxx"
#include "subsystem_mgr.hxx"

#include <simgear/math/SGMath.hxx>



////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystem
////////////////////////////////////////////////////////////////////////


SGSubsystem::SGSubsystem ()
  : _suspended(false)
{
}

SGSubsystem::~SGSubsystem ()
{
}

void
SGSubsystem::init ()
{
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
  _suspended = true;
}

void
SGSubsystem::suspend (bool suspended)
{
  _suspended = suspended;
}

void
SGSubsystem::resume ()
{
  _suspended = false;
}

bool
SGSubsystem::is_suspended () const
{
  return _suspended;
}


void
SGSubsystem::printTimingInformation ()
{
   SGTimeStamp startTime;
   for ( eventTimeVecIterator i = timingInfo.begin();
          i != timingInfo.end();
          i++) {
       if (i == timingInfo.begin()) {
           startTime = i->getTime();
       } else {
           SGTimeStamp endTime = i->getTime();
           SG_LOG(SG_GENERAL, SG_ALERT, "- Getting to timestamp :   "
                  << i->getName() << " takes " << endTime - startTime
                  << " sec.");
	   startTime = endTime;
       }
   }
}



void SGSubsystem::stamp(const string& name)
{
    timingInfo.push_back(TimingInfo(name, SGTimeStamp::now()));
}


////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystemGroup.
////////////////////////////////////////////////////////////////////////

SGSubsystemGroup::SGSubsystemGroup () :
  _fixedUpdateTime(-1.0),
  _updateTimeRemainder(0.0)
{
}

SGSubsystemGroup::~SGSubsystemGroup ()
{
    // reverse order to prevent order dependency problems
    for (unsigned int i = _members.size(); i > 0; i--)
    {
        _members[i-1]->printTimingStatistics();
        delete _members[i-1];
    }
}

void
SGSubsystemGroup::init ()
{
    for (unsigned int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->init();
}

void
SGSubsystemGroup::postinit ()
{
    for (unsigned int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->postinit();
}

void
SGSubsystemGroup::reinit ()
{
    for (unsigned int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->reinit();
}

void
SGSubsystemGroup::bind ()
{
    for (unsigned int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->bind();
}

void
SGSubsystemGroup::unbind ()
{
    // reverse order to prevent order dependency problems
    for (unsigned int i = _members.size(); i > 0; i--)
       _members[i-1]->subsystem->unbind();
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
      _updateTimeRemainder = delta_time_sec - (loopCount * _fixedUpdateTime);
      delta_time_sec = _fixedUpdateTime;
    }

    while (loopCount-- > 0) {
      for (unsigned int i = 0; i < _members.size(); i++)
      {
           SGTimeStamp timeStamp = SGTimeStamp::now();
           _members[i]->update(delta_time_sec); // indirect call
           timeStamp = timeStamp - SGTimeStamp::now();
           double b = timeStamp.toUSecs();
           _members[i]->updateExecutionTime(b);
           double threshold = _members[i]->getTimeWarningThreshold();
           if (( b > threshold ) && (b > 10000)) {
               _members[i]->printTimingInformation(b);
           }
      }
    } // of multiple update loop
}

void 
SGSubsystemGroup::collectDebugTiming(bool collect)
{
    for (unsigned int i = 0; i < _members.size(); i++)
    {
        _members[i]->collectDebugTiming(collect);
    }
}

void
SGSubsystemGroup::suspend ()
{
    for (unsigned int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->suspend();
}

void
SGSubsystemGroup::resume ()
{
    for (unsigned int i = 0; i < _members.size(); i++)
        _members[i]->subsystem->resume();
}

bool
SGSubsystemGroup::is_suspended () const
{
    return false;
}

void
SGSubsystemGroup::set_subsystem (const string &name, SGSubsystem * subsystem,
                                 double min_step_sec)
{
    Member * member = get_member(name, true);
    if (member->subsystem != 0)
        delete member->subsystem;
    member->name = name;
    member->subsystem = subsystem;
    member->min_step_sec = min_step_sec;
}

SGSubsystem *
SGSubsystemGroup::get_subsystem (const string &name)
{
    Member * member = get_member(name);
    if (member != 0)
        return member->subsystem;
    else
        return 0;
}

void
SGSubsystemGroup::remove_subsystem (const string &name)
{
    for (unsigned int i = 0; i < _members.size(); i++) {
        if (name == _members[i]->name) {
            _members.erase(_members.begin() + i);
            return;
        }
    }
}

void
SGSubsystemGroup::set_fixed_update_time(double dt)
{
  _fixedUpdateTime = dt;
}

void
SGSubsystemGroup::Member::printTimingStatistics ()
{
    if (collectTimeStats) {
        double minTime = timeStat.min()   / 1000;
        double maxTime = timeStat.max()   / 1000;
        double meanTime = timeStat.mean() / 1000;
        double stddev   = timeStat.stdDev()   / 1000;

        char buffer[256];
        snprintf(buffer, 256, "Timing summary for %20s.\n"
                              "-  mean time: %04.2f ms.\n"
                              "-  min time : %04.2f ms.\n"
                              "-  max time : %04.2f ms.\n"
                              "- stddev    : %04.2f ms.\n", name.c_str(), meanTime, minTime, maxTime, stddev);
        SG_LOG(SG_GENERAL, SG_ALERT, buffer);
    }
}


bool
SGSubsystemGroup::has_subsystem (const string &name) const
{
    return (((SGSubsystemGroup *)this)->get_member(name) != 0);
}

SGSubsystemGroup::Member *
SGSubsystemGroup::get_member (const string &name, bool create)
{
    for (unsigned int i = 0; i < _members.size(); i++) {
        if (_members[i]->name == name)
            return _members[i];
    }
    if (create) {
        Member * member = new Member;
        _members.push_back(member);
        return member;
    } else {
        return 0;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystemGroup::Member
////////////////////////////////////////////////////////////////////////


SGSubsystemGroup::Member::Member ()
    : name(""),
      subsystem(0),
      min_step_sec(0),
      elapsed_sec(0),
      collectTimeStats(false)
{
}

// This shouldn't be called due to subsystem pointer ownership issues.
SGSubsystemGroup::Member::Member (const Member &)
{
}

SGSubsystemGroup::Member::~Member ()
{
    delete subsystem;
}

void
SGSubsystemGroup::Member::update (double delta_time_sec)
{
    elapsed_sec += delta_time_sec;
    if (elapsed_sec >= min_step_sec) {
        if (!subsystem->is_suspended()) {
            subsystem->update(elapsed_sec);
            elapsed_sec = 0;
        }
    }
}


void 
SGSubsystemGroup::Member::printTimingInformation(double time)
{
     if (collectTimeStats) {
         SG_LOG(SG_GENERAL, SG_ALERT, "Subsystem Timing Alert : " << time << " " << name);
         subsystem->printTimingInformation();
     }
}

double SGSubsystemGroup::Member::getTimeWarningThreshold()
{
    return (timeStat.mean() + 3 * timeStat.stdDev());
}

void SGSubsystemGroup::Member::updateExecutionTime(double time)
{
    if (collectTimeStats) {
        timeStat += time;
    }
}





////////////////////////////////////////////////////////////////////////
// Implementation of SGSubsystemMgr.
////////////////////////////////////////////////////////////////////////


SGSubsystemMgr::SGSubsystemMgr ()
{
}

SGSubsystemMgr::~SGSubsystemMgr ()
{
}

void
SGSubsystemMgr::init ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
            _groups[i].init();
}

void
SGSubsystemMgr::postinit ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
            _groups[i].postinit();
}

void
SGSubsystemMgr::reinit ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
            _groups[i].reinit();
}

void
SGSubsystemMgr::bind ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i].bind();
}

void
SGSubsystemMgr::unbind ()
{
    // reverse order to prevent order dependency problems
    for (int i = MAX_GROUPS-1; i >= 0; i--)
        _groups[i].unbind();
}

void
SGSubsystemMgr::update (double delta_time_sec)
{
    for (int i = 0; i < MAX_GROUPS; i++) {
        _groups[i].update(delta_time_sec);
    }
}

void 
SGSubsystemMgr::collectDebugTiming(bool collect)
{
    for (int i = 0; i < MAX_GROUPS; i++) {
        _groups[i].collectDebugTiming(collect);
    }
}

void
SGSubsystemMgr::suspend ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i].suspend();
}

void
SGSubsystemMgr::resume ()
{
    for (int i = 0; i < MAX_GROUPS; i++)
        _groups[i].resume();
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
    SG_LOG(SG_GENERAL, SG_INFO, "Adding subsystem " << name);
    get_group(group)->set_subsystem(name, subsystem, min_time_sec);

    if (_subsystem_map.find(name) != _subsystem_map.end()) {
        SG_LOG(SG_GENERAL, SG_ALERT, "Adding duplicate subsystem " << name);
        throw sg_exception("duplicate subsystem");
    }
    _subsystem_map[name] = subsystem;
}

SGSubsystemGroup *
SGSubsystemMgr::get_group (GroupType group)
{
    return &(_groups[group]);
}

SGSubsystem *
SGSubsystemMgr::get_subsystem (const string &name)
{
    map<string,SGSubsystem *>::iterator s =_subsystem_map.find(name);

    if (s == _subsystem_map.end())
        return 0;
    else
        return s->second;
}

// end of fgfs.cxx
