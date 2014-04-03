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
#include <cerrno>

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

#if defined(_POSIX_TIMERS) && (0 < _POSIX_TIMERS)
static clockid_t getClockId()
{
#if defined(_POSIX_MONOTONIC_CLOCK)
    static clockid_t clockid = CLOCK_MONOTONIC;
    static bool firstTime = true;
    if (!firstTime)
        return clockid;

    firstTime = false;
    // For the first time test if the monotonic clock is available.
    // If so use this, if not use the realtime clock.
    struct timespec ts;
    if (-1 == clock_gettime(clockid, &ts) && errno == EINVAL)
        clockid = CLOCK_REALTIME;
    return clockid;
#else
    return CLOCK_REALTIME;
#endif
}

#endif

void SGTimeStamp::stamp() {
#ifdef _WIN32
    unsigned int t;
    t = timeGetTime();
    _sec = t / 1000;
    _nsec = ( t - ( _sec * 1000 ) ) * 1000 * 1000;
#elif defined(_POSIX_TIMERS) && (0 < _POSIX_TIMERS)
    struct timespec ts;
    clock_gettime(getClockId(), &ts);
    _sec = ts.tv_sec;
    _nsec = ts.tv_nsec;
#elif defined( HAVE_GETTIMEOFDAY )
    struct timeval current;
    gettimeofday(&current, NULL);
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

// sleep based timing loop.
//
// Calling sleep, even usleep() on linux is less accurate than
// we like, but it does free up the cpu for other tasks during
// the sleep so it is desirable.  Because of the way sleep()
// is implemented in consumer operating systems like windows
// and linux, you almost always sleep a little longer than the
// requested amount.
//
// To combat the problem of sleeping too long, we calculate the
// desired wait time and shorten it by 2000us (2ms) to avoid
// [hopefully] over-sleep'ing.  The 2ms value was arrived at
// via experimentation.  We follow this up at the end with a
// simple busy-wait loop to get the final pause timing exactly
// right.
//
// Assuming we don't oversleep by more than 2000us, this
// should be a reasonable compromise between sleep based
// waiting, and busy waiting.
//
// Usually posix timer resolutions are low enough that we
// could just leave this to the operating system today.
// The day where the busy loop was introduced in flightgear,
// the usual kernels still had just about 10ms (=HZ for
// the timer tick) accuracy which is too bad to catch 60Hz...
bool SGTimeStamp::sleepUntil(const SGTimeStamp& abstime)
{
    // FreeBSD is missing clock_nanosleep, see
    // https://wiki.freebsd.org/FreeBSD_and_Standards
#if defined(_POSIX_TIMERS) && (0 < _POSIX_TIMERS) && !defined(__FreeBSD__)
    SGTimeStamp abstimeForSleep = abstime;

    // Always undersleep by resolution of the clock
    struct timespec ts;
    if (-1 != clock_getres(getClockId(), &ts)) {
        abstimeForSleep -= SGTimeStamp::fromSecNSec(ts.tv_sec, ts.tv_nsec);
    } else {
        abstimeForSleep -= SGTimeStamp::fromSecMSec(0, 2);
    }

    ts.tv_sec = abstimeForSleep._sec;
    ts.tv_nsec = abstimeForSleep._nsec;
    for (;;) {
        int ret = clock_nanosleep(getClockId(), TIMER_ABSTIME, &ts, NULL);
        if (-1 == ret && errno != EINTR)
            return false;
        if (ret == 0)
            break;
    }

    // The busy loop for the rest
    SGTimeStamp currentTime;
    do {
        currentTime.stamp();
    } while (currentTime < abstime);

    return true;

#elif defined _WIN32

    SGTimeStamp currentTime;
    currentTime.stamp();
    if (abstime <= currentTime)
        return true;

    SGTimeStamp abstimeForSleep = abstime - SGTimeStamp::fromSecMSec(0, 2);
    for (;abstimeForSleep < currentTime;) {
        SGTimeStamp timeDiff = abstimeForSleep - currentTime;
        if (timeDiff < SGTimeStamp::fromSecMSec(0, 1))
            break;
        // Don't know, but may be win32 has something better today??
        Sleep(static_cast<DWORD>(timeDiff.toMSecs()));

        currentTime.stamp();
    }

    // Follow by a busy loop
    while (currentTime < abstime) {
        currentTime.stamp();
    }

    return true;

#else

    SGTimeStamp currentTime;
    currentTime.stamp();
    if (abstime <= currentTime)
        return true;

    SGTimeStamp abstimeForSleep = abstime - SGTimeStamp::fromSecMSec(0, 2);
    for (;abstimeForSleep < currentTime;) {
        SGTimeStamp timeDiff = abstimeForSleep - currentTime;
        if (timeDiff < SGTimeStamp::fromSecUSec(0, 1))
            break;
        // Its documented that some systems bail out on usleep for longer than 1s
        // since we recheck the current time anyway just wait for
        // less than a second multiple times
        bool truncated = false;
        if (SGTimeStamp::fromSec(1) < timeDiff) {
            timeDiff = SGTimeStamp::fromSec(1);
            truncated = true;
        }
        int ret = usleep(static_cast<int>(timeDiff.toUSecs()));
        if (-1 == ret && errno != EINTR)
            return false;
        if (ret == 0 && !truncated)
            break;

        currentTime.stamp();
    }

    // Follow by a busy loop
    while (currentTime < abstime) {
        currentTime.stamp();
    }

    return true;

#endif
}

bool SGTimeStamp::sleepFor(const SGTimeStamp& reltime)
{
    // see comment above regarding FreeBSD
#if defined(_POSIX_TIMERS) && (0 < _POSIX_TIMERS) && !defined(__FreeBSD__)
    struct timespec ts;
    ts.tv_sec = reltime._sec;
    ts.tv_nsec = reltime._nsec;
    for (;;) {
        struct timespec rem;
        int ret = clock_nanosleep(getClockId(), 0, &ts, &rem);
        if (-1 == ret && errno != EINTR)
            return false;
        if (ret == 0)
            break;
        // Use the remainder for the next cycle.
        ts = rem;
    }
    return true;
#elif defined _WIN32
    if (reltime < SGTimeStamp::fromSecMSec(0, 1))
        return true;
    // Don't know, but may be win32 has something better today??
    Sleep(static_cast<DWORD>(reltime.toMSecs()));
    return true;
#elif defined __APPLE__
    // the generic version below behaves badly on Mac; use nanosleep directly,
    // similar to the POSIX timers version
    struct timespec ts;
    ts.tv_sec = reltime._sec;
    ts.tv_nsec = reltime._nsec;
    
    for (;;) {
        struct timespec rem;
        int ret = nanosleep(&ts, &rem);
        if (-1 == ret && errno != EINTR)
            return false;
        if (ret == 0)
            break;
        // Use the remainder for the next cycle.
        ts = rem;
    }
    return true;
#else
    SGTimeStamp abstime;
    abstime.stamp();
    abstime += reltime;

    SGTimeStamp currentTime;
    currentTime.stamp();
    for (;abstime < currentTime;) {
        SGTimeStamp timeDiff = abstime - currentTime;
        if (timeDiff < SGTimeStamp::fromSecUSec(0, 1))
            break;
        // Its documented that some systems bail out on usleep for longer than 1s
        // since we recheck the current time anyway just wait for
        // less than a second multiple times
        bool truncated = false;
        if (SGTimeStamp::fromSec(1) < timeDiff) {
            timeDiff = SGTimeStamp::fromSec(1);
            truncated = true;
        }
        int ret = usleep(static_cast<int>(timeDiff.toUSecs()));
        if (-1 == ret && errno != EINTR)
            return false;
        if (ret == 0 && !truncated)
            break;

        currentTime.stamp();
    }

    return true;
#endif
}

int SGTimeStamp::elapsedMSec() const
{
    SGTimeStamp now;
    now.stamp();
    
    return static_cast<int>((now - *this).toMSecs());
}
