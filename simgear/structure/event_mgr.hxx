// eventmMgr.hxx -- periodic event scheduler
//
// Written by Curtis Olson, started December 1997.
// Modified by Bernie Bright, April 2002.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$

#ifndef SG_EVENT_MGR_HXX
#define SG_EVENT_MGR_HXX 1

#include <simgear/compiler.h>

#include <simgear/structure/callback.hxx>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/timing/timestamp.hxx>

#include <vector>
#include <string>

SG_USING_STD(vector);
SG_USING_STD(string);


class SGEvent
{

public:

    typedef int interval_type;

private:

    string _name;
    SGCallback* _callback;
    SGSubsystem* _subsystem;
    interval_type _repeat_value;
    interval_type _initial_value;
    int _ms_to_go;

    unsigned long _cum_time;    // cumulative processor time of this event
    unsigned long _min_time;    // time of quickest execution
    unsigned long _max_time;    // time of slowest execution
    unsigned long _count;       // number of times executed

public:

    /**
     * 
     */
    SGEvent();

    SGEvent( const char* desc,
             SGCallback* cb,
             interval_type repeat_value,
             interval_type initial_value );

    SGEvent( const char* desc,
             SGSubsystem* subsystem,
             interval_type repeat_value,
             interval_type initial_value );

    /**
     * 
     */
    ~SGEvent();

    /**
     * 
     */
    inline void reset()
    {
        _ms_to_go = _repeat_value;
    }

    /**
     * Execute this event's callback.
     */
    void run();

    inline string name() const { return _name; }
    inline interval_type repeat_value() const { return _repeat_value; }
    inline int value() const { return _ms_to_go; }

    /**
     * Display event statistics.
     */
    void print_stats() const;

    /**
     * Update the elapsed time for this event.
     * @param dt_ms elapsed time in milliseconds.
     * @return true if elapsed time has expired.
     */
    inline bool update( int dt_ms )
    {
        return (_ms_to_go -= dt_ms) <= 0;
    }
};


class SGEventMgr : public SGSubsystem
{
private:

    typedef SGEvent::interval_type interval_type;
    typedef vector< SGEvent > event_container_type;

    void add( const SGEvent& event );

    // registered events.
    event_container_type event_table;


public:
    SGEventMgr();
    ~SGEventMgr();

    /**
     * Initialize the scheduling subsystem.
     */
    void init();
    void reinit();
    void bind();
    void unbind();

    /*
     * Update the elapsed time for all events.
     * @param dt elapsed time in seconds.
     */
    void update( double dt );

    /**
     * register a free standing function to be executed some time in the future.
     * @param desc A brief description of this callback for logging.
     * @param cb The callback function to be executed.
     * @param repeat_value repetition rate in milliseconds.
     * @param initial_value initial delay value in milliseconds.  A value of
     * -1 means run immediately.
     */
    template< typename Fun >
    inline void add( const char* name,
                     const Fun& f,
                     interval_type repeat_value,
                     interval_type initial_value = -1 )
    {
        this->add( SGEvent( name,
                            make_callback(f),
                            repeat_value,
                            initial_value ) );
    }

    /**
     * register a subsystem of which the update function will be executed some
     * time in the future.
     * @param desc A brief description of this callback for logging.
     * @param subsystem The subsystem of which the update function will be
     * executed.
     * @param repeat_value repetition rate in milliseconds.
     * @param initial_value initial delay value in milliseconds.  A value of
     * -1 means run immediately.
     */
    inline void add( const char* name,
                     SGSubsystem* subsystem,
                     interval_type repeat_value,
                     interval_type initial_value = -1 )
    {
        this->add( SGEvent( name,
                            subsystem,
                            repeat_value,
                            initial_value ) );
    }

    template< class ObjPtr, typename MemFn >
    inline void add( const char* name,
                     const ObjPtr& p,
                     MemFn pmf,
                     interval_type repeat_value,
                     interval_type initial_value = -1 )
    {
        this->add( SGEvent( name,
                            make_callback(p,pmf),
                            repeat_value,
                            initial_value ) );
    }

    /**
     * Display statistics for all registered events.
     */
    void print_stats() const;
};


#endif //SG_EVENT_MGR_HXX
