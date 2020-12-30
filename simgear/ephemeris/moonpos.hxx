/**************************************************************************
 * moonpos.hxx
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
#ifndef _MOONPOS_HXX_
#define _MOONPOS_HXX_


#include <simgear/constants.h>

#include <simgear/ephemeris/celestialBody.hxx>
#include <simgear/ephemeris/star.hxx>


class MoonPos : public CelestialBody
{

private:

    double xg, yg;     // the moon's rectangular geocentric coordinates
    double ye, ze;     // the moon's rectangular equatorial coordinates
    double distance;   // the moon's distance to the earth
    double distance_in_a;  // the moon's distance to the earth in unit of its semi-mayor axis a
    double age;        // the moon's age from 0 to 2pi
    double phase;      // the moon's phase
    double log_I;      // the moon's illuminance outside the atmosphere (logged)
    double I_factor;   // the illuminance factor for the moon, between 0 and 1.
    // void TexInit();  // This should move to the constructor eventually.

    // GLUquadricObj *moonObject;
    // GLuint Sphere;
    // GLuint moon_texid;
    // GLuint moon_halotexid;
    // GLubyte *moon_texbuf;
    // GLubyte *moon_halotexbuf;
  
    // void setHalo();

public:

    MoonPos(double mjd);
    MoonPos();
    ~MoonPos();
    void updatePositionTopo(double mjd, double lst, double lat, Star *ourSun);
    void updatePosition(double mjd, Star *ourSun);
  // void newImage();
    double getM() const;
    double getw() const;
    double getxg() const;
    double getyg() const;
    double getye() const;
    double getze() const;
    double getDistance() const;
    double getDistanceInMayorAxis() const;
    double getAge() const;
    double getPhase() const;
    double getLogIlluminance() const;
    double getIlluminanceFactor() const;
};

inline double MoonPos::getM() const
{
  return M;
}

inline double MoonPos::getw() const
{
  return w;
}

inline double MoonPos::getxg() const
{
  return xg;
}

inline double MoonPos::getyg() const
{
  return yg;
}

inline double MoonPos::getye() const
{
   return ye;
}

inline double MoonPos::getze() const
{
   return ze;
}

inline double MoonPos::getDistance() const
{
  return distance;
}

inline double MoonPos::getDistanceInMayorAxis() const
{
  return distance_in_a;
}

inline double MoonPos::getAge() const
{
  return age;
}

inline double MoonPos::getPhase() const
{
  return phase;
}

inline double MoonPos::getLogIlluminance() const
{
  return log_I;
}

inline double MoonPos::getIlluminanceFactor() const
{
  return I_factor;
}

#endif // _MOONPOS_HXX_
