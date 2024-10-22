// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#ifdef __BORLANDC__
#  define exception c_exception
#endif
#include <cmath>

#include "neptune.hxx"

/*************************************************************************
 * Neptune::Neptune(double mjd)
 * Public constructor for class Neptune
 * Argument: The current time.
 * the hard coded orbital elements for Neptune are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Neptune::Neptune(double mjd) :
  CelestialBody(131.7806,   3.0173000E-5,
		1.7700,	   -2.550E-7,
		272.8461,  -6.027000E-6,	
		30.058260,  3.313E-8,
		0.008606,   2.150E-9,
		260.2471,   0.00599514700, mjd)
{
}
Neptune::Neptune() :
  CelestialBody(131.7806,   3.0173000E-5,
		1.7700,	   -2.550E-7,
		272.8461,  -6.027000E-6,	
		30.058260,  3.313E-8,
		0.008606,   2.150E-9,
		260.2471,   0.00599514700)
{
}
/*************************************************************************
 * void Neptune::updatePosition(double mjd, Star *ourSun)
 * 
 * calculates the current position of Neptune, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Neptune specific equation
 *************************************************************************/
void Neptune::updatePosition(double mjd, Star *ourSun)
{
  CelestialBody::updatePosition(mjd, ourSun);
  magnitude = -6.90 + 5*log10 (r*R) + 0.001 *FV;
}
