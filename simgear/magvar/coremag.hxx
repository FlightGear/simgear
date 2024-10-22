// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2000 Edward A Williams <Ed_Williams@compuserve.com>

/**
 * @file
 * @brief Compute local magnetic variation given position, altitude, and date
 * This is an implementation of the NIMA WMM 2000: http://www.nima.mil/GandG/ngdc-wmm2000.html
 */

//
// Adapted from Excel 3.0 version 3/27/94 EAW
// Recoded in C++ by Starry Chan
// WMM95 added and rearranged in ANSI-C EAW 7/9/95
// Put shell around program and made Borland & GCC compatible EAW 11/22/95
// IGRF95 added 2/96 EAW
// WMM2000 IGR2000 added 2/00 EAW
// Released under GPL 3/26/00 EAW
// Adaptions and modifications for the SimGear project  3/27/2000 CLO

#ifndef SG_MAGVAR_HXX
#define SG_MAGVAR_HXX

/* Convert date to Julian day    1950-2049 */
unsigned long int yymmdd_to_julian_days( int yy, int mm, int dd );

/* return variation (in degrees) given geodetic latitude (radians), longitude
(radians) ,height (km) and (Julian) date
N and E lat and long are positive, S and W negative
*/
double calc_magvar( double lat, double lon, double h, long dat, double* field );


#endif // SG_MAGVAR_HXX
