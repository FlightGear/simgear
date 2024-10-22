// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#include <cmath>

#include "uranus.hxx"

/*************************************************************************
 * Uranus::Uranus(double mjd)
 * Public constructor for class Uranus
 * Argument: The current time.
 * the hard coded orbital elements for Uranus are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Uranus::Uranus(double mjd) :
  CelestialBody(74.00050,   1.3978000E-5,
		0.7733,     1.900E-8,
		96.66120,   3.0565000E-5,
		19.181710, -1.55E-8,
		0.047318,   7.450E-9,
		142.5905,   0.01172580600, mjd)
{
}
Uranus::Uranus() :
  CelestialBody(74.00050,   1.3978000E-5,
		0.7733,     1.900E-8,
		96.66120,   3.0565000E-5,
		19.181710, -1.55E-8,
		0.047318,   7.450E-9,
		142.5905,   0.01172580600)
{
}

/*************************************************************************
 * void Uranus::updatePosition(double mjd, Star *ourSun)
 * 
 * calculates the current position of Uranus, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Uranus specific equation
 *************************************************************************/
void Uranus::updatePosition(double mjd, Star *ourSun)
{
  CelestialBody::updatePosition(mjd, ourSun);
  magnitude = -7.15 + 5*log10( r*R) + 0.001 * FV;
}
