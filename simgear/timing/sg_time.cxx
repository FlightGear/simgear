// sg_time.cxx -- data structures and routines for managing time related stuff.
//
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


#include <simgear_config.h>
#include <simgear/compiler.h>

#include <errno.h>		// for errno

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

#include <string>

#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>  // for get/setitimer, gettimeofday, struct timeval
#endif
#ifdef HAVE_UNISTD_H
#  include <unistd.h>    // for gettimeofday()
#endif

#include <math.h>        // for NAN

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include "sg_time.hxx"
#include "timezone.h"
#include "lowleveltime.h"

#define DEGHR(x)        ((x)/15.)
#define RADHR(x)        DEGHR(x*SGD_RADIANS_TO_DEGREES)

using std::string;

static const double MJD0    = 2415020.0;
static const double J2000   = 2451545.0 - MJD0;
static const double SIDRATE = 0.9972695677;

// tzContainer stores all the current Timezone control points/
std::unique_ptr<SGTimeZoneContainer> static_tzContainer;

void SGTime::init( const SGGeod& location, const SGPath& root, time_t init_time )
{
    gst_diff = -9999.0;

    if ( init_time ) {
	cur_time = init_time;
    } else {
	cur_time = time(NULL); 
    }

    char* gmt = asctime(gmtime(&cur_time));
    char* local = asctime(localtime(&cur_time));
    gmt[strlen(gmt) - 1] = '\0';
    local[strlen(local) - 1] = '\0';
    SG_LOG( SG_EVENT, SG_DEBUG,
                "Current greenwich mean time = " << gmt);
    SG_LOG( SG_EVENT, SG_DEBUG,
             "Current local time          = " << local);

    if ( !root.isNull()) {
        if (!static_tzContainer.get()) {
            SGPath zone( root );
            zone.append( "zone.tab" );
            SG_LOG( SG_EVENT, SG_INFO, "Reading timezone info from: " << zone );
            std::string zs = zone.utf8Str();
            static_tzContainer.reset(new SGTimeZoneContainer( zs.c_str() ));
        }

        SGTimeZone* nearestTz = static_tzContainer->getNearest(location);

        SGPath name( root );
        name.append( nearestTz->getDescription() );
        zonename = name.utf8Str();
        SG_LOG( SG_EVENT, SG_DEBUG, "Using zonename = " << zonename );
    } else {
        zonename.erase();
    }
}

SGTime::SGTime( const SGGeod& location, const SGPath& root,
           time_t init_time )
{
    init(location, root, init_time);
}

SGTime::SGTime( const SGPath& root ) {
    init( SGGeod(), root, 0 );
}


SGTime::SGTime() {
    init( SGGeod(), SGPath(), 0 );
}


SGTime::~SGTime()
{
}


// given Julian Date and Longitude (decimal degrees West) compute
// Local Sidereal Time, in decimal hours.
//
// Provided courtesy of ecdowney@noao.edu (Elwood Downey) 
static double sidereal_precise( double mjd, double lng )
{
    /* printf ("Current Lst on JD %13.5f at %8.4f degrees West: ", 
       mjd + MJD0, lng); */

    // convert to required internal units
    lng *= SGD_DEGREES_TO_RADIANS;

    // compute LST and print
    double gst = sgTimeCalcGST( mjd );
    double lst = gst - RADHR( lng );
    lst -= 24.0 * floor( lst / 24.0 );
    // printf ("%7.4f\n", lstTmp);

    return lst;
}


// return a courser but cheaper estimate of sidereal time
static double sidereal_course( time_t cur_time, const struct tm *gmt, double lng )
{
    time_t start_gmt, now;
    double diff, part, days, hours, lstTmp;
  
    now = cur_time;
    start_gmt = sgTimeGetGMT(gmt->tm_year, 2, 21, 12, 0, 0);
  
    diff = (now - start_gmt) / (3600.0 * 24.0);
  
    part = fmod(diff, 1.0);
    days = diff - part;
    hours = gmt->tm_hour + gmt->tm_min/60.0 + gmt->tm_sec/3600.0;
  
    lstTmp = (days - lng)/15.0 + hours - 12;
  
    while ( lstTmp < 0.0 ) {
	lstTmp += 24.0;
    }
    
    return lstTmp;
}

// Update the time related variables
void SGTime::update( const SGGeod& location, time_t ct, long int warp )
{
    double gst_precise, gst_course;


    tm * gmt = &m_gmt;


    // get current Unix calendar time (in seconds)
    // warp += warp_delta;
    if ( ct ) {
	cur_time = ct + warp;
    } else {
	cur_time = time(NULL) + warp;
    }

    // get GMT break down for current time

    memcpy( gmt, gmtime(&cur_time), sizeof(tm) );
    // calculate modified Julian date starting with current
    mjd = sgTimeCurrentMJD( ct, warp );

    // add in partial day
    mjd += (gmt->tm_hour / 24.0) + (gmt->tm_min / (24.0 * 60.0)) +
	(gmt->tm_sec / (24.0 * 60.0 * 60.0));

    // convert "back" to Julian date + partial day (as a fraction of one)
    jd = mjd + MJD0;

    // printf("  Current Longitude = %.3f\n", FG_Longitude * SGD_RADIANS_TO_DEGREES);

    // Calculate local side real time
    if ( gst_diff < -100.0 ) {
	// first time through do the expensive calculation & cheap
        // calculation to get the difference.
	SG_LOG( SG_EVENT, SG_DEBUG, "  First time, doing precise gst" );
	gst_precise = gst = sidereal_precise( mjd, 0.00 );
	gst_course = sidereal_course( cur_time, gmt, 0.00 );
      
	gst_diff = gst_precise - gst_course;

	lst = sidereal_course( cur_time, gmt,
                               -location.getLongitudeDeg() ) + gst_diff;
    } else {
	// course + difference should drift off very slowly
	gst = sidereal_course( cur_time, gmt, 0.00 ) + gst_diff;
	lst = sidereal_course( cur_time, gmt,
                               -location.getLongitudeDeg()  ) + gst_diff;
    }
}


// Given lon/lat, update timezone information and local_offset
void SGTime::updateLocal( const SGGeod& aLocation, const SGPath& root ) {
  SGGeod location(aLocation);
    if (!aLocation.isValid()) {
        location = SGGeod();
    }
    
    time_t currGMT;
    time_t aircraftLocalTime;
    SGTimeZone* nearestTz = static_tzContainer->getNearest(location);
    SGPath zone( root );
    zone.append ( nearestTz->getDescription() );
    zonename = zone.utf8Str();

    //Avoid troubles when zone.tab hasn't got the right line endings
    if (zonename[zonename.size()-1] == '\r')
    {
      zonename[zonename.size()-1]=0;
      zone = SGPath(zonename);
    }

    currGMT = sgTimeGetGMT( gmtime(&cur_time) );
    std::string zs = zone.utf8Str();
    aircraftLocalTime = sgTimeGetGMT( (fgLocaltime(&cur_time, zs.c_str())) );
    local_offset = aircraftLocalTime - currGMT;
    // cout << "Using " << local_offset << " as local time offset Timezone is " 
    //      << zonename << endl;
}


// given a date in months, mn, days, dy, years, yr, return the
// modified Julian date (number of days elapsed since 1900 jan 0.5),
// mjd.  Adapted from Xephem.
double sgTimeCalcMJD(int mn, double dy, int yr) {
    double mjd;

    // internal book keeping data
    static double last_mjd, last_dy;
    static int last_mn, last_yr;

    int b, d, m, y;
    long c;
  
    if (mn == last_mn && yr == last_yr && dy == last_dy) {
	mjd = last_mjd;
    }
  
    m = mn;
    y = (yr < 0) ? yr + 1 : yr;
    if (mn < 3) {
	m += 12;
	y -= 1;
    }
  
    if (yr < 1582 || (yr == 1582 && (mn < 10 || (mn == 10 && dy < 15)))) {
	b = 0;
    } else {
	int a;
	a = y/100;
	b = 2 - a + a/4;
    }
  
    if (y < 0) {
	c = (long)((365.25*y) - 0.75) - 694025L;
    } else {
	c = (long)(365.25*y) - 694025L;
    }
  
    d = (int)(30.6001*(m+1));
  
    mjd = b + c + d + dy - 0.5;
  
    last_mn = mn;
    last_dy = dy;
    last_yr = yr;
    last_mjd = mjd;

    return mjd;
}


// return the current modified Julian date (number of days elapsed
// since 1900 jan 0.5), mjd.
double sgTimeCurrentMJD( time_t ct, long int warp ) {

    struct tm m_gmt;    // copy of system gmtime(&time_t) structure
    struct tm *gmt = &m_gmt;

    // get current Unix calendar time (in seconds)
    // warp += warp_delta;
    time_t cur_time;
    if ( ct ) {
        cur_time = ct + warp;
    } else {
        cur_time = time(NULL) + warp;
    }

    // get GMT break down for current time
    memcpy( gmt, gmtime(&cur_time), sizeof(tm) );

    // calculate modified Julian date
    // t->mjd = cal_mjd ((int)(t->gmt->tm_mon+1), (double)t->gmt->tm_mday, 
    //     (int)(t->gmt->tm_year + 1900));
    double mjd = sgTimeCalcMJD( (int)(gmt->tm_mon+1), (double)gmt->tm_mday, 
                                (int)(gmt->tm_year + 1900) );

    return mjd;
}


// given an mjd, calculate greenwich mean sidereal time, gst
double sgTimeCalcGST( double mjd ) {
    double gst;

    double day = floor(mjd-0.5)+0.5;
    double hr = (mjd-day)*24.0;
    double T, x;

    T = ((int)(mjd - 0.5) + 0.5 - J2000)/36525.0;
    x = 24110.54841 + (8640184.812866 + (0.093104 - 6.2e-6 * T) * T) * T;
    x /= 3600.0;
    gst = (1.0/SIDRATE)*hr + x;

    return gst;
}


/******************************************************************
 * The following are some functions that were included as SGTime
 * members, although they currently don't make use of any of the
 * class's variables. Maybe this'll change in the future
 *****************************************************************/

// Return time_t for Sat Mar 21 12:00:00 GMT
//

time_t sgTimeGetGMT(int year, int month, int day, int hour, int min, int sec)
{
    struct tm mt;

    mt.tm_mon = month;
    mt.tm_mday = day;
    mt.tm_year = year;
    mt.tm_hour = hour;
    mt.tm_min = min;
    mt.tm_sec = sec;
    mt.tm_isdst = -1; // let the system determine the proper time zone

#if defined(SG_WINDOWS)
    return _mkgmtime(&mt);
#elif defined( HAVE_TIMEGM )
    return ( timegm(&mt) );
#else
    #error Unix platforms should have timegm
#endif
}

// format time
char* sgTimeFormatTime( const struct tm* p, char* buf )
{
    sprintf( buf, "%d/%d/%2d %d:%02d:%02d", 
	     p->tm_mon, p->tm_mday, p->tm_year,
	     p->tm_hour, p->tm_min, p->tm_sec);
    return buf;
}
