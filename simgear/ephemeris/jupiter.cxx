/**************************************************************************
 * jupiter.cxx
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




