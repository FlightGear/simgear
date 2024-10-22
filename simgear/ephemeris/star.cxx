// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#include <simgear_config.h>
#include <cmath>

#include <simgear/debug/logstream.hxx>

#include "star.hxx"


/*************************************************************************
 * Star::Star(double mjd)
 * Public constructor for class Star
 * Argument: The current time.
 * the hard coded orbital elements our sun are passed to 
 * CelestialBody::CelestialBody();
 * note that the word sun is avoided, in order to prevent some compilation
 * problems on sun systems 
 ************************************************************************/
Star::Star(double mjd) :
    CelestialBody (0.000000,  0.0000000000,
		   0.0000,    0.00000,
		   282.9404,  4.7093500E-5,	
		   1.0000000, 0.000000,	
		   0.016709,  -1.151E-9,
		   356.0470,  0.98560025850, mjd)
{
    distance = 0.0;
}

Star::Star() :
    CelestialBody (0.000000,  0.0000000000,
		   0.0000,    0.00000,
		   282.9404,  4.7093500E-5,	
		   1.0000000, 0.000000,	
		   0.016709,  -1.151E-9,
		   356.0470,  0.98560025850)
{
    distance = 0.0;
}

Star::~Star()
{
}


/*************************************************************************
 * void Star::updatePosition(double mjd, Star *ourSun)
 * 
 * calculates the current position of our sun.
 *************************************************************************/
void Star::updatePosition(double mjd)
{
  double 
    actTime, eccAnom, 
    xv, yv, v, r,
    xe, ecl;

  updateOrbElements(mjd);
  
  actTime = sgCalcActTime(mjd);
  ecl = SGD_DEGREES_TO_RADIANS * (23.4393 - 3.563E-7 * actTime); // Angle in Radians
  eccAnom = sgCalcEccAnom(M, e);  // Calculate the eccentric Anomaly (also known as solving Kepler's equation)
  
  xv = cos(eccAnom) - e;
  yv = sqrt (1.0 - e*e) * sin(eccAnom);
  v = atan2 (yv, xv);                   // the sun's true anomaly
  distance = r = sqrt (xv*xv + yv*yv);  // and its distance

  lonEcl = v + w; // the sun's true longitude
  latEcl = 0;

  // convert the sun's true longitude to ecliptic rectangular 
  // geocentric coordinates (xs, ys)
  xs = r * cos (lonEcl);
  ys = r * sin (lonEcl);

  // convert ecliptic coordinates to equatorial rectangular
  // geocentric coordinates

  xe = xs;
  ye = ys * cos (ecl);
  ze = ys * sin (ecl);

  // And finally, calculate right ascension and declination
  rightAscension = atan2 (ye, xe);
  declination = atan2 (ze, sqrt (xe*xe + ye*ye));
}
