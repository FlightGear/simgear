/**
 * \file sg_time.hxx
 * Data structures and routines for managing time related values.
 */

// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _SG_TIME_HXX
#define _SG_TIME_HXX


#ifndef __cplusplus
# error This library requires C++
#endif


#include <simgear/compiler.h>

#include <ctime>
#include <memory> // for std::auto_ptr
#include <string>

// forward decls
class SGPath;
class SGGeod;
 
/**
 * A class to calculate and manage a variety of time parameters.
 * The SGTime class provides many real-world time values. It
 * calculates current time in seconds, GMT time, local time zone,
 * local offset in seconds from GMT, Julian date, and sidereal
 * time. All of these operate with seconds as their granularity so
 * this class is not intended for timing sub-second events. These
 * values are intended as input to things like real world lighting
 * calculations and real astronomical object placement.

 * To properly use the SGTime class there are a couple of things to be
 * aware of. After creating an instance of the class, you will need to
 * periodically (i.e. before every frame) call the update()
 * method. Optionally, if you care about updating time zone
 * information based on your latitude and longitude, you can call the
 * updateLocal() method periodically as your position changes by
 * significant amounts.

 */

class SGTime {

private:

    // Points to the current local timezone name;
    std::string zonename;

    // Unix "calendar" time in seconds
    time_t cur_time;

    // Break down of equivalent GMT time
    struct tm m_gmt;    // copy of system gmtime(&time_t) structure

    // offset of local time relative to GMT
    time_t local_offset;

    // Julian date
    double jd;

    // modified Julian date
    double mjd;

    // side real time at prime meridian
    double gst;

    // local sidereal time
    double lst;

    // the difference between the precise / expensive sidereal time
    // algorithm result and the quick course result.  course_gst +
    // gst_diff has pretty good accuracy over the span of a couple hours
    double gst_diff;

    /** init common constructor code */
    void init( const SGGeod& location, const SGPath& root,
               time_t init_time );

public:

    /** Default constructor */
    SGTime();

    /**
     * Create an instance based on a specified position and data file path.
     * This creates an instance of the SGTime object. When calling the
     * constructor you need to provide a root path pointing to your
     * time zone definition tree. Optionally, you can call a form of
     * the constructor that accepts your current longitude and
     * latitude in radians.
     *
     * If you don't know your position when you call the SGTime
     * constructor, you can just use the first form (which assumes 0,
     * 0).
     *
     * @param location  Current geodetic location
     * @param root      Root path point to data file location (timezone, etc.)
     * @param init_time Provide an initialization time, 0 means use current
     *                  clock time
     */
    SGTime( const SGGeod& location,
            const SGPath& root,
            time_t init_time );

    /**
     * Create an instance given a data file path.
     * @param root root path point to data file location (timezone, etc.)
     */
    SGTime( const SGPath& root );

    /** Destructor */
    ~SGTime();

    /** 
     * Update the time related variables.
     * The update() method requires you to pass in your position and
     * an optional time offset in seconds. The offset (or warp) allows
     * you to offset "sim" time relative to "real" time. The update()
     * method is designed to be called by the host application before
     * every frame.
     *
     * @param location  Current geodetic location
     * @param ct        Specify a unix time, otherwise specify 0 to use current
     *                  clock time
     * @param warp      Optional time offset in seconds. This allows to advance
     *                  or rewind "time".
     */
    void update( const SGGeod& location, time_t ct, long int warp );

    /** Deprecated method. To be removed after the next release... */
    void update( double lon_rad, double lat_rad, time_t ct, long int warp ) DEPRECATED;

    /**
     * Given lon/lat, update timezone information and local_offset
     * The updateLocal() method is intended to be called less
     * frequently - only when your position is likely to be changed
     * enough that your timezone may have changed as well. In the
     * FlightGear project we call updateLocal() every few minutes from
     * our periodic event manager.
     *
     * @param location  Current geodetic location
     * @param root      Bbase path containing time zone directory
     */
    void updateLocal( const SGGeod& location, const std::string& root );

    /** @return current system/unix time in seconds */
    inline time_t get_cur_time() const { return cur_time; };

    /** @return time zone name for your current position*/
    inline const char * get_zonename() const { return zonename.c_str(); }

    /** @return GMT in a "brokent down" tm structure */
    inline struct tm* getGmt()const { return (struct tm *)&m_gmt; };

    /** @return julian date */
    inline double getJD() const { return jd; };

    /** @return modified julian date */
    inline double getMjd() const { return mjd; };

    /** @return local side real time */
    inline double getLst() const { return lst; };

    /** @return grenich side real time (lst when longitude == 0) */
    inline double getGst() const { return gst; };

    /** @return offset in seconds to local timezone time */
    inline time_t get_local_offset() const { return local_offset; };
};


// Some useful utility functions that don't make sense to be part of
// the SGTime class

/**
 * \relates SGTime
 * Return unix time in seconds for the given date (relative to GMT)
 * @param year current GMT year
 * @param month current GMT month
 * @param day current GMT day
 * @param hour current GMT hour
 * @param minute current minute
 * @param second current second
 * @return unix/system time in seconds
 */
time_t sgTimeGetGMT(int year, int month, int day, 
		    int hour, int minute, int second);

/**
 * \relates SGTime
 * this is just a wrapper for sgTimeGetGMT that allows an alternate
 * form of input parameters.
 * @param the_time the current GMT time in the tm structure
 * @return unix/system time in seconds
 */
inline time_t sgTimeGetGMT(struct tm* the_time) {
    // printf("Using: %24s as input\n", asctime(the_time));
    return sgTimeGetGMT(the_time->tm_year,
			the_time->tm_mon,
			the_time->tm_mday,
			the_time->tm_hour,
			the_time->tm_min,
			the_time->tm_sec);
}

/**
 * \relates SGTime
 * Given a date in our form, return the equivalent modified Julian
 * date (number of days elapsed since 1900 jan 0.5), mjd.  Adapted
 * from Xephem.
 * @param mn month
 * @param dy day
 * @param yr year
 * @return modified julian date */
double sgTimeCalcMJD(int mn, double dy, int yr);

/**
 * \relates SGTime
 * Given an optional offset from current time calculate the current
 * modified julian date.
 * @param ct specify a unix time, otherwise specify 0 to use current
          clock time
 * @param warp number of seconds to offset from current time (0 if no offset)
 * @return current modified Julian date (number of days elapsed
 * since 1900 jan 0.5), mjd. */
double sgTimeCurrentMJD( time_t ct /* = 0 */, long int warp /* = 0 */ );

/**
 * \relates SGTime
 * Given an mjd, calculate greenwich mean sidereal time, gst
 * @param mjd modified julian date
 * @return greenwich mean sidereal time (gst) */
double sgTimeCalcGST( double mjd );

/**
 * \relates SGTime
 * Format time in a pretty form
 * @param p time specified in a tm struct
 * @param buf buffer space to contain the result
 * @return pointer to character array containt the result
 */
char* sgTimeFormatTime( const struct tm* p, char* buf );


#endif // _SG_TIME_HXX
