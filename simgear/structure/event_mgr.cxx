#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "event_mgr.hxx"

#include <simgear/debug/logstream.hxx>

void SGEventMgr::add(const std::string& name, SGCallback* cb,
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

    SGTimer* t = new SGTimer;
    t->interval = interval;
    t->callback = cb;
    t->repeat = repeat;
    t->name = name;
    t->running = false;
    
    SGTimerQueue* q = simtime ? &_simQueue : &_rtQueue;

    q->insert(t, delay);
}

SGTimer::~SGTimer()
{
    delete callback;
    callback = NULL;
}

void SGTimer::run()
{
    (*callback)();
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
    
  SGTimer* t = _simQueue.findByName(name);
  if (t) {
    _simQueue.remove(t);
  } else if ((t = _rtQueue.findByName(name))) {
    _rtQueue.remove(t);
  } else {
    SG_LOG(SG_GENERAL, SG_WARN, "removeTask: no task found with name:" << name);
    return;
  }
  if (t->running) {
    // mark as not repeating so that the SGTimerQueue::update()
    // will clean it up
    t->repeat = false;
  } else {
    delete t;
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
// This is the priority queue implementation:
////////////////////////////////////////////////////////////////////////

SGTimerQueue::SGTimerQueue(int size)
{
    _now = 0;
    _numEntries = 0;
    _tableSize = 1;
    while(size > _tableSize)
        _tableSize = ((_tableSize + 1)<<1) - 1;

    _table = new HeapEntry[_tableSize];
    for(int i=0; i<_tableSize; i++) {
	    _table[i].pri = 0;
        _table[i].timer = 0;
    }
}


SGTimerQueue::~SGTimerQueue()
{
    clear();
    delete[] _table;
}

void SGTimerQueue::clear()
{
    // delete entries
    for(int i=0; i<_numEntries; i++) {
        delete _table[i].timer;
    }
    
    _numEntries = 0;
    
    // clear entire table to empty
    for(int i=0; i<_tableSize; i++) {
	    _table[i].pri = 0;
        _table[i].timer = 0;
    }
}
int maxTimerQueuePerItem_us = 30;
void SGTimerQueue::update(double deltaSecs, std::map<std::string, double> &timingStats)
{
    _now += deltaSecs;

    while (_numEntries && nextTime() <= _now) {
        SGTimer* t = remove();
        if (t->repeat)
            insert(t, t->interval);
        // warning: this is not thread safe
        // but the entire timer queue isn't either
        SGTimeStamp timeStamp;
        timeStamp.stamp();
        t->running = true;
        t->run();
        t->running = false;
        timingStats[t->name] += timeStamp.elapsedMSec() / 1000.0;
        if (!t->repeat)
            delete t;
    }
}

void SGTimerQueue::insert(SGTimer* timer, double time)
{
    if(_numEntries >= _tableSize)
	growArray();

    _numEntries++;
    _table[_numEntries-1].pri = -(_now + time);
    _table[_numEntries-1].timer = timer;

    siftUp(_numEntries-1);
}

SGTimer* SGTimerQueue::remove(SGTimer* t)
{
    int entry;
    for(entry=0; entry<_numEntries; entry++)
        if(_table[entry].timer == t)
            break;
    if(entry == _numEntries)
        return 0;

    // Swap in the last item in the table, and sift down
    swap(entry, _numEntries-1);
    _numEntries--;
    siftDown(entry);

    return t;
}

SGTimer* SGTimerQueue::remove()
{
    if(_numEntries == 0) {
	return 0;
    } else if(_numEntries == 1) {
	_numEntries = 0;
	return _table[0].timer;
    }

    SGTimer *result = _table[0].timer;
    _table[0] = _table[_numEntries - 1];
    _numEntries--;
    siftDown(0);
    return result;
}

void SGTimerQueue::siftDown(int n)
{
    // While we have children bigger than us, swap us with the biggest
    // child.
    while(lchild(n) < _numEntries) {
        int bigc = lchild(n);
        if(rchild(n) < _numEntries && pri(rchild(n)) > pri(bigc))
            bigc = rchild(n);
        if(pri(bigc) <= pri(n))
            break;
        swap(n, bigc);
        n = bigc;
    }
}

void SGTimerQueue::siftUp(int n)
{
    while((n != 0) && (_table[n].pri > _table[parent(n)].pri)) {
	swap(n, parent(n));
	n = parent(n);
    }
    siftDown(n);
}

void SGTimerQueue::growArray()
{
    _tableSize = ((_tableSize+1)<<1) - 1;
    HeapEntry *newTable = new HeapEntry[_tableSize];
    for(int i=0; i<_numEntries; i++) {
	newTable[i].pri  = _table[i].pri;
	newTable[i].timer = _table[i].timer;
    }
    delete[] _table;
    _table = newTable;
}

SGTimer* SGTimerQueue::findByName(const std::string& name) const
{
  for (int i=0; i < _numEntries; ++i) {
    if (_table[i].timer->name == name) {
      return _table[i].timer;
    }
  }
  
  return NULL;
}

void SGTimerQueue::dump()
{
    for (int i=0; i < _numEntries; ++i) {
        const auto t = _table[i].timer;
        SG_LOG(SG_GENERAL, SG_INFO, "\ttimer:" << t->name << ", interval=" << t->interval);
    }
}
