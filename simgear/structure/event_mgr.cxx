#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "event_mgr.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/structure/exception.hxx>

namespace // anonymous namespace for internal stuff
{
    
class SGTimer
{
public:
    SGTimer(const std::string& aName, double aInterval, SGCallback* aCb, bool aRepeat, bool aIsSimTime) :
        name(aName),
        interval(aInterval),
        callback(aCb),
        repeat(aRepeat),
        simTime(aIsSimTime)
    {
    }
    
    ~SGTimer()
    {
    }

    void run()
    {
        (*callback)();
    }
  
    std::string name;
    double interval;
    std::auto_ptr<SGCallback> callback;
    bool repeat;
    bool simTime;
    
    SGTimeStamp due;
};

bool operator==(SGTimer* t, const std::string& aName)
{
    return (t->name == aName);
}

bool orderTimers(SGTimer* a, SGTimer* b)
{
    return b->due < a->due;
}

typedef std::vector<SGTimer*> TimerQueue;

} // of anonymouse namespace

class SGEventMgr::EventMgrPrivate
{
public:
    void runQueue(const SGTimeStamp& aNow, TimerQueue& q)
    {        
        if (!q.empty()) {
       //     std::cout << "now:" << aNow << ", task " << q.front()->name << " is due:" << q.front()->due << std::endl;
        }
        
        while (!q.empty() && (q.front()->due <= aNow)) {
            execTask(aNow, q);
        }
    }
    
    void execTask(const SGTimeStamp& aNow, TimerQueue& q)
    {
        SGTimer* task = q.front();
        std::pop_heap(q.begin(), q.end(), &orderTimers);
        q.pop_back();
        
    //    std::cout << "running task " << task->name << std::endl;
     //   std::cout << "now:" << aNow << ", task due:" << task->due << std::endl;
        
        runningTask = task;
        task->run();
        runningTask = NULL;
        
        if (task->repeat) {
            scheduleTask(aNow, task, q);
        } else {
            toBeDeleted.push_back(task);
        }
    }
    
    void execFrameTasks(bool execSimTasks)
    {
        // avoid issues with modification of 'execNextFrame' while running;
        // take a copy, then clear to empty.
        TimerQueue execThisFrame;
        execThisFrame.swap(execNextFrame);
        execNextFrame.clear();
        
        for (TimerQueue::iterator it=execThisFrame.begin(); it != execThisFrame.end(); ++it) {
            // skip sim-tasks if required (we're paused, probably)
            if (!execSimTasks && (*it)->simTime) {
                execNextFrame.push_back(*it);
                continue;
            }
            
            runningTask = *it;
            (*it)->run();
            runningTask = NULL;
            toBeDeleted.push_back(*it);
        }
    }
    
    void scheduleTask(const SGTimeStamp& aNow, SGTimer* task, TimerQueue& q)
    {
        task->due.setTime(task->interval);
        task->due += aNow;
        q.push_back(task);
        std::push_heap(q.begin(), q.end(), &orderTimers);
    }
  
    void scheduleTaskWithDelay(const SGTimeStamp& aNow, SGTimer* task, TimerQueue& q, double delay)
    {
      task->due.setTime(delay);
      task->due += aNow;
      q.push_back(task);
      std::push_heap(q.begin(), q.end(), &orderTimers);
    }
  
    // linear search, O(N) in size of the queue
    SGTimer* removeByName(const std::string& aName, TimerQueue& q)
    {
        SGTimer* result = NULL;
        TimerQueue::iterator it = std::find(q.begin(), q.end(), aName);
        if (it != q.end()) {
            result = *it;
            q.erase(it);
        // since we're modifying the vector outside the heap operations,
        // we must remake the heap now. 
            std::make_heap(q.begin(), q.end(), &orderTimers);
        }
        
        return result;
    }
    
    void clearDeleteList()
    {
        for (TimerQueue::iterator it=toBeDeleted.begin(); it != toBeDeleted.end(); ++it) {
            delete *it;
        }
        
        toBeDeleted.clear();
    }
    
    TimerQueue simQ, realtimeQ;
    SGTimeStamp simNow; ///< track simulation now time
    TimerQueue toBeDeleted;
    TimerQueue execNextFrame;
    
    SGTimer* runningTask;
};

/////////////////////////////////////////////////////////////////////////////

SGEventMgr::SGEventMgr() :
  d(new EventMgrPrivate)
{
    d->runningTask = NULL;
}

SGEventMgr::~SGEventMgr()
{
    for (TimerQueue::iterator it=d->simQ.begin(); it != d->simQ.end(); ++it) {
      delete *it;
    }
    
    for (TimerQueue::iterator it=d->realtimeQ.begin(); it != d->realtimeQ.end(); ++it) {
      delete *it;
    }
    
    d->clearDeleteList();
}

void SGEventMgr::add(const std::string& name, SGCallback* cb,
                     double interval,
                     bool repeat, bool simtime)
{
    if (repeat && (interval <= 0.0)) {
        SG_LOG(SG_GENERAL, SG_ALERT, "requested a repeating event with 0 interval, forbidden");
        throw sg_range_exception("zero-interval repeating event requested");
    }
    
    SGTimer* t = new SGTimer(name, interval, cb, repeat, simtime);
    if (interval <= 0.0) {
        // special-case, next-frame timer
        d->execNextFrame.push_back(t);
        return;
    }
        
    TimerQueue& q(simtime ? d->simQ : d->realtimeQ);
    SGTimeStamp now = simtime ? d->simNow : SGTimeStamp::now();
    d->scheduleTaskWithDelay(now, t, q, interval);
}

void SGEventMgr::update(double delta_time_sec)
{ 
    d->execFrameTasks(delta_time_sec > 0.0);
    
    d->runQueue(SGTimeStamp::now(), d->realtimeQ);
  
// update our model of simulation time - better if this was absolute?
    d->simNow += SGTimeStamp::fromSec(delta_time_sec);
   
// run the simulation time queue
    d->runQueue(d->simNow, d->simQ);
    
    d->clearDeleteList();
}

void SGEventMgr::removeTask(const std::string& name)
{
    std::cout << "removing task:" << name << std::endl;
    
    if (d->runningTask && d->runningTask->name == name) {
        d->runningTask->repeat = false;
        return; // everything else will be done by execTask;
    }
    
    SGTimer* t = d->removeByName(name, d->simQ);
    if (!t) {
        t = d->removeByName(name, d->realtimeQ);
    }
  
    if (!t) {
        SG_LOG(SG_GENERAL, SG_WARN, "removeTask: no task found with name:" << name);
        return;
    }

    d->toBeDeleted.push_back(t);
}

void SGEventMgr::rescheduleTask(const std::string& name, double aInterval)
{
    SGTimer* t = d->removeByName(name, d->simQ);
    if (t) {
        d->scheduleTask(d->simNow, t, d->simQ);
        return;
    }
    
    t = d->removeByName(name, d->realtimeQ);
    if (t) {
        d->scheduleTask(SGTimeStamp::now(), t, d->realtimeQ);
        return;
    }
    
    SG_LOG(SG_GENERAL, SG_WARN, "rescheduleTask: no task found with name:" << name);
}
