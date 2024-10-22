// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#include <cmath>

#include "venus.hxx"

/*************************************************************************
 * Venus::Venus(double mjd)
 * Public constructor for class Venus
 * Argument: The current time.
 * the hard coded orbital elements for Venus are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Venus::Venus(double mjd) :
  CelestialBody(76.67990,  2.4659000E-5, 
		3.3946,    2.75E-8,
		54.89100,  1.3837400E-5,
		0.7233300, 0.000000,
		0.006773, -1.302E-9,
		48.00520,  1.60213022440, mjd)
{
}
Venus::Venus() :
  CelestialBody(76.67990,  2.4659000E-5, 
		3.3946,    2.75E-8,
		54.89100,  1.3837400E-5,
		0.7233300, 0.000000,
		0.006773, -1.302E-9,
		48.00520,  1.60213022440)
{
}

/*************************************************************************
 * void Venus::updatePosition(double mjd, Star *ourSun)
 * 
 * calculates the current position of Venus, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Venus specific equation
 *************************************************************************/
void Venus::updatePosition(double mjd, Star *ourSun)
{
  CelestialBody::updatePosition(mjd, ourSun);
  magnitude = -4.34 + 5*log10( r*R ) + 0.013 * FV + 4.2E-07 * pow(FV,3);
}
