// constants.h -- various constant definitions
//
// Written by Curtis Olson, started February 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$

/** \file constants.h
 * Various constant definitions.
 */


#ifndef _SG_CONSTANTS_H
#define _SG_CONSTANTS_H


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#else
#  ifdef SG_MATH_EXCEPTION_CLASH
#    define exception C_exception
#  endif
#  include <math.h>
#endif

#include <plib/sg.h>


// Make sure PI is defined in its various forms

// SG_PI and SGD_PI (float and double) come from plib/sg.h

/** 2 * PI */
#define SGD_2PI      6.28318530717958647692

/** PI / 2 */
#ifdef M_PI_2
#  define  SGD_PI_2  M_PI_2
#else
#  define  SGD_PI_2  1.57079632679489661923
#endif

/** PI / 4 */
#define SGD_PI_4     0.78539816339744830961

/** \def SG_E "e" */
#ifdef M_E
#  define SG_E     M_E
#else
#  define SG_E     2.7182818284590452354
#endif

/** pi/180/60/60, or about 100 feet at earths' equator */
#define SG_ONE_SECOND 4.848136811E-6


/** Radius of Earth in kilometers at the equator.  Another source had
 *  6378.165 but this is probably close enough */
#define SG_EARTH_RAD 6378.155


// Earth parameters for WGS 84, taken from LaRCsim/ls_constants.h

/** Value of earth radius from LaRCsim (ft) */
#define SG_EQUATORIAL_RADIUS_FT  20925650.

/** Value of earth radius from LaRCsim (meter) */
#define SG_EQUATORIAL_RADIUS_M   6378138.12

/** Radius squared (ft) */
#define SG_EQ_RAD_SQUARE_FT 437882827922500.

/** Radius squared (meter) */
#define SG_EQ_RAD_SQUARE_M   40680645877797.1344

// Conversions

/** Arc seconds to radians.  (arcsec*pi)/(3600*180) = rad */
#define SG_ARCSEC_TO_RAD    4.84813681109535993589e-06 

/** Radians to arc seconds.  (rad*3600*180)/pi = arcsec */
#define SG_RAD_TO_ARCSEC    206264.806247096355156

/** Feet to Meters */
#define SG_FEET_TO_METER    0.3048

/** Meters to Feet */
#define SG_METER_TO_FEET    3.28083989501312335958  

/** Meters to Nautical Miles.  1 nm = 6076.11549 feet */
#define SG_METER_TO_NM      0.00053995680

/** Nautical Miles to Meters */
#define SG_NM_TO_METER      1852.0000

/** Radians to Nautical Miles.  1 nm = 1/60 of a degree */
#define SG_NM_TO_RAD        0.00029088820866572159

/** Nautical Miles to Radians */
#define SG_RAD_TO_NM        3437.7467707849392526

/** For divide by zero avoidance, this will be close enough to zero */
#define SG_EPSILON 0.0000001


/** Highest binobj format version we know how to read/write.  This starts at
 *  0 and can go up to 65535 */
#define SG_BINOBJ_VERSION 5

/** for backwards compatibility */
#define SG_SCENERY_FILE_FORMAT "0.4"


#endif // _SG_CONSTANTS_H
