// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#include <cmath>

#include "mars.hxx"

/*************************************************************************
 * Mars::Mars(double mjd)
 * Public constructor for class Mars
 * Argument: The current time.
 * the hard coded orbital elements for Mars are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Mars::Mars(double mjd) :
  CelestialBody(49.55740,  2.1108100E-5,
		1.8497,   -1.78E-8,
		286.5016,  2.9296100E-5,
		1.5236880, 0.000000,
		0.093405,  2.516E-9,
		18.60210,  0.52402077660, mjd)
{
}
Mars::Mars() :
  CelestialBody(49.55740,  2.1108100E-5,
		1.8497,   -1.78E-8,
		286.5016,  2.9296100E-5,
		1.5236880, 0.000000,
		0.093405,  2.516E-9,
		18.60210,  0.52402077660)
{
}
/*************************************************************************
 * void Mars::updatePosition(double mjd, Star *ourSun)
 * 
 * calculates the current position of Mars, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Mars specific equation
 *************************************************************************/
void Mars::updatePosition(double mjd, Star *ourSun)
{
  CelestialBody::updatePosition(mjd, ourSun);
  magnitude = -1.51 + 5*log10( r*R ) + 0.016 * FV;
}
