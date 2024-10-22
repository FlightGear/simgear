// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1999 Durk Talsma

/**
 * @file
 * @brief SGTimeZone is derived from geocoord, and stores the timezone centerpoint, as well as the countrycode and the timezone descriptor. 
 *        The latter is used in order to get the local time.
 */

#ifndef _TIMEZONE_H_
#define _TIMEZONE_H_

#include <string>
#include <vector>
#include <memory>

#include <simgear/math/SGMath.hxx>
#include <simgear/math/SGGeod.hxx>

class SGPath;

class SGTimeZone
{

private:

  SGVec3d centerpoint; // centre of timezone, in cartesian coordinates
  std::string countryCode;
  std::string descriptor;

public:

    /**
     * Build a timezone object with a specifed latitude, longitude, country
     * code, and descriptor
     * @param pt centerpoint
     * @param cc country code
     * @param desc descriptor
     */
    SGTimeZone(const SGGeod& pt, char* cc, char* desc);

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
     * Return the descriptor string
     * @return descriptor string (char array)
     */
    const char * getDescription() { return descriptor.c_str(); };
    
    const SGVec3d& cartCenterpoint() const
    {
      return centerpoint;
    }
};

/**
 * SGTimeZoneContainer 
 */

class SGTimeZoneContainer final
{
public:
  SGTimeZoneContainer(const SGPath& path);
  ~SGTimeZoneContainer();   // non-virtual intentional
  
  SGTimeZone* getNearest(const SGGeod& ref) const;
  
private:
    class SGTimeZoneContainerPrivate;
    
    std::unique_ptr<SGTimeZoneContainerPrivate> d;
};



#endif // _TIMEZONE_H_
