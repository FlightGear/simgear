#include "event_mgr.hxx"

#include <simgear/math/SGMath.hxx>
#include <simgear/debug/logstream.hxx>

void SGEventMgr::add(const std::string& name, SGCallback* cb,
                     double interval, double delay,
                     bool repeat, bool simtime)
{
    // Clamp the delay value to 1 usec, so that user code can use
    // "zero" as a synonym for "next frame".
    if(delay <= 0) delay = 0.000001;

    SGTimer* t = new SGTimer;
    t->interval = interval;
    t->callback = cb;
    t->mgr = this;
    t->repeat = repeat;
    t->simtime = simtime;
    t->name = name;
    
    SGTimerQueue* q = simtime ? &_simQueue : &_rtQueue;

    q->insert(t, delay);
}

void SGTimer::run()
{
    (*callback)();

    if(repeat) {
        SGTimerQueue* q = simtime ? &mgr->_simQueue : &mgr->_rtQueue;
        q->insert(this, interval);
    } else {
        delete callback;
        delete this;
    }
}

void SGEventMgr::update(double delta_time_sec)
{
    _simQueue.update(delta_time_sec);
    
    double rt = _rtProp ? _rtProp->getDoubleValue() : 0;
    _rtQueue.update(rt);
}

void SGEventMgr::removeTask(const std::string& name)
{
  SGTimer* t = _simQueue.findByName(name);
  if (t) {
    _simQueue.remove(t);
  } else if ((t = _rtQueue.findByName(name))) {
    _rtQueue.remove(t);
  } else {
    SG_LOG(SG_GENERAL, SG_WARN, "removeTask: no task found with name:" << name);
    return;
  }
  
  delete t;
}

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
    for(int i=0; i<_numEntries; i++) {
        delete _table[i].timer;
        _table[i].timer = 0;
    }
    _numEntries = 0;
    delete[] _table;
    _table = 0;
    _tableSize = 0;
}

void SGTimerQueue::update(double deltaSecs)
{
    _now += deltaSecs;
    while(_numEntries && nextTime() <= _now) {
        SGTimer* t = remove();
        t->run();
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

