#ifndef _SG_EVENT_MGR_HXX
#define _SG_EVENT_MGR_HXX

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "callback.hxx"

class SGEventMgr;

struct SGTimer {
    double interval;
    SGCallback* callback;
    SGEventMgr* mgr;
    bool repeat;
    bool simtime;
    void run();
};

class SGTimerQueue {
public:
    SGTimerQueue(int preSize=1);
    ~SGTimerQueue();

    void update(double deltaSecs);

    double now() { return _now; }

    void     insert(SGTimer* timer, double time);
    SGTimer* remove(SGTimer* timer);
    SGTimer* remove();

    SGTimer* nextTimer() { return _numEntries ? _table[0].timer : 0; }
    double   nextTime()  { return -_table[0].pri; }

private:
    // The "priority" is stored as a negative time.  This allows the
    // implemenetation to treat the "top" of the heap as the largest
    // value and avoids developer mindbugs. ;)
    struct HeapEntry { double pri; SGTimer* timer; };

    int parent(int n) { return ((n+1)/2) - 1; }
    int lchild(int n) { return ((n+1)*2) - 1; }
    int rchild(int n) { return ((n+1)*2 + 1) - 1; }
    double pri(int n) { return _table[n].pri; }
    void swap(int a, int b) {
	HeapEntry tmp = _table[a];
	_table[a] = _table[b];
	_table[b] = tmp;
    }
    void siftDown(int n);
    void siftUp(int n);
    void growArray();
    void check();

    double _now;
    HeapEntry *_table;
    int _numEntries;
    int _tableSize;
};

class SGEventMgr : public SGSubsystem
{
public:
    SGEventMgr() { _freezeProp = 0; }
    ~SGEventMgr() { _freezeProp = 0; }

    virtual void init() {}
    virtual void update(double delta_time_sec);

    void setFreezeProperty(SGPropertyNode* node) { _freezeProp = node; }

    /**
     * Add a single function callback event as a repeating task.
     * ex: addTask("foo", &Function ... )
     */
    template<typename FUNC>
    inline void addTask(const char* name, const FUNC& f,
                        double interval, double delay=0, bool sim=false)
    { add(make_callback(f), interval, delay, true, sim); }

    /**
     * Add a single function callback event as a one-shot event.
     * ex: addEvent("foo", &Function ... )
     */
    template<typename FUNC>
    inline void addEvent(const char* name, const FUNC& f,
                         double delay, bool sim=false)
    { add(make_callback(f), 0, delay, false, sim); }

    /**
     * Add a object/method pair as a repeating task.
     * ex: addTask("foo", &object, &ClassName::Method, ...)
     */
    template<class OBJ, typename METHOD>
    inline void addTask(const char* name,
                        const OBJ& o, METHOD m,
                        double interval, double delay=0, bool sim=false)
    { add(make_callback(o,m), interval, delay, true, sim); }

    /**
     * Add a object/method pair as a repeating task.
     * ex: addEvent("foo", &object, &ClassName::Method, ...)
     */
    template<class OBJ, typename METHOD>
    inline void addEvent(const char* name,
                         const OBJ& o, METHOD m,
                         double delay, bool sim=false)
    { add(make_callback(o,m), 0, delay, false, sim); }

private:
    friend struct SGTimer;

    void add(SGCallback* cb,
             double interval, double delay,
             bool repeat, bool simtime);

    SGPropertyNode* _freezeProp;
    SGTimerQueue _rtQueue; 
    SGTimerQueue _simQueue;
};

#endif // _SG_EVENT_MGR_HXX
