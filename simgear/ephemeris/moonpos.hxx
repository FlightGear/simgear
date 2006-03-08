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
    void updatePosition(double mjd, double lst, double lat, Star *ourSun);
    // void newImage();
};


#endif // _MOONPOS_HXX_
