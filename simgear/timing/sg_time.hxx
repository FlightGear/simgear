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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <GL/glut.h>

#include <simgear/compiler.h>

#ifdef FG_HAVE_STD_INCLUDES
#  include <ctime>
#else
#  include <time.h>
#endif

// #include <FDM/flight.hxx>

#include "timezone.h"
// #include "lowleveltime.h"


enum sgTimingOffsetType {
    SG_TIME_SYS_OFFSET   = 0,
    SG_TIME_GMT_OFFSET   = 1,
    SG_TIME_LAT_OFFSET   = 2,
    SG_TIME_SYS_ABSOLUTE = 3,
    SG_TIME_GMT_ABSOLUTE = 4,
    SG_TIME_LAT_ABSOLUTE = 5
};


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

    // the difference between the precise sidereal time algorithm
    // result and the course result.  course_gst + diff has good
    // accuracy for the short term
    double gst_diff;

    // internal book keeping data
    double last_mjd, last_dy;
    int last_mn, last_yr;

public:

    SGTime( const string& root );
    ~SGTime();

    inline double getJD() const { return jd; };
    inline double getMjd() const { return mjd; };
    inline double getLst() const { return lst; };
    inline double getGst() const { return gst; };
    inline time_t get_cur_time() const { return cur_time; };
    inline struct tm* getGmt()const { return gmt; };
  
    // void adjust_warp(int val) { warp += val; };
    // void adjust_warp_delta(int val) { warp_delta += val; };

    // Initialize the time dependent variables
    void init( double lon, double lat, const string& root );
    // time_t timeOffset, sgTimingOffsetType offsetType );

    // Update the time dependent variables
    void update( double lon, double lat, double alt_m, long int warp );
    void updateLocal( double lon, double lat, const string& root );

    void cal_mjd (int mn, double dy, int yr);
    void utc_gst(); 
    double sidereal_precise (double lng);
    double sidereal_course(double lng); 
    // static SGTime *cur_time_params;

    // Some other stuff which were changed to SGTime members on
    // questionable grounds -:)
    // time_t get_start_gmt(int year);
    time_t get_gmt(int year, int month, int day, 
		   int hour, int minute, int second);
    time_t get_gmt(struct tm* the_time);

    inline char* get_zonename() const { return zonename; }

    char* format_time( const struct tm* p, char* buf );
    long int fix_up_timezone( long int timezone_orig );

    // inline int get_warp_delta() const { return warp_delta; }
};


inline time_t SGTime::get_gmt(struct tm* the_time) // this is just a wrapper
{
  //printf("Using: %24s as input\n", asctime(the_time));
  return get_gmt(the_time->tm_year,
	  the_time->tm_mon,
	  the_time->tm_mday,
	  the_time->tm_hour,
	  the_time->tm_min,
	  the_time->tm_sec);
}


#endif // _SG_TIME_HXX
