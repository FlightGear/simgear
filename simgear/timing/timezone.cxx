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

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/io/sg_file.hxx>

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
    char* double_end;
    sign = latlon[0];
    strncpy(buffer, &latlon[1], 2);
    buffer[2] = 0;
    lat = std::strtod(buffer, &double_end);
    strncpy(buffer, &latlon[3], 2);
    buffer[2] = 0;
    lat += (std::strtod(buffer, &double_end) / 60);
    int nextPos;
    if (strlen(latlon) > 12) {
        nextPos = 7;
        strncpy(buffer, &latlon[5], 2);
        buffer[2] = 0;
        lat += (std::strtod(buffer, &double_end) / 3600.0);
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
    lon = std::strtod(buffer, &double_end);
    nextPos += 3;
    strncpy(buffer, &latlon[nextPos], 2);
    buffer[2] = 0;
 
    lon += (std::strtod(buffer, &double_end) / 60);
    if (strlen(latlon) > 12) {
        nextPos += 2;
        strncpy(buffer, &latlon[nextPos], 2); 
        buffer[2] = 0;
        lon +=  (std::strtod(buffer, &double_end) / 3600.00);
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
  std::string tzdb_file = filename;
  if (tzdb_file.find("zone.tab") == std::string::npos)
  {
    SGPath path(filename);
    sg_ifstream tzdb_file(path, std::ios::in);
    if ( !tzdb_file.is_open() ) {
      throw sg_io_exception("cannot open timezone file", filename);
    }

    tzdb_buffer = tzdb_file.read_all();
    cd = ZDOpenDatabaseFromMemory((void*)tzdb_buffer.c_str(), tzdb_buffer.size());
    if (!cd) {
      throw sg_io_exception("timezone database read error");
    }
  }
  else // zone.tab is in filename
  {
    char buffer[256];
#if defined(SG_WINDOWS)
    const std::wstring wfile = simgear::strutils::convertUtf8ToWString(filename);
    FILE* infile = _wfopen(wfile.c_str(), L"rb");
#else
    FILE* infile = fopen(filename, "rb");
#endif
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
    is_zone_tab = true;
  }
}

SGTimeZoneContainer::~SGTimeZoneContainer()
{
  if (is_zone_tab)
  {
    TZVec::iterator it = zones.begin();
    for (; it != zones.end(); ++it) {
      delete *it;
    }
  }
  else if (cd)
  {
    ZDCloseDatabase(cd);
  }
}

SGTimeZone* SGTimeZoneContainer::getNearest(const SGGeod& ref) const
{
  SGTimeZone* match = NULL;

  if (is_zone_tab)
  {
    SGVec3d refCart(SGVec3d::fromGeod(ref));
    double minDist2 = HUGE_VAL;
  
    TZVec::const_iterator it = zones.begin();
    for (; it != zones.end(); ++it) {
      double d2 = distSqr((*it)->cartCenterpoint(), refCart);
      if (d2 < minDist2) {
        match = *it;
        minDist2 = d2;
      }
    }
  }
  else if (cd) // timezone16.bin
  {
    char *CountryAlpha2 = nullptr;
    char *TimezoneIdPrefix = nullptr;
    char *TimezoneId = nullptr;

    float safezone = 0;
    float lat = ref.getLatitudeDeg();
    float lon = ref.getLongitudeDeg();
    ZoneDetectResult *results = ZDLookup(cd, lat, lon, &safezone);
    if (results && results[0].data)
    {
      for(unsigned i=0; i<results[0].numFields; ++i)
      {
        if(results[0].fieldNames[i] && results[0].data[i])
        {
          std::string fieldName = results[0].fieldNames[i];
          if (fieldName == "CountryAlpha2") {
            CountryAlpha2 = results[0].data[i];
          } else if (fieldName == "TimezoneIdPrefix") {
            TimezoneIdPrefix = results[0].data[i];
          } else if (fieldName == "TimezoneId") {
            TimezoneId = results[0].data[i];
          }
        }
      }

      std::string desc;
      desc = TimezoneIdPrefix;
      desc += TimezoneId;

      match = new SGTimeZone(ref, CountryAlpha2, (char*)desc.c_str());
    }
  }

  return match;
}

