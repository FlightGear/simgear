// sg_time.hxx -- data structures and routines for managing time related stuff.
//
// Written by Curtis Olson, started August 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _SG_TIME_HXX
#define _SG_TIME_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef FG_HAVE_STD_INCLUDES
#  include <ctime>
#else
#  include <time.h>
#endif

#include "timezone.h"


// Define a structure containing time parameters
class SGTime {

private:
    // tzContainer stores all the current Timezone control points/
    TimezoneContainer* tzContainer;

    // Points to the current local timezone name;
    char *zonename;

    // Unix "calendar" time in seconds
    time_t cur_time;

    // Break down of equivalent GMT time
    struct tm *gmt;

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

public:

    SGTime( const string& root );
    ~SGTime();

    // Initialize the time related variables
    void init( double lon, double lat, const string& root );

    // Update the time related variables
    void update( double lon, double lat, double alt_m, long int warp );

    // Given lon/lat, update timezone information and local_offset
    void updateLocal( double lon, double lat, const string& root );

    // given Julian Date and Longitude (decimal degrees West) compute
    // Local Sidereal Time, in decimal hours.
    //
    // Provided courtesy of ecdowney@noao.edu (Elwood Downey) 
    double sidereal_precise( double lng );

    // return a courser but cheaper estimate of sidereal time
    double sidereal_course( double lng );

    // Some other stuff which were changed to SGTime members on
    // questionable grounds -:)
    time_t get_gmt(int year, int month, int day, 
		   int hour, int minute, int second);
    time_t get_gmt(struct tm* the_time);

    inline double getJD() const { return jd; };
    inline double getMjd() const { return mjd; };
    inline double getLst() const { return lst; };
    inline double getGst() const { return gst; };
    inline time_t get_cur_time() const { return cur_time; };
    inline struct tm* getGmt()const { return gmt; };
    inline char* get_zonename() const { return zonename; }
};


// this is just a wrapper
inline time_t SGTime::get_gmt(struct tm* the_time) {
    // printf("Using: %24s as input\n", asctime(the_time));
    return get_gmt(the_time->tm_year,
		   the_time->tm_mon,
		   the_time->tm_mday,
		   the_time->tm_hour,
		   the_time->tm_min,
		   the_time->tm_sec);
}

// given a date in months, mn, days, dy, years, yr, return the
// modified Julian date (number of days elapsed since 1900 jan 0.5),
// mjd.  Adapted from Xephem.
double sgTimeCalcMJD(int mn, double dy, int yr);

// given an mjd, calculate greenwich mean sidereal time, gst
double sgTimeCalcGST( double mjd );

// format time
char* sgTimeFormatTime( const struct tm* p, char* buf );


#endif // _SG_TIME_HXX
