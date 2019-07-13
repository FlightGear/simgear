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


#ifndef __SUBSYSTEM_MGR_HXX
#define __SUBSYSTEM_MGR_HXX 1


#include <simgear/compiler.h>

#include <string>
#include <map>
#include <vector>
#include <functional>

#include <simgear/timing/timestamp.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/props/propsfwd.hxx>

class TimingInfo
{
private:
    std::string eventName;
    SGTimeStamp time;

public:
    TimingInfo(const std::string& name, const SGTimeStamp &t) :
        eventName(name), time(t)
    { }
    const std::string& getName() const { return eventName; }
    const SGTimeStamp& getTime() const { return time; }
};

// forward decls
class SampleStatistic;
class SGSubsystemGroup;
class SGSubsystemMgr;

typedef std::vector<TimingInfo> eventTimeVec;
typedef std::vector<TimingInfo>::iterator eventTimeVecIterator;

typedef void (*SGSubsystemTimingCb)(void* userData, const std::string& name, SampleStatistic* pStatistic);

/**
 * Basic interface for all FlightGear subsystems.
 *
 * <p>This is an abstract interface that all FlightGear subsystems
 * will eventually implement.  It defines the basic operations for
 * each subsystem: initialization, property binding and unbinding, and
 * updating.  Interfaces may define additional methods, but the
 * preferred way of exchanging information with other subsystems is
 * through the property tree.</p>
 *
 * <p>To publish information through a property, a subsystem should
 * bind it to a variable or (if necessary) a getter/setter pair in the
 * bind() method, and release the property in the unbind() method:</p>
 *
 * <pre>
 * void MySubsystem::bind ()
 * {
 *   fgTie("/controls/flight/elevator", &_elevator);
 *   fgSetArchivable("/controls/flight/elevator");
 * }
 *
 * void MySubsystem::unbind ()
 * {
 *   fgUntie("/controls/flight/elevator");
 * }
 * </pre>
 *
 * <p>To reference a property (possibly) from another subsystem, there
 * are two alternatives.  If the property will be referenced only
 * infrequently (say, in the init() method), then the fgGet* methods
 * declared in fg_props.hxx are the simplest:</p>
 *
 * <pre>
 * void MySubsystem::init ()
 * {
 *   _errorMargin = fgGetFloat("/display/error-margin-pct");
 * }
 * </pre>
 *
 * <p>On the other hand, if the property will be referenced frequently
 * (say, in the update() method), then the hash-table lookup required
 * by the fgGet* methods might be too expensive; instead, the
 * subsystem should obtain a reference to the actual property node in
 * its init() function and use that reference in the main loop:</p>
 *
 * <pre>
 * void MySubsystem::init ()
 * {
 *   _errorNode = fgGetNode("/display/error-margin-pct", true);
 * }
 *
 * void MySubsystem::update (double delta_time_sec)
 * {
 *   do_something(_errorNode.getFloatValue());
 * }
 * </pre>
 *
 * <p>The node returned will always be a pointer to SGPropertyNode,
 * and the subsystem should <em>not</em> delete it in its destructor
 * (the pointer belongs to the property tree, not the subsystem).</p>
 *
 * <p>The program may ask the subsystem to suspend or resume
 * sim-time-dependent operations; by default, the suspend() and
 * resume() methods set the protected variable <var>_suspended</var>,
 * which the subsystem can reference in its update() method, but
 * subsystems may also override the suspend() and resume() methods to
 * take different actions.</p>
 */

class SGSubsystem : public SGReferenced
{
public:
    using TimerStats = std::map<std::string, double>;
    /**
   * Default constructor.
   */
  SGSubsystem ();

  /**
   * Virtual destructor to ensure that subclass destructors are called.
   */
  virtual ~SGSubsystem ();


  /**
   * Initialize the subsystem.
   *
   * <p>This method should set up the state of the subsystem, but
   * should not bind any properties.  Note that any dependencies on
   * the state of other subsystems should be placed here rather than
   * in the constructor, so that FlightGear can control the
   * initialization order.</p>
   */
  virtual void init ();

  typedef enum
  {
    INIT_DONE,      ///< subsystem is fully initialised
    INIT_CONTINUE   ///< init should be called again
  } InitStatus;

  virtual InitStatus incrementalInit ();

  /**
   * Initialize parts that depend on other subsystems having been initialized.
   *
   * <p>This method should set up all parts that depend on other
   * subsystems. One example is the scripting/Nasal subsystem, which
   * is initialized last. So, if a subsystem wants to execute Nasal
   * code in subsystem-specific configuration files, it has to do that
   * in its postinit() method.</p>
   */
  virtual void postinit ();


  /**
   * Reinitialize the subsystem.
   *
   * <p>This method should cause the subsystem to reinitialize itself,
   * and (normally) to reload any configuration files.</p>
   */
  virtual void reinit ();


  /**
   * Shutdown the subsystem.
   *
   * <p>Release any state associated with subsystem. Shutdown happens in
   * the reverse order to init(), so this is the correct place to do
   * shutdown that depends on other subsystems.
   * </p>
   */
  virtual void shutdown ();

  /**
   * Acquire the subsystem's property bindings.
   *
   * <p>This method should bind all properties that the subsystem
   * publishes.  It will be invoked after init, but before any
   * invocations of update.</p>
   */
  virtual void bind ();


  /**
   * Release the subsystem's property bindings.
   *
   * <p>This method should release all properties that the subsystem
   * publishes.  It will be invoked by FlightGear (not the destructor)
   * just before the subsystem is removed.</p>
   */
  virtual void unbind ();


  /**
   * Update the subsystem.
   *
   * <p>FlightGear invokes this method every time the subsystem should
   * update its state.</p>
   *
   * @param delta_time_sec The delta time, in seconds, since the last
   * update.  On first update, delta time will be 0.
   */
  virtual void update (double delta_time_sec) = 0;


  /**
   * Suspend operation of this subsystem.
   *
   * <p>This method instructs the subsystem to suspend
   * sim-time-dependent operations until asked to resume.  The update
   * method will still be invoked so that the subsystem can take any
   * non-time-dependent actions, such as updating the display.</p>
   *
   * <p>It is not an error for the suspend method to be invoked when
   * the subsystem is already suspended; the invocation should simply
   * be ignored.</p>
   */
  virtual void suspend ();


  /**
   * Suspend or resume operation of this subsystem.
   *
   * @param suspended true if the subsystem should be suspended, false
   * otherwise.
   */
  virtual void suspend (bool suspended);


  /**
   * Resume operation of this subsystem.
   *
   * <p>This method instructs the subsystem to resume
   * sim-time-depended operations.  It is not an error for the resume
   * method to be invoked when the subsystem is not suspended; the
   * invocation should simply be ignored.</p>
   */
  virtual void resume ();


  /**
   * Test whether this subsystem is suspended.
   *
   * @return true if the subsystem is suspended, false if it is not.
   */
  virtual bool is_suspended () const;

  /**
   * Trigger the callback to report timing information for all subsystems.
   */
  void reportTiming(void);

  virtual void reportTimingStats(TimerStats *_lastValues);
  /**
   * Place time stamps at strategic points in the execution of subsystems
   * update() member functions. Predominantly for debugging purposes.
   */
  void stamp(const std::string& name);

    /**
     * composite name for this subsystem (type name & optional instance name)
     */
    std::string subsystemId() const
    { return _subsystemId; }

    /**
     * @brief the type (class)-specific part of the subsystem name.
     */
    std::string subsystemClassId() const;
    
    /**
     * @brief the instance part of the subsystem name. Empty if this
     * subsystem is not instanced
     */
    std::string subsystemInstanceId() const;
    
    virtual bool is_group() const
    { return false; }
    
    virtual SGSubsystemMgr* get_manager() const;

    /// get the parent group of this subsystem
    SGSubsystemGroup* get_group() const;
    
    // ordering here is exceptionally important, due to
    // liveness of ranges. If you're extending this consider
    // carefully where the new state lies and position it correctly.
    // Also extend the tests to ensure you handle all cases.
    enum class State {
        INVALID = -1,
        ADD = 0,
        BIND,
        INIT,
        POSTINIT,
        REINIT,
        SUSPEND,
        RESUME,
        SHUTDOWN,
        UNBIND,
        REMOVE
    };
    
    /**
     * debug helper, print a state as a string
     */
    static std::string nameForState(State s);

    /**
    * gets fine grained stats of time elapsed since last clear
    * returns map of ident and time
    */
    virtual const TimerStats  &getTimerStats() {
        return _timerStats;
    }

    /**
    * clear fine grained stats that are over the specified value.
    */
    virtual void resetTimerStats(double val = 0) { }

protected:
    friend class SGSubsystemMgr;
    friend class SGSubsystemGroup;

    void set_name(const std::string& n);

    void set_group(SGSubsystemGroup* group);

    /// composite name for the subsystem (type name and instance name if this
    /// is an instanced subsystem. (Since this member was originally defined as
    /// protected, not private, we can't rename it easily)
    std::string _name;
    
    bool _suspended = false;

    eventTimeVec timingInfo;

    static SGSubsystemTimingCb reportTimingCb;
    static void* reportTimingUserData;
    static bool reportTimingStatsRequest;
    static int maxTimePerFrame_ms;

private:
    /// composite name for the subsystem (type name and instance name if this
    /// is an instanced subsystem. (Since this member was originally defined as
    /// protected, not private, we can't rename it easily)
    std::string _subsystemId;

    SGSubsystemGroup* _group = nullptr;
protected:
    TimerStats _timerStats, _lastTimerStats;
    double _executionTime;
    double _lastExecutionTime;

};

typedef SGSharedPtr<SGSubsystem> SGSubsystemRef;

/**
 * A group of FlightGear subsystems.
 */
class SGSubsystemGroup : public SGSubsystem
{
public:
    SGSubsystemGroup ();
    virtual ~SGSubsystemGroup ();

    // Subsystem API.
    void bind() override;
    InitStatus incrementalInit() override;
    void init() override;
    void postinit() override;
    void reinit() override;
    void resume() override;
    void shutdown() override;
    void suspend() override;
    void unbind() override;
    void update(double delta_time_sec) override;
    bool is_suspended() const override;

    virtual void set_subsystem (const std::string &name,
                                SGSubsystem * subsystem,
                                double min_step_sec = 0);
    
    void set_subsystem (SGSubsystem * subsystem, double min_step_sec = 0);
    
    virtual SGSubsystem * get_subsystem (const std::string &name);
    bool remove_subsystem (const std::string &name);
    virtual bool has_subsystem (const std::string &name) const;

    void reportTimingStats(TimerStats *_lastValues) override;
    /**
     * Remove all subsystems.
     */
    virtual void clearSubsystems();

    void reportTiming(void);

    /**
     *
     */
    void set_fixed_update_time(double fixed_dt);

    /**
     * retrive list of member subsystem names
     */
    string_list member_names() const;

    template<class T>
    T* get_subsystem()
    {
        return dynamic_cast<T*>(get_subsystem(T::staticSubsystemClassId()));
    }

    bool is_group() const override
    { return true; }
    
    SGSubsystemMgr* get_manager() const override;

private:
    void forEach(std::function<void(SGSubsystem*)> f);
    void reverseForEach(std::function<void(SGSubsystem*)> f);

    void notifyWillChange(SGSubsystem* sub, SGSubsystem::State s);
    void notifyDidChange(SGSubsystem* sub, SGSubsystem::State s);
    
    friend class SGSubsystemMgr;

    void set_manager(SGSubsystemMgr* manager);
    
    class Member;
    Member* get_member (const std::string &name, bool create = false);
    
    using MemberVec = std::vector<Member*>;
    MemberVec _members;

    // track the state of this group, so we can transition added/removed
    // members correctly
    SGSubsystem::State _state = SGSubsystem::State::INVALID;
    double _fixedUpdateTime;
    double _updateTimeRemainder;

  /// index of the member we are currently init-ing
    int _initPosition;
    
    /// back-pointer to the manager, for the root groups. (sub-groups
    /// will have this as null, and chain via their parent)
    SGSubsystemMgr* _manager = nullptr;
};

typedef SGSharedPtr<SGSubsystemGroup> SGSubsystemGroupRef;

/**
 * Manage subsystems for FlightGear.
 *
 * This top-level subsystem will eventually manage all of the
 * subsystems in FlightGear: it broadcasts its life-cycle events
 * (init, bind, etc.) to all of the subsystems it manages.  Subsystems
 * are grouped to guarantee order of initialization and execution --
 * currently, the only two groups are INIT and GENERAL, but others
 * will appear in the future.
 *
 * All subsystems are named as well as grouped, and subsystems can be
 * looked up by name and cast to the appropriate subtype when another
 * subsystem needs to invoke specialized methods.
 *
 * The subsystem manager owns the pointers to all the subsystems in
 * it.
 */
class SGSubsystemMgr : public SGSubsystem
{
public:
    SGSubsystemMgr(const SGSubsystemMgr&) = delete;
    SGSubsystemMgr& operator=(const SGSubsystemMgr&) = delete;

    /**
     * Types of subsystem groups.
     */
    enum GroupType {
        INVALID = -1,
        INIT = 0,
        GENERAL,
        FDM,        ///< flight model, autopilot, instruments that run coupled
        POST_FDM,   ///< certain subsystems depend on FDM data
        DISPLAY,    ///< view, camera, rendering updates
        SOUND/*I want to be last!*/,  ///< needs to run AFTER display, to allow concurrent GPU/sound processing
        MAX_GROUPS
    };

    SGSubsystemMgr ();
    virtual ~SGSubsystemMgr ();

    // Subsystem API.
    void bind() override;
    void init() override;
    InitStatus incrementalInit() override;
    void postinit() override;
    void reinit() override;
    void resume() override;
    void shutdown() override;
    void suspend() override;
    void unbind() override;
    void update(double delta_time_sec) override;
    bool is_suspended() const override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "subsystem-mgr"; }

    virtual void add (const char * name,
                      SGSubsystem * subsystem,
                      GroupType group = GENERAL,
                      double min_time_sec = 0);

    /**
     * remove a subsystem, and return true on success
     * returns false if the subsystem was not found
     */
    bool remove(const char* name);

    virtual SGSubsystemGroup * get_group (GroupType group);

    SGSubsystem* get_subsystem(const std::string &name) const;

    SGSubsystem* get_subsystem(const std::string &name, const std::string& subsystemInstanceId) const;

    void reportTiming();
    void setReportTimingCb(void* userData, SGSubsystemTimingCb cb) { reportTimingCb = cb; reportTimingUserData = userData; }
    void setReportTimingStats(bool v) { reportTimingStatsRequest = v; }

    /**
     * @brief set the root property node for this subsystem manager
     * subsystems can retrieve this value during init/bind (or at any time)
     * to base their own properties trees off of.
     */
    void set_root_node(SGPropertyNode_ptr node);
    
    /**
     * @brief retrieve the root property node for this subsystem manager
     */
    SGPropertyNode_ptr root_node() const;
    
    template<class T>
    T* get_subsystem() const
    {
        return dynamic_cast<T*>(get_subsystem(T::staticSubsystemClassId()));
    }

// instanced overloads, for both raw char* and std::string
// these concatenate the subsystem type name with the instance name
    template<class T>
    T* get_subsystem(const char* subsystemInstanceId) const
    {
        return dynamic_cast<T*>(get_subsystem(T::staticSubsystemClassId(), subsystemInstanceId));
    }

    template<class T>
    T* get_subsystem(const std::string& subsystemInstanceId) const
    {
        return dynamic_cast<T*>(get_subsystem(T::staticSubsystemClassId(), subsystemInstanceId));
    }

    /**
     * @brief Subsystem dependency structure.
     */
    struct Dependency {
        enum Type {
            HARD,               ///< The subsystem cannot run without this subsystem dependency.
            SOFT,               ///< The subsystem uses this subsystem dependency, but can run without it.
            SEQUENCE,           ///< Used for ordering subsystem initialisation.
            NONSUBSYSTEM_HARD,  ///< The subsystem cannot run without this non-subsystem dependency.
            NONSUBSYSTEM_SOFT,  ///< The subsystem uses this non-subsystem dependency, but can run without it.
            PROPERTY            ///< The subsystem requires this property to exist to run.
        };

        std::string name;
        Type type;
    };

    using DependencyVec = std::vector<SGSubsystemMgr::Dependency>;
    using SubsystemFactoryFunctor = std::function<SGSubsystemRef()>;

    /**
     * @brief register a subsytem with the manager
     *
     */ 
    static void registerSubsystem(const std::string& name,
                                  SubsystemFactoryFunctor f,
                                  GroupType group,
                                  bool isInstanced = false,
                                  double updateInterval = 0.0,
                                  std::initializer_list<Dependency> deps = {});

    template<class T>
    class Registrant 
    {
    public:
        Registrant(GroupType group = GENERAL,
                   std::initializer_list<Dependency> deps = {},
                   double updateInterval = 0.0)
        {
            SubsystemFactoryFunctor factory = [](){ return new T; };
            SGSubsystemMgr::registerSubsystem(T::staticSubsystemClassId(),
                                              factory, group,
                                              false, updateInterval,
                                              deps);
        }

        // could implement a dtor to unregister but not needed at the moment
    };
    
    template<class T>
    class InstancedRegistrant
    {
    public:
        InstancedRegistrant(GroupType group = GENERAL,
                            std::initializer_list<Dependency> deps = {},
                            double updateInterval = 0.0)
        {
            SubsystemFactoryFunctor factory = [](){ return new T; };
            SGSubsystemMgr::registerSubsystem(T::staticSubsystemClassId(),
                                              factory, group,
                                              true, updateInterval,
                                              deps);
        }
        
        // could implement a dtor to unregister but not needed at the moment
    };
    
    /**
     * @brief templated add function, subsystem is deduced automatically
     *
     */
    template <class T>
    SGSharedPtr<T> add(GroupType customGroup = INVALID, double customInterval = 0.0)
    {
        auto ref = create(T::staticSubsystemClassId());
        
        
        const GroupType group = (customGroup == INVALID) ?
            defaultGroupFor(T::staticSubsystemClassId()) : customGroup;
        const double interval = (customInterval == 0.0) ?
        defaultUpdateIntervalFor(T::staticSubsystemClassId()) : customInterval;
        add(ref->subsystemId().c_str(), ref.ptr(), group, interval);
        return dynamic_cast<T*>(ref.ptr());
    }
    
    /**
     * @brief templated creation function, only makes the instance but
     * doesn't add to the manager or group heirarchy
     */
    template <class T>
    SGSharedPtr<T> create()
    {
        auto ref = create(T::staticSubsystemClassId());
        return dynamic_cast<T*>(ref.ptr());
    }
    
    SGSubsystemRef create(const std::string& name);
    
    template <class T>
    SGSharedPtr<T> createInstance(const std::string& subsystemInstanceId)
    {
        auto ref = createInstance(T::staticSubsystemClassId(), subsystemInstanceId);
        return dynamic_cast<T*>(ref.ptr());
    }
    
    SGSubsystemRef createInstance(const std::string& name, const std::string& subsystemInstanceId);

    static GroupType defaultGroupFor(const char* name);
    static double defaultUpdateIntervalFor(const char* name);
    static const DependencyVec& dependsFor(const char* name);
    
    /**
     * @brief delegate to recieve notifications when the subsystem
     * configuration changes. For any event/state change, a before (will)
     * and after (did) change notifications are sent.
     */
    class Delegate
    {
    public:
        virtual void willChange(SGSubsystem* sub, SGSubsystem::State newState);
        virtual void didChange(SGSubsystem* sub, SGSubsystem::State currentState);
    };
    
    void addDelegate(Delegate * d);
    void removeDelegate(Delegate * d);
    
    /**
     * @brief return a particular subsystem manager by name. Passing an
     * empty string retrived the default/global subsystem manager, assuming it
     * has been created.
     */
    static SGSubsystemMgr* getManager(const std::string& id);
private:
    friend class SGSubsystem;
    friend class SGSubsystemGroup;
    
    void notifyDelegatesWillChange(SGSubsystem* sub, State newState);
    void notifyDelegatesDidChange(SGSubsystem* sub, State statee);
    
    std::vector<SGSubsystemGroupRef> _groups;
    unsigned int _initPosition = 0;
    bool _destructorActive = false;
    SGPropertyNode_ptr _rootNode;
    
    // non-owning reference, this is to accelerate lookup
    // by name which otherwise needs a full walk of the entire tree
    using SubsystemDict = std::map<std::string, SGSubsystem*>;
    mutable SubsystemDict _subsystemNameCache;

    using DelegateVec = std::vector<Delegate*>;
    DelegateVec _delegates;
};

#endif // __SUBSYSTEM_MGR_HXX
