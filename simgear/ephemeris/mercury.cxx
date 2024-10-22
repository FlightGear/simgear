// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Durk Talsma

#include <cmath>

#include "mercury.hxx"

/*************************************************************************
 * Mercury::Mercury(double mjd)
 * Public constructor for class Mercury
 * Argument: The current time.
 * the hard coded orbital elements for Mercury are passed to 
 * CelestialBody::CelestialBody();
 ************************************************************************/
Mercury::Mercury(double mjd) :
  CelestialBody (48.33130,   3.2458700E-5,
                  7.0047,    5.00E-8,
                  29.12410,  1.0144400E-5,
                  0.3870980, 0.000000,
                  0.205635,  5.59E-10,
                  168.6562,  4.09233443680, mjd)
{
}
Mercury::Mercury() :
  CelestialBody (48.33130,   3.2458700E-5,
                  7.0047,    5.00E-8,
                  29.12410,  1.0144400E-5,
                  0.3870980, 0.000000,
                  0.205635,  5.59E-10,
                  168.6562,  4.09233443680)
{
}
/*************************************************************************
 * void Mercury::updatePosition(double mjd, Star *ourSun)
 * 
 * calculates the current position of Mercury, by calling the base class,
 * CelestialBody::updatePosition(); The current magnitude is calculated using 
 * a Mercury specific equation
 *************************************************************************/
void Mercury::updatePosition(double mjd, Star *ourSun)
{
  CelestialBody::updatePosition(mjd, ourSun);
  magnitude = -0.36 + 5*log10( r*R ) + 0.027 * FV + 2.2E-13 * pow(FV, 6); 
}


