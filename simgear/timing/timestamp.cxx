/**
 * \file timestamp.cxx
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


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <ctime>

#ifdef HAVE_SYS_TIMEB_H
#  include <sys/timeb.h> // for ftime() and struct timeb
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>    // for gettimeofday() and the _POSIX_TIMERS define
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // for get/setitimer, gettimeofday, struct timeval
#endif

#if defined(_POSIX_TIMERS) && (0 < _POSIX_TIMERS)
#  include <time.h>
#  include <errno.h>
#endif

#ifdef WIN32
#  include <windows.h>
#  if defined( __CYGWIN__ ) || defined( __CYGWIN32__ )
#    define NEAR /* */
#    define FAR  /* */
#  endif
#  include <mmsystem.h>
#endif

#include "timestamp.hxx"

void SGTimeStamp::stamp() {
#if defined( WIN32 ) && !defined(__CYGWIN__)
    unsigned int t;
    t = timeGetTime();
    _sec = t / 1000;
    _nsec = ( t - ( seconds * 1000 ) ) * 1000 * 1000;
#elif defined(_POSIX_TIMERS) && (0 < _POSIX_TIMERS)
    struct timespec ts;
#if defined(_POSIX_MONOTONIC_CLOCK)
    static clockid_t clockid = CLOCK_MONOTONIC;
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;
        // For the first time test if the monotonic clock is available.
        // If so use this if not use the realtime clock.
        if (-1 == clock_gettime(clockid, &ts) && errno == EINVAL)
            clockid = CLOCK_REALTIME;
    }
    clock_gettime(clockid, &ts);
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
    _sec = ts.tv_sec;
    _nsec = ts.tv_nsec;
#elif defined( HAVE_GETTIMEOFDAY )
    struct timeval current;
    struct timezone tz;
    // sg_timestamp currtime;
    gettimeofday(&current, &tz);
    _sec = current.tv_sec;
    _nsec = current.tv_usec * 1000;
#elif defined( HAVE_GETLOCALTIME )
    SYSTEMTIME current;
    GetLocalTime(&current);
    _sec = current.wSecond;
    _nsec = current.wMilliseconds * 1000 * 1000;
#elif defined( HAVE_FTIME )
    struct timeb current;
    ftime(&current);
    _sec = current.time;
    _nsec = current.millitm * 1000 * 1000;
#else
# error Port me
#endif
}

