// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#include <cmath>

#include "saturn.hxx"

/*************************************************************************
 * Saturn::Saturn(double mjd)
 * Public constructor for class Saturn
 * Argument: The current time.
 * the hard coded orbital elements for Saturn are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Saturn::Saturn(double mjd) :
  CelestialBody(113.6634,   2.3898000E-5,
		2.4886,	   -1.081E-7,
		339.3939,   2.9766100E-5,
		9.5547500,  0.000000,
		0.055546,  -9.499E-9,
		316.9670,   0.03344422820, mjd)
{
}
Saturn::Saturn() :
  CelestialBody(113.6634,   2.3898000E-5,
		2.4886,	   -1.081E-7,
		339.3939,   2.9766100E-5,
		9.5547500,  0.000000,
		0.055546,  -9.499E-9,
		316.9670,   0.03344422820)
{
}

/*************************************************************************
 * void Saturn::updatePosition(double mjd, Star *ourSun)
 * 
 * calculates the current position of Saturn, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Saturn specific equation
 *************************************************************************/
void Saturn::updatePosition(double mjd, Star *ourSun)
{
  CelestialBody::updatePosition(mjd, ourSun);
  
  double actTime = sgCalcActTime(mjd);
  double ir = 0.4897394;
  double Nr = 2.9585076 + 6.6672E-7*actTime;
  double B = asin (sin(declination) * cos(ir) - 
		   cos(declination) * sin(ir) *
		   sin(rightAscension - Nr));
  double ring_magn = -2.6 * sin(fabs(B)) + 1.2 * pow(sin(B), 2);
  magnitude = -9.0 + 5*log10(r*R) + 0.044 * FV + ring_magn;
}

