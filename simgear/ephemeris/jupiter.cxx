// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#include <cmath>

#include "jupiter.hxx"

/*************************************************************************
 * Jupiter::Jupiter(double mjd)
 * Public constructor for class Jupiter
 * Argument: The current time.
 * the hard coded orbital elements for Jupiter are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Jupiter::Jupiter(double mjd) :
  CelestialBody(100.4542,  2.7685400E-5,	
		1.3030,   -1.557E-7,
		273.8777,  1.6450500E-5,
		5.2025600, 0.000000,
		0.048498,  4.469E-9,
		19.89500,  0.08308530010, mjd)
{
}

Jupiter::Jupiter() :
  CelestialBody(100.4542,  2.7685400E-5,	
		1.3030,   -1.557E-7,
		273.8777,  1.6450500E-5,
		5.2025600, 0.000000,
		0.048498,  4.469E-9,
		19.89500,  0.08308530010)
{
}

/*************************************************************************
 * void Jupiter::updatePosition(double mjd, Star *ourSun)
 * 
 * calculates the current position of Jupiter, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Jupiter specific equation
 *************************************************************************/
void Jupiter::updatePosition(double mjd, Star *ourSun)
{
  CelestialBody::updatePosition(mjd, ourSun);
  magnitude = -9.25 + 5*log10( r*R ) + 0.014 * FV;
}




