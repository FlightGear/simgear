//
// SGEventMgr.cxx -- Event Manager
//
// Written by Bernie Bright, started April 2002.
//
// Copyright (C) 2002  Curtis L. Olson  - curt@me.umn.edu
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <simgear/debug/logstream.hxx>

#include "event_mgr.hxx"

SGEvent::SGEvent()
    : _name(""),
      _callback(0),
      _subsystem(0),
      _repeat_value(0),
      _initial_value(0),
      _ms_to_go(0),
      _cum_time(0),
      _min_time(100000),
      _max_time(0),
      _count(0)
{
}

SGEvent::SGEvent( const char* name,
                  SGCallback* cb,
                  interval_type repeat_value,
                  interval_type initial_value )
    : _name(name),
      _callback(cb),
      _subsystem(NULL),
      _repeat_value(repeat_value),
      _initial_value(initial_value),
      _cum_time(0),
      _min_time(100000),
      _max_time(0),
      _count(0)
{
    if (_initial_value < 0)
    {
        this->run();
        _ms_to_go = _repeat_value;
    }
    else
    {
        _ms_to_go = _initial_value;
    }
}

SGEvent::SGEvent( const char* name,
                  const SGSubsystem* subsystem,
                  interval_type repeat_value,
                  interval_type initial_value )
    : _name(name),
      _callback(NULL),
      _subsystem((SGSubsystem*)&subsystem),
      _repeat_value(repeat_value),
      _initial_value(initial_value),
      _cum_time(0),
      _min_time(100000),
      _max_time(0),
      _count(0)
{
    if (_initial_value < 0)
    {
        this->run();
        _ms_to_go = _repeat_value;
    }
    else
    {
        _ms_to_go = _initial_value;
    }
}


SGEvent::~SGEvent()
{
    //delete callback_;
}

void
SGEvent::run()
{
    SGTimeStamp start_time;
    SGTimeStamp finish_time;

    start_time.stamp();

    // run the event
    if (_callback)
    {
        (*_callback)();
    } else if (_subsystem)
    {
        _subsystem->update(_repeat_value);
    }

    finish_time.stamp();

    ++_count;

    unsigned long duration = finish_time - start_time;

    _cum_time += duration;

    if ( duration < _min_time ) {
        _min_time = duration;
    }

    if ( duration > _max_time ) {
        _max_time = duration;
    }
}

void
SGEvent::print_stats() const
{
    SG_LOG( SG_EVENT, SG_INFO, 
            "  " << _name
            << " int=" << _repeat_value / 1000.0
            << " cum=" << _cum_time
            << " min=" << _min_time
            << " max=" <<  _max_time
            << " count=" << _count
            << " ave=" << _cum_time / (double)_count );
}



SGEventMgr::SGEventMgr()
{
}

SGEventMgr::~SGEventMgr()
{
}

void
SGEventMgr::init()
{
    SG_LOG( SG_EVENT, SG_INFO, "Initializing event manager" );

    event_table.clear();
}

void
SGEventMgr::reinit()
{
}


void
SGEventMgr::bind()
{
}

void
SGEventMgr::unbind()
{
}

void
SGEventMgr::update( double dt )
{
    int dt_ms = int(dt * 1000);

    if (dt_ms < 0)
    {
        SG_LOG( SG_GENERAL, SG_ALERT,
                "SGEventMgr::update() called with negative delta T" );
        return;
    }

    int min_value = 0;
    event_container_type::iterator first = event_table.begin();
    event_container_type::iterator last = event_table.end();
    event_container_type::iterator event = event_table.end();

    // Scan all events.  Run one whose interval has expired.
    while (first != last)
    {
        if (first->update( dt_ms ))
        {
            if (first->value() < min_value)
            {
                // Select event with largest negative value.
                // Its been waiting longest.
                min_value = first->value();
                event = first;
            }
        }
        ++first;
    }

    if (event != last)
    {
        event->run();

        if (event->repeat_value() > 0)
        {
            event->reset();
        }
        else
        {
            SG_LOG( SG_GENERAL, SG_DEBUG, "Deleting event " << event->name() );
            event_table.erase( event );
        }
    }
}

void
SGEventMgr::add( const SGEvent& event )
{
    event_table.push_back( event );

    SG_LOG( SG_EVENT, SG_INFO, "registered event " << event.name()
            << " to run every " << event.repeat_value() << "ms" );
}

void
SGEventMgr::print_stats() const
{
    SG_LOG( SG_EVENT, SG_INFO, "" );
    SG_LOG( SG_EVENT, SG_INFO, "Event Stats" );
    SG_LOG( SG_EVENT, SG_INFO, "-----------" );

    event_container_type::const_iterator first = event_table.begin();
    event_container_type::const_iterator last = event_table.end();
    for (; first != last; ++first)
    {
        first->print_stats();
    }

    SG_LOG( SG_EVENT, SG_INFO, "" );
}
