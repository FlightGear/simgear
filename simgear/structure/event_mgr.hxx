// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef _SG_EVENT_MGR_HXX
#define _SG_EVENT_MGR_HXX

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

#include <utility>

#include "callback.hxx"

class SGEventMgr;

class SGTimer
{
public:
    SGTimer() = default;

    void run();

    std::string name;
    double interval = 0.0;
    simgear::Callback callback;
    bool repeat = false;
    bool running = false;

    // Allow move only
    SGTimer& operator=(const SGTimer &) = delete;
    SGTimer(const SGTimer &other) = delete;
    SGTimer& operator=(SGTimer &&) = default;
    SGTimer(SGTimer &&other) = default;
};

/*! Queue to execute SGTimers after given delays */
class SGTimerQueue final
{
public:
    SGTimerQueue() = default;
    ~SGTimerQueue() = default;      // non-virtual intentional

    void clear();
    void update(double deltaSecs, std::map<std::string, double> &timingStats);
    void insert(std::unique_ptr<SGTimer> timer, double time);
    bool removeByName(const std::string& name);

    void dump();

private:
    std::unique_ptr<SGTimer> remove();
    double nextTime() const { return _table[0].pri; }

    struct Entry {
        double pri;
        std::unique_ptr<SGTimer> timer;

        bool operator>(const Entry& other) const { return pri > other.pri; }
    };

    std::unique_ptr<SGTimer> _current_timer;
    double _now = 0.0;
    std::vector<Entry> _table;
};

class SGEventMgr : public SGSubsystem
{
public:
    SGEventMgr();
    virtual ~SGEventMgr();

    // Subsystem API.
    void init() override;
    void shutdown() override;
    void unbind() override;
    void update(double delta_time_sec) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "events"; }

    void setRealtimeProperty(SGPropertyNode* node) { _rtProp = node; }

    /**
     * Add a callback as a one-shot event.
     */
    inline void addEvent(const std::string& name, simgear::Callback cb,
                         double delay, bool sim=false)
    { add(name, std::move(cb), 0, delay, false, sim); }

    /**
     * Add a callback as a repeating task.
     */
    inline void addTask(const std::string& name,
                        simgear::Callback cb,
                        double interval, double delay=0, bool sim=false)
    { add(name, std::move(cb), interval, delay, true, sim); }


    void removeTask(const std::string& name);

    void dump();

private:
    friend class SGTimer;

    void add(const std::string& name, simgear::Callback cb,
             double interval, double delay,
             bool repeat, bool simtime);

    SGPropertyNode_ptr _freezeProp;
    SGPropertyNode_ptr _rtProp;
    SGTimerQueue _rtQueue;
    SGTimerQueue _simQueue;
    bool _inited, _shutdown;
};

#endif // _SG_EVENT_MGR_HXX
