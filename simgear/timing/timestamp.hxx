/**
 * \file timestamp.hxx
 * Provides a class for managing a timestamp (seconds & milliseconds.)
 */

// Written by Curtis Olson, started December 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _TIMESTAMP_HXX
#define _TIMESTAMP_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include <simgear/compiler.h>


// MSVC++ 6.0 kuldge - Need forward declaration of friends.
class SGTimeStamp;
SGTimeStamp operator + (const SGTimeStamp& t, const long& m);
long operator - (const SGTimeStamp& a, const SGTimeStamp& b);

/**
 * The SGTimeStamp class allows you to mark and compare time stamps
 * with microsecond accuracy (if your system has support for this
 * level of accuracy.)
 *
 * The SGTimeStamp is useful for tracking the elapsed time of various
 * events in your program. You can also use it to keep constistant
 * motion across varying frame rates.
 */

class SGTimeStamp {

private:

    long seconds;
    long usec;

public:

    /** Default constructor */
    SGTimeStamp();

    /**
     * This creates an instance of the SGTimeStamp object. When
     * calling the constructor you may provide initial seconds an
     * microseconds values.
     * @param s initial seconds value
     * @param m initial microseconds value
     */
    SGTimeStamp( const long s, const long m );

    /** Update stored time to current time (seconds and microseconds) */
    void stamp();

    /**
     * Increment the saved time by the specified number of microseconds
     * @param t time stamp
     * @param m microseconds increment
     * @return new time stamp
     */
    friend SGTimeStamp operator + (const SGTimeStamp& t, const long& m);

    /**
     * Subtract two time stamps returning the difference in microseconds.
     * @param a timestamp 1
     * @param b timestame 2
     * @return difference in microseconds
     */
    friend long operator - (const SGTimeStamp& a, const SGTimeStamp& b);

    /** @return the saved seconds of this time stamp */
    inline long get_seconds() const { return seconds; }

    /** @return the saved microseconds of this time stamp */
    inline long get_usec() const { return usec; }
};

inline SGTimeStamp::SGTimeStamp() :
    seconds(0),
    usec(0)
{
}

inline SGTimeStamp::SGTimeStamp( const long s, const long u ) {
    seconds = s;
    usec = u;
}

#endif // _TIMESTAMP_HXX


