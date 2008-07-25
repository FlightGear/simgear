/* -*- Mode: C++ -*- *****************************************************
 * timezone.h
 * Written by Durk Talsma. Started July 1999.
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
 **************************************************************************/

/** \file timezone.h
 *
 * Provides SGTimeZone and SGTimeZoneContainer
 *
 */

#ifndef _TIMEZONE_H_
#define _TIMEZONE_H_

#include <stdio.h>
#include <string>

#include <simgear/timing/geocoord.h>

/**
 * SGTimeZone is derived from geocoord, and stores the timezone centerpoint,
 * as well as the countrycode and the timezone descriptor. The latter is 
 * used in order to get the local time. 
 *
 */

class SGTimeZone : public SGGeoCoord
{

private:

  std::string countryCode;
  std::string descriptor;

public:

    /**
     * Default constructor.
     */
    SGTimeZone() : SGGeoCoord()
    {
        countryCode.erase(); 
        descriptor.erase();
    };

    /**
     * Build a timezone object with a specifed latitude, longitude, country
     * code, and descriptor
     * @param la latitude
     * @param lo longitude
     * @param cc country code
     * @param desc descriptor
     */
    SGTimeZone(float la, float lo, char* cc, char* desc);

    /**
     * Build a timezone object from a textline in zone.tab
     * @param infoString the textline from zone.tab
     */
    SGTimeZone(const char *infoString);

    /**
     * The copy constructor
     * @param other the source object
     */
    SGTimeZone(const SGTimeZone &other);

    /**
     * Virutal destructor 
     */
    virtual ~SGTimeZone() { };
  
    /**
     * Return the descriptor string
     * @return descriptor string (char array)
     */
    virtual const char * getDescription() { return descriptor.c_str(); };
};

/**
 * SGTimeZoneContainer is derived from SGGeoCoordContainer, and has some 
 * added functionality.
 */

class SGTimeZoneContainer : public SGGeoCoordContainer
{
 public:
  SGTimeZoneContainer(const char *filename);
  virtual ~SGTimeZoneContainer();
};



#endif // _TIMEZONE_H_
