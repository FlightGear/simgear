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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 **************************************************************************/

/*************************************************************************
 *
 * SGTimeZone is derived from geocoord, and stores the timezone centerpoint,
 * as well as the countrycode and the timezone descriptor. The latter is 
 * used in order to get the local time. 
 *
 ************************************************************************/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "timezone.h"

SGTimeZone::SGTimeZone(float la, float lo, char* cc, char* desc) :
    SGGeoCoord(la, lo)
{ 
    countryCode = cc;
    descriptor = desc;
}

/* Build a timezone object from a textline in zone.tab */
SGTimeZone::SGTimeZone(const char *infoString) :
    SGGeoCoord()
{
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
}

/* the copy constructor */
SGTimeZone::SGTimeZone(const SGTimeZone& other)
{
    lat = other.getLat();
    lon = other.getLon();
    countryCode = other.countryCode;
    descriptor = other.descriptor;
}


/********* Member functions for SGTimeZoneContainer class ********/

SGTimeZoneContainer::SGTimeZoneContainer(const char *filename)
{
    char buffer[256];
    FILE* infile = fopen(filename, "rb");
    if (!(infile)) {
        fprintf(stderr, "Unable to open file %s\n", filename);
        exit(1);
    } else { 
        errno = 0;
    
        while (1) {
            fgets(buffer, 256, infile);
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
                data.push_back(new SGTimeZone(buffer));
            }
        }
        if ( errno ) {
            perror( "SGTimeZoneContainer()" );
            errno = 0;
        }
    }
}

SGTimeZoneContainer::~SGTimeZoneContainer()
{
}
