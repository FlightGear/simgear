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
#  include <unistd.h>    // for gettimeofday()
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // for get/setitimer, gettimeofday, struct timeval
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
    seconds = t / 1000;
    usec = ( t - ( seconds * 1000 ) ) * 1000;
#elif defined( HAVE_GETTIMEOFDAY )
    struct timeval current;
    struct timezone tz;
    // sg_timestamp currtime;
    gettimeofday(&current, &tz);
    seconds = current.tv_sec;
    usec = current.tv_usec;
#elif defined( HAVE_GETLOCALTIME )
    SYSTEMTIME current;
    GetLocalTime(&current);
    seconds = current.wSecond;
    usec = current.wMilliseconds * 1000;
#elif defined( HAVE_FTIME )
    struct timeb current;
    ftime(&current);
    seconds = current.time;
    usec = current.millitm * 1000;
#else
# error Port me
#endif
}

// increment the time stamp by the number of microseconds (usec)
SGTimeStamp operator + (const SGTimeStamp& t, const long& m) {
    return SGTimeStamp( t.seconds + ( t.usec + m ) / 1000000,
			( t.usec + m ) % 1000000 );
}

// difference between time stamps in microseconds (usec)
long operator - (const SGTimeStamp& a, const SGTimeStamp& b)
{
    return 1000000 * (a.seconds - b.seconds) + (a.usec - b.usec);
}
