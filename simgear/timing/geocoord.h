/* -*- Mode: C++ -*- *****************************************************
 * geocoord.h
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
 * This file defines a small and simple class to store geocentric 
 * coordinates. Basically, class GeoCoord is intended as a base class for
 * any kind of of object, that can be categorized according to its 
 * location on earth, be it navaids, or aircraft. This class for originally
 * written for FlightGear, in order to store Timezone control points. 
 *
 ************************************************************************/


#ifndef _GEOCOORD_H_
#define _GEOCOORD_H_

#include <simgear/compiler.h>


#include <math.h>
#include <string>
#include STL_IOSTREAM
//#include <streambuf> // looks like streambuf does not exist on linux.
// But it looks like it isn't used anyways -:)
#include <vector>

SG_USING_NAMESPACE(std);

#include <simgear/constants.h>

class SGGeoCoord
{
protected:
  float lat;
  float lon;

public:
  SGGeoCoord() { lat = 0.0; lon = 0.0;};
  SGGeoCoord(float la, float lo) { lat = la; lon = lo;};
  SGGeoCoord(const SGGeoCoord& other);
  virtual ~SGGeoCoord() {};
  
  void set(float la, float lo) { lat = la; lon = lo; }; 
  float getLat() const { return lat; };
  float getLon() const { return lon; };
  float getX()   const { return cos(SGD_DEGREES_TO_RADIANS*lat) * cos(SGD_DEGREES_TO_RADIANS*lon); };
  float getY()   const { return cos(SGD_DEGREES_TO_RADIANS*lat) * sin(SGD_DEGREES_TO_RADIANS*lon); };
  float getZ()   const { return sin(SGD_DEGREES_TO_RADIANS*lat); };


  //double getAngle(const SGGeoCoord& other) const;
  virtual void print() {} ; 
  virtual const char * getDescription() {return 0;};
};

typedef vector<SGGeoCoord*> SGGeoCoordVector;
typedef vector<SGGeoCoord*>::iterator SGGeoCoordVectorIterator;
typedef vector<SGGeoCoord*>::const_iterator SGGeoCoordVectorConstIterator;

/************************************************************************
 * SGGeoCoordContainer is a simple container class, that stores objects
 * derived from SGGeoCoord. Basically, it is a wrapper around an STL vector,
 * with some added functionality
 ***********************************************************************/

class SGGeoCoordContainer
{
protected:
  SGGeoCoordVector data;

public:
  SGGeoCoordContainer() {};
  virtual ~SGGeoCoordContainer();

  const SGGeoCoordVector& getData() const { return data; };
  SGGeoCoord* getNearest(const SGGeoCoord& ref) const;
};


#endif // _GEO_COORD_H_
