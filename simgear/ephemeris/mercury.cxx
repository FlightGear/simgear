/**************************************************************************
 * mercury.cxx
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


