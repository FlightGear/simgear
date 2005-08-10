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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/compiler.h>

#ifdef SG_HAVE_STD_INCLUDES
#  include <ctime>
#else
#  include <time.h>
#endif

#ifdef HAVE_SYS_TIMEB_H
#  include <sys/timeb.h> // for ftime() and struct timeb
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>    // for gettimeofday()
#endif
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // for get/setitimer, gettimeofday, struct timeval
#endif

// -dw- want to use metrowerks time.h
#ifdef macintosh
#  include <time.h>
#  include <timer.h>
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
    seconds = 0;
    usec =  t * 1000;
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
// -dw- uses time manager
#elif defined( macintosh )
    UnsignedWide ms;
    Microseconds(&ms);
	
    seconds = ms.lo / 1000000;
    usec = ms.lo - ( seconds * 1000000 );
#else
# error Port me
#endif
}

// increment the time stamp by the number of microseconds (usec)
SGTimeStamp operator + (const SGTimeStamp& t, const long& m) {
#if defined( WIN32 )
    return SGTimeStamp( 0, t.usec + m );
#else
    return SGTimeStamp( t.seconds + ( t.usec + m ) / 1000000,
			( t.usec + m ) % 1000000 );
#endif
}

// difference between time stamps in microseconds (usec)
long operator - (const SGTimeStamp& a, const SGTimeStamp& b)
{
#if defined( WIN32 )
    return a.usec - b.usec;
#else
    return 1000000 * (a.seconds - b.seconds) + (a.usec - b.usec);
#endif
}
