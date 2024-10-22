// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1999 Durk Talsma

/**
 * @file
 * @brief SGTimeZone is derived from geocoord, and stores the timezone centerpoint, as well as the countrycode and the timezone descriptor. 
 *        The latter is used in order to get the local time.
 */

#include <simgear_config.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <cstdlib>

#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/strutils.hxx>
#include <simgear/io/sg_mmap.hxx>
#include <simgear/misc/sg_path.hxx>

#include "timezone.h"

#define ZD_EXPORT
#include "zonedetect.h"

using std::string;

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

class SGTimeZoneContainer::SGTimeZoneContainerPrivate
{
public:
    ~SGTimeZoneContainerPrivate()
    {
        for (auto z : zones) {
            delete z;
        }
            
        if (cd) {
            ZDCloseDatabase(cd);
        }
    }
    
    SGMMapFile file;
    ZoneDetect *cd = nullptr;
    const char* buffer = nullptr;
    size_t size = 0;

    // zone.tab related
    bool is_zone_tab = false;
    using TZVec = std::vector<SGTimeZone*>;
    TZVec zones;
};

SGTimeZoneContainer::SGTimeZoneContainer(const SGPath& path) :
    d(new SGTimeZoneContainerPrivate)
{
    if (path.file() != "zone.tab") {
        if ( !d->file.open(path, SG_IO_IN) ) {
            throw sg_io_exception("cannot open timezone file", path);
        }

        d->buffer = d->file.get();
        d->size = d->file.get_size();
        d->cd = ZDOpenDatabaseFromMemory((void*) d->buffer, d->size);
        if (!d->cd) {
          throw sg_io_exception("timezone database read error");
        }
    }
  else // zone.tab is in filename
  {
    char buffer[256];
#if defined(SG_WINDOWS)
      const std::wstring wfile = path.wstr();
    FILE* infile = _wfopen(wfile.c_str(), L"rb");
#else
      const auto s = path.utf8Str();
    FILE* infile = fopen(s.c_str(), "rb");
#endif
    if (!(infile)) {
        throw sg_exception("Unable to open time zone file", "", sg_location{path});
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
            d->zones.push_back(new SGTimeZone(buffer));
        }
    }
    if ( errno ) {
        perror( "SGTimeZoneContainer()" );
        errno = 0;
    }
    
    fclose(infile);
    d->is_zone_tab = true;
  }
}

SGTimeZoneContainer::~SGTimeZoneContainer()
{
    
}

SGTimeZone* SGTimeZoneContainer::getNearest(const SGGeod& ref) const
{
  SGTimeZone* match = NULL;

  if (d->is_zone_tab)
  {
    SGVec3d refCart(SGVec3d::fromGeod(ref));
    double minDist2 = HUGE_VAL;
  
    for (auto z : d->zones) {
      double d2 = distSqr(z->cartCenterpoint(), refCart);
      if (d2 < minDist2) {
          match = z;
        minDist2 = d2;
      }
    }
  }
  else if (d->cd) // timezone16.bin
  {
    char *CountryAlpha2 = nullptr;
    char *TimezoneIdPrefix = nullptr;
    char *TimezoneId = nullptr;

    float safezone = 0;
    float lat = ref.getLatitudeDeg();
    float lon = ref.getLongitudeDeg();
    ZoneDetectResult *results = ZDLookup(d->cd, lat, lon, &safezone);
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

      if (TimezoneIdPrefix && TimezoneId) {
        const auto desc = string{TimezoneIdPrefix} + string{TimezoneId};
        match = new SGTimeZone(ref, CountryAlpha2, (char*)desc.c_str());
      }
    }
  }

  return match;
}

