#ifndef _SG_EVENT_MGR_HXX
#define _SG_EVENT_MGR_HXX

#include <memory> 

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include "callback.hxx"

#if 0
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

    SGTimer* findByName(const std::string& name) const;
private:
    // The "priority" is stored as a negative time.  This allows the
    // implementation to treat the "top" of the heap as the largest
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

    // gcc complains there is no function specification anywhere.
    // void check();

    double _now;
    HeapEntry *_table;
    int _numEntries;
    int _tableSize;
};
#endif

class SGEventMgr : public SGSubsystem
{
public:
    SGEventMgr();
    ~SGEventMgr();

    virtual void init() {}
    virtual void update(double delta_time_sec);

    // FIXME - remove once FG is in sync.
    void setRealtimeProperty(SGPropertyNode*) { }

    void setSimTime(SGPropertyNode* node);

    /**
     * Add a single function callback event as a repeating task.
     * ex: addTask("foo", &Function ... )
     */
    template<typename FUNC>
    inline void addTask(const std::string& name, const FUNC& f,
                        double interval, bool sim=false)
    { add(name, make_callback(f), interval, true, sim); }

    /**
     * Add a single function callback event as a one-shot event.
     * ex: addEvent("foo", &Function ... )
     */
    template<typename FUNC>
    inline void addEvent(const std::string& name, const FUNC& f,
                         double delay, bool sim=false)
    { add(name, make_callback(f), delay, false, sim); }

    /**
     * Add a object/method pair as a repeating task.
     * ex: addTask("foo", &object, &ClassName::Method, ...)
     */
    template<class OBJ, typename METHOD>
    inline void addTask(const std::string& name,
                        const OBJ& o, METHOD m,
                        double interval, bool sim=false)
    { add(name, make_callback(o,m), interval, true, sim); }

    /**
     * Add a object/method pair as a repeating task.
     * ex: addEvent("foo", &object, &ClassName::Method, ...)
     */
    template<class OBJ, typename METHOD>
    inline void addEvent(const std::string& name,
                         const OBJ& o, METHOD m,
                         double delay, bool sim=false)
    { add(name, make_callback(o,m), delay, false, sim); }


    void removeTask(const std::string& name);
  
    void rescheduleTask(const std::string& name, double aInterval);
private:
    class EventMgrPrivate;
    
    void add(const std::string& name, SGCallback* cb,
             double interval,
             bool repeat, bool simtime);

    std::auto_ptr<EventMgrPrivate> d;
};

#endif // _SG_EVENT_MGR_HXX
