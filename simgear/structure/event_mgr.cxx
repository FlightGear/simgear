#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "event_mgr.hxx"

#include <algorithm>

#include <simgear/debug/logstream.hxx>

void SGTimer::run()
{
    callback();
}

void SGEventMgr::add(const std::string& name, simgear::Callback cb,
                     double interval, double delay,
                     bool repeat, bool simtime)
{
    // Prevent Nasal from attempting to add timers after the subsystem has been
    // shut down.
    if (_shutdown)
        return;

    // Clamp the delay value to 1 usec, so that user code can use
    // "zero" as a synonym for "next frame".
    if(delay <= 0) delay = 1e-6;
    if(interval <= 0) interval = 1e-6; // No timer endless loops please...

    auto t = std::make_unique<SGTimer>();
    t->interval = interval;
    t->callback = cb;
    t->repeat = repeat;
    t->name = name;
    t->running = false;
    
    SGTimerQueue& q = simtime ? _simQueue : _rtQueue;

    q.insert(std::move(t), delay);
}

SGEventMgr::SGEventMgr() :
    _inited(false),
    _shutdown(false)
{
}

SGEventMgr::~SGEventMgr()
{
    _shutdown = true;
}

void SGEventMgr::unbind()
{
    _freezeProp.clear();
    _rtProp.clear();
}

void SGEventMgr::init()
{
    if (_inited) {
        // protected against duplicate calls here, in case
		// init ever does something more complex in the future.
		return;
    }
	
    // The event manager dtor and ctor are not called on reset, so reset the flag here.
    _shutdown = false;

    _inited = true;
}

void SGEventMgr::shutdown()
{
    _inited = false;
    _shutdown = true;

    _simQueue.clear();
    _rtQueue.clear();
}

void SGEventMgr::update(double delta_time_sec)
{
    _simQueue.update(delta_time_sec, _timerStats);

    double rt = _rtProp ? _rtProp->getDoubleValue() : 0;
    _rtQueue.update(rt, _timerStats);
}

void SGEventMgr::removeTask(const std::string& name)
{
    // due to the ordering of the event-mgr in FG, tasks can be removed
    // after we are shutdown (and hence, have all been cleared). Guard
    // against this so we don't generate warnings below.
    if (!_inited) {
        return;
    }

    bool removedSim = false;
    bool removeRT = false;

    removedSim = _simQueue.removeByName(name);
    if(!removedSim) {
        removeRT = _rtQueue.removeByName(name);
    }

    if(!removedSim && !removeRT) {
        SG_LOG(SG_GENERAL, SG_WARN, "removeTask: no task found with name:" << name);
        return;
    }
}

void SGEventMgr::dump()
{
    SG_LOG(SG_GENERAL, SG_INFO, "EventMgr: sim-time queue:");
    _simQueue.dump();
    SG_LOG(SG_GENERAL, SG_INFO, "EventMgr: real-time queue:");
    _rtQueue.dump();
}

// Register the subsystem.
SGSubsystemMgr::Registrant<SGEventMgr> registrantSGEventMgr(
    SGSubsystemMgr::DISPLAY);


////////////////////////////////////////////////////////////////////////
// SGTimerQueue
////////////////////////////////////////////////////////////////////////

void SGTimerQueue::clear()
{
    _table.clear();
}

void SGTimerQueue::update(double deltaSecs, std::map<std::string, double> &timingStats)
{
    _now += deltaSecs;

    while (!_table.empty() && nextTime() <= _now) {
        _current_timer = remove();

        // warning: this is not thread safe
        // but the entire timer queue isn't either
        SGTimeStamp timeStamp;
        timeStamp.stamp();
        _current_timer->running = true;
        _current_timer->run();
        _current_timer->running = false;
        timingStats[_current_timer->name] += timeStamp.elapsedMSec() / 1000.0;

        // insert() after run() because the timer can remove itself with removeByName()
        if(_current_timer->repeat) {
            double interval = _current_timer->interval;
            insert(std::move(_current_timer), interval);
        }

        _current_timer = nullptr;

    }
}

std::unique_ptr<SGTimer> SGTimerQueue::remove()
{
    if(_table.empty()) {
        return nullptr;
    }

    std::pop_heap(_table.begin(), _table.end(), std::greater<>());
    auto ptr = std::move(_table[_table.size()-1].timer);
    _table.pop_back();
    return ptr;
}


void SGTimerQueue::insert(std::unique_ptr<SGTimer> timer, double time)
{
    _table.push_back({_now + time, std::move(timer)});
    std::push_heap(_table.begin(), _table.end(), std::greater<>());
}

void SGTimerQueue::dump()
{
    for (const Entry &entry : _table) {
        const auto &t = entry.timer;
        SG_LOG(SG_GENERAL, SG_INFO, "\ttimer:" << t->name << ", interval=" << t->interval);
    }
}

bool SGTimerQueue::removeByName(const std::string& name)
{
    for (size_t i=0; i < _table.size(); ++i) {
        if (_table[i].timer->name == name) {
            std::pop_heap(_table.begin()+i, _table.end(), std::greater<>());
            _table.pop_back();
            if(i != 0) {
              std::make_heap(_table.begin(), _table.end(), std::greater<>());
            }
            return true;
        }
    }

    // Not found in queue, but maybe the timer is currently running
    if(_current_timer && _current_timer->name == name) {
        _current_timer->repeat = false;
        return true;
    }

    return false;
}
