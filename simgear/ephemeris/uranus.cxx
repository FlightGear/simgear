/**************************************************************************
 * uranus.cxx
 * Written by Durk Talsma. Originally started October 1997, for distribution  
 * with the FlightGear project. Version 2 was written in August and 
 * September 1998. This code is based upon algorithms and data kindly 
 * provided by Mr. Paul Schlyter. (pausch@saaf.se). 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 **************************************************************************/

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
