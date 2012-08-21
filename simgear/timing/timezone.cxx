/* -*- Mode: C++ -*- *****************************************************
 * timezone.cc
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

/*************************************************************************
 *
 * SGTimeZone is derived from geocoord, and stores the timezone centerpoint,
 * as well as the countrycode and the timezone descriptor. The latter is 
 * used in order to get the local time. 
 *
 ************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <cstdlib>

#include <simgear/structure/exception.hxx>

#include "timezone.h"

SGTimeZone::SGTimeZone(const SGGeod& geod, char* cc, char* desc) :
    centerpoint(SGVec3d::fromGeod(geod))
{ 
    countryCode = cc;
    descriptor = desc;
}

/* Build a timezone object from a textline in zone.tab */
SGTimeZone::SGTimeZone(const char *infoString)
{
    double lat = 0.0, lon = 0.0;
    
    int i = 0;
    while (infoString[i] != '\t')
        i++;
    char buffer[128];
    char latlon[128];
    strncpy(buffer, infoString, i);
    buffer[i] = 0;
    countryCode = buffer;
    i ++;
    int start = i;
    while (infoString[i] != '\t') {
        i++;
    }
    int size = i - start;
    strncpy(latlon, (&infoString[start]), size);
    latlon[size] = 0;
    char sign;
    sign = latlon[0];
    strncpy(buffer, &latlon[1], 2);
    buffer[2] = 0;
    lat = atof(buffer);
    strncpy(buffer, &latlon[3], 2);
    buffer[2] = 0;
    lat += (atof(buffer) / 60);
    int nextPos;
    if (strlen(latlon) > 12) {
        nextPos = 7;
        strncpy(buffer, &latlon[5], 2);
        buffer[2] = 0;
        lat += (atof(buffer) / 3600.0);
    } else {
        nextPos = 5;
    }
    if (sign == '-') {
        lat = -lat;
    }

    sign = latlon[nextPos];
    nextPos++;
    strncpy(buffer, &latlon[nextPos], 3);
    buffer[3] = 0;
    lon = atof(buffer);
    nextPos += 3;
    strncpy(buffer, &latlon[nextPos], 2);
    buffer[2] = 0;
 
    lon  += (atof(buffer) / 60);
    if (strlen(latlon) > 12) {
        nextPos += 2;
        strncpy(buffer, &latlon[nextPos], 2); 
        buffer[2] = 0;
        lon +=  (atof (buffer) / 3600.00);
    }
    if (sign == '-') {
        lon = -lon;
    }
    i ++;
    start = i;
    while (!((infoString[i] == '\t') || (infoString[i] == '\n'))) {
        i++;
    }
    size = i - start;
    strncpy(buffer, (&infoString[start]), size);
    buffer[size] = 0;
    descriptor = buffer;
    
    centerpoint = SGVec3d::fromGeod(SGGeod::fromDeg(lon, lat));
}

/* the copy constructor */
SGTimeZone::SGTimeZone(const SGTimeZone& other)
{
    centerpoint = other.centerpoint;
    countryCode = other.countryCode;
    descriptor = other.descriptor;
}


/********* Member functions for SGTimeZoneContainer class ********/

SGTimeZoneContainer::SGTimeZoneContainer(const char *filename)
{
    char buffer[256];
    FILE* infile = fopen(filename, "rb");
    if (!(infile)) {
        std::string e = "Unable to open time zone file '";
        throw sg_exception(e + filename + '\'');
    }
    
    errno = 0;

    while (1) {
        if (0 == fgets(buffer, 256, infile))
            break;
        if (feof(infile)) {
            break;
        }
        for (char *p = buffer; *p; p++) {
            if (*p == '#') {
                *p = 0;
                break;
            }    
        }
        if (buffer[0]) {
            zones.push_back(new SGTimeZone(buffer));
        }
    }
    if ( errno ) {
        perror( "SGTimeZoneContainer()" );
        errno = 0;
    }
    
    fclose(infile);
}

SGTimeZoneContainer::~SGTimeZoneContainer()
{
  TZVec::iterator it = zones.begin();
  for (; it != zones.end(); ++it) {
    delete *it;
  }
}

SGTimeZone* SGTimeZoneContainer::getNearest(const SGGeod& ref) const
{
  SGVec3d refCart(SGVec3d::fromGeod(ref));
  SGTimeZone* match = NULL;
  double minDist2 = HUGE_VAL;
  
  TZVec::const_iterator it = zones.begin();
  for (; it != zones.end(); ++it) {
    double d2 = distSqr((*it)->cartCenterpoint(), refCart);
    if (d2 < minDist2) {
      match = *it;
      minDist2 = d2;
    }
  }

  return match;
}

