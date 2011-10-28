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

#include <iosfwd>
#include <iomanip>
#include <sstream>
#include <simgear/compiler.h>
#include <simgear/math/SGCMath.hxx>

/**
 * The SGTimeStamp class allows you to mark and compare time stamps
 * with nanosecond accuracy (if your system has support for this
 * level of accuracy).
 *
 * The SGTimeStamp is useful for tracking the elapsed time of various
 * events in your program. You can also use it to keep consistent
 * motion across varying frame rates.
 *
 * Note SGTimestamp does not deliver the time of day. The content of this
 * stamps might be, dependent on the implementation, a time to an arbitrary
 * base time.
 */

class SGTimeStamp {
public:
    typedef long sec_type;
    typedef int nsec_type;

    /** Default constructor, initialize time to zero. */
    SGTimeStamp() :
      _nsec(0),
      _sec(0)
    { }

    /** Hmm, might reenable them at some time, but since it is not clear
        what the input unit of the int is, omit them for now.
        Use the static constructor functions where it is clear from the method
        name how the arguments are meant.
     */
//     SGTimeStamp(sec_type sec)
//     { setTime(sec, 0); }
//     SGTimeStamp(int sec)
//     { setTime(sec, 0); }
//     SGTimeStamp(const double& sec)
//     { setTime(sec); }

    /** Update stored time to current time (seconds and nanoseconds) */
    void stamp();

    /** Set the time from a double value */
    void setTime(const double& seconds)
    {
        sec_type wholeSecs = sec_type(floor(seconds));
        nsec_type reminder;
        reminder = nsec_type(floor((seconds - wholeSecs)*(1000*1000*1000)));
        setTime(wholeSecs, reminder);
    }
    /** Set the time from a seconds/nanoseconds pair */
    void setTime(sec_type sec, nsec_type nsec)
    {
        if (0 <= nsec) {
            _sec = sec + nsec / (1000*1000*1000);
            _nsec = nsec % (1000*1000*1000);
        } else {
            _sec = sec - 1 + nsec / (1000*1000*1000);
            _nsec = (1000*1000*1000) + nsec % (1000*1000*1000);
        }
    }

    
    /** @return the saved seconds of this time stamp */
    long get_seconds() const { return _sec; }

    /** @return the saved microseconds of this time stamp */
    int get_usec() const { return _nsec/1000; }

    /** @return the saved seconds of this time stamp */
    const sec_type& getSeconds() const
    { return _sec; }
    /** @return the saved nanoseconds of this time stamp */
    const nsec_type& getNanoSeconds() const
    { return _nsec; }

    /** @return the value of the timestamp in nanoseconds,
     *  use doubles to avoid overflow.
     *  If you need real nanosecond accuracy for time differences, build up a
     *  SGTimeStamp reference time and compare SGTimeStamps directly.
     */
    double toNSecs() const
    { return _nsec + double(_sec)*1000*1000*1000; }

    /** @return the value of the timestamp in microseconds,
     *  use doubles to avoid overflow.
     *  If you need real nanosecond accuracy for time differences, build up a
     *  SGTimeStamp reference time and compare SGTimeStamps directly.
     */
    double toUSecs() const
    { return 1e-3*_nsec + double(_sec)*1000*1000; }

    /** @return the value of the timestamp in milliseconds,
     *  use doubles to avoid overflow.
     *  If you need real nanosecond accuracy for time differences, build up a
     *  SGTimeStamp reference time and compare SGTimeStamps directly.
     */
    double toMSecs() const
    { return 1e-6*_nsec + double(_sec)*1000; }

    /** @return the value of the timestamp in seconds,
     *  use doubles to avoid overflow.
     *  If you need real nanosecond accuracy for time differences, build up a
     *  SGTimeStamp reference time and compare SGTimeStamps directly.
     */
    double toSecs() const
    { return 1e-9*_nsec + _sec; }

    /** Inplace addition.
     */
    SGTimeStamp& operator+=(const SGTimeStamp& c)
    {
        _sec += c._sec;
        _nsec += c._nsec;
        if ((1000*1000*1000) <= _nsec) {
            _nsec -= (1000*1000*1000);
            _sec += 1;
        }
        return *this;
    }

    /** Inplace subtraction.
     */
    SGTimeStamp& operator-=(const SGTimeStamp& c)
    {
        _sec -= c._sec;
        _nsec -= c._nsec;
        if (_nsec < 0) {
            _nsec += (1000*1000*1000);
            _sec -= 1;
        }
        return *this;
    }

    /**
     *  Create SGTimeStamps from input with given units 
     */
    static SGTimeStamp fromSecMSec(sec_type sec, nsec_type msec)
    { return SGTimeStamp(sec, 1000*1000*msec); }
    static SGTimeStamp fromSecUSec(sec_type sec, nsec_type usec)
    { return SGTimeStamp(sec, 1000*usec); }
    static SGTimeStamp fromSecNSec(sec_type sec, nsec_type nsec)
    { return SGTimeStamp(sec, nsec); }

    static SGTimeStamp fromSec(int sec)
    { SGTimeStamp ts; ts.setTime(sec); return ts; }
    static SGTimeStamp fromSec(const double& sec)
    { SGTimeStamp ts; ts.setTime(sec); return ts; }
    static SGTimeStamp fromMSec(nsec_type msec)
    { return SGTimeStamp(0, 1000*1000*msec); }
    static SGTimeStamp fromUSec(nsec_type usec)
    { return SGTimeStamp(0, 1000*usec); }
    static SGTimeStamp fromNSec(nsec_type nsec)
    { return SGTimeStamp(0, nsec); }

    /**
     *  Return a timestamp with the current time.
     */
    static SGTimeStamp now()
    { SGTimeStamp ts; ts.stamp(); return ts; }

    /**
     *  Sleep until the time of abstime is passed.
     */
    static bool sleepUntil(const SGTimeStamp& abstime);

    /**
     *  Sleep for reltime.
     */
    static bool sleepFor(const SGTimeStamp& reltime);

    /**
     *  Alias for the most common use case with milliseconds.
     */
    static bool sleepForMSec(unsigned msec)
    { return sleepFor(fromMSec(msec)); }

    /**
     * elapsed time since the stamp was taken, in msec
     */
    int elapsedMSec() const;
private:
    SGTimeStamp(sec_type sec, nsec_type nsec)
    { setTime(sec, nsec); }

    nsec_type _nsec;
    sec_type _sec;
};

inline bool
operator==(const SGTimeStamp& c1, const SGTimeStamp& c2)
{
  if (c1.getNanoSeconds() != c2.getNanoSeconds())
    return false;
  return c1.getSeconds() == c2.getSeconds();
}

inline bool
operator!=(const SGTimeStamp& c1, const SGTimeStamp& c2)
{ return !operator==(c1, c2); }

inline bool
operator<(const SGTimeStamp& c1, const SGTimeStamp& c2)
{
  if (c1.getSeconds() < c2.getSeconds())
    return true;
  if (c1.getSeconds() > c2.getSeconds())
    return false;
  return c1.getNanoSeconds() < c2.getNanoSeconds();
}

inline bool
operator>(const SGTimeStamp& c1, const SGTimeStamp& c2)
{ return c2 < c1; }

inline bool
operator>=(const SGTimeStamp& c1, const SGTimeStamp& c2)
{ return !(c1 < c2); }

inline bool
operator<=(const SGTimeStamp& c1, const SGTimeStamp& c2)
{ return !(c1 > c2); }

inline SGTimeStamp
operator+(const SGTimeStamp& c1)
{ return c1; }

inline SGTimeStamp
operator-(const SGTimeStamp& c1)
{ return SGTimeStamp::fromSec(0) -= c1; }

inline SGTimeStamp
operator+(const SGTimeStamp& c1, const SGTimeStamp& c2)
{ return SGTimeStamp(c1) += c2; }

inline SGTimeStamp
operator-(const SGTimeStamp& c1, const SGTimeStamp& c2)
{ return SGTimeStamp(c1) -= c2; }

template<typename char_type, typename traits_type> 
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& os, const SGTimeStamp& c)
{
  std::basic_stringstream<char_type, traits_type> stream;

  SGTimeStamp pos = c;
  if (c.getSeconds() < 0) {
    stream << stream.widen('-');
    pos = - c;
  }
  stream << pos.getSeconds() << stream.widen('.');
  stream << std::setw(9) << std::setfill('0') << pos.getNanoSeconds();

  return os << stream.str();
}

#endif // _TIMESTAMP_HXX


