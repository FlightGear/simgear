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

#if 0
#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#  include <float.h>
#endif

#include STL_STRING
SG_USING_STD(string);

#include <vector>
SG_USING_STD(vector);
#endif

#include <string>
#include <map>
#include <vector>
SG_USING_STD(map);
SG_USING_STD(vector);
SG_USING_STD(string);

#include <simgear/props/props.hxx>
#include <simgear/timing/timestamp.hxx>


class TimingInfo
{
private:
    string eventName;
    SGTimeStamp time;

public: 
    TimingInfo(string name, SGTimeStamp &t) { eventName = name; time = t;};
    string getName() { return eventName; };
    SGTimeStamp getTime() { return time; };
};

typedef vector<TimingInfo> eventTimeVec;
typedef vector<TimingInfo>::iterator eventTimeVecIterator;



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
class SGSubsystem
{
public:

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
   * Suspend or resum operation of this subsystem.
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

  void printTimingInformation();

  void stamp(string name);


protected:

  bool _suspended;

  eventTimeVec timingInfo;
  //int test;

};



/**
 * A group of FlightGear subsystems.
 */
class SGSubsystemGroup : public SGSubsystem
{
public:

    SGSubsystemGroup ();
    virtual ~SGSubsystemGroup ();

    virtual void init ();
    virtual void postinit ();
    virtual void reinit ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double delta_time_sec);
    virtual void suspend ();
    virtual void resume ();
    virtual bool is_suspended () const;

    virtual void set_subsystem (const string &name,
                                SGSubsystem * subsystem,
                                double min_step_sec = 0);
    virtual SGSubsystem * get_subsystem (const string &name);
    virtual void remove_subsystem (const string &name);
    virtual bool has_subsystem (const string &name) const;


private:

    struct Member {

        Member ();
        Member (const Member &member);
        virtual ~Member ();

        virtual void update (double delta_time_sec);
	void printTimingInformation();

        string name;
        SGSubsystem * subsystem;
        double min_step_sec;
        double elapsed_sec;
    };

    Member * get_member (const string &name, bool create = false);

    vector<Member *> _members;
};



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

    /**
     * Types of subsystem groups.
     */
    enum GroupType {
        INIT = 0,
        GENERAL,
        MAX_GROUPS
    };

    SGSubsystemMgr ();
    virtual ~SGSubsystemMgr ();

    virtual void init ();
    virtual void postinit ();
    virtual void reinit ();
    virtual void bind ();
    virtual void unbind ();
    virtual void update (double delta_time_sec);
    virtual void suspend ();
    virtual void resume ();
    virtual bool is_suspended () const;

    virtual void add (const char * name,
                      SGSubsystem * subsystem,
                      GroupType group = GENERAL, 
                      double min_time_sec = 0);

    virtual SGSubsystemGroup * get_group (GroupType group);

    virtual SGSubsystem * get_subsystem(const string &name);

private:

    SGSubsystemGroup _groups[MAX_GROUPS];
    map<string,SGSubsystem *> _subsystem_map;

};



#endif // __SUBSYSTEM_MGR_HXX

