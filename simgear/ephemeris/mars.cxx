/**************************************************************************
 * mars.cxx
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
