/**************************************************************************
 * star.hxx
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
#ifndef _STAR_HXX_
#define _STAR_HXX_


#include <simgear/ephemeris/celestialBody.hxx>


class Star : public CelestialBody
{

private:

    double xs, ys;     // the sun's rectangular geocentric coordinates
    double ye, ze;     // the sun's rectangularequatorial rectangular geocentric coordinates
    double distance;   // the sun's distance to the earth

public:

    Star (double mjd);
    Star ();
    ~Star();
    void updatePosition(double mjd);
    double getM() const;
    double getw() const;
    double getxs() const;
    double getys() const;
    double getye() const;
    double getze() const;
    double getDistance() const;
};


inline double Star::getM() const
{
  return M;
}

inline double Star::getw() const
{
  return w;
}

inline double Star::getxs() const
{
  return xs;
}

inline double Star::getys() const
{
  return ys;
}

inline double Star::getye() const
{
   return ye;
}

inline double Star::getze() const
{
   return ze;
}

inline double Star::getDistance() const
{
  return distance;
}


#endif // _STAR_HXX_














