/* -*- Mode: C++ -*- *****************************************************
 * geocoord.h
 * Written by Durk Talsma. Started March 1998.
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
 * coordinates. Basically, class SGGeoCoord is intended as a base class for
 * any kind of of object, that can be categorized according to its 
 * location on earth, be it navaids, or aircraft. This class for originally
 * written for FlightGear, in order to store Timezone control points. 
 *
 ************************************************************************/
#include "geocoord.h"
#include <plib/sg.h>

SGGeoCoord::SGGeoCoord(const SGGeoCoord& other)
{
  lat = other.lat;
  lon = other.lon;
}

// double SGGeoCoord::getAngle(const SGGeoCoord& other) const
// {
//   Vector first(      getX(),       getY(),       getZ());
//   Vector secnd(other.getX(), other.getY(), other.getZ());
//     double
//       dot = VecDot(first, secnd),
//       len1 = first.VecLen(),
//       len2 = secnd.VecLen(),
//       len = len1 * len2,
//       angle = 0;
//     //printf ("Dot: %f, len1: %f len2: %f\n", dot, len1, len2);
//     /*Vector pPos = prevPos - Reference->prevPos;
//       Vector pVel = prevVel - Reference->prevVel;*/


//     if ( ( (dot / len) < 1) && (dot / len > -1) && len )
//     	angle = acos(dot / len);
//     return angle;
// }

// SGGeoCoord* SGGeoCoordContainer::getNearest(const SGGeoCoord& ref) const
// {
//   float angle, maxAngle = 180;

//   SGGeoCoordVectorConstIterator i, nearest;
//   for (i = data.begin(); i != data.end(); i++)
//     {
//       angle = SGD_RADIANS_TO_DEGREES * (*i)->getAngle(ref);
//       if (angle < maxAngle)
// 	{
// 	  maxAngle = angle;
// 	  nearest = i;
// 	}
//     }
//   return *nearest;
// }


SGGeoCoord* SGGeoCoordContainer::getNearest(const SGGeoCoord& ref) const
{
  sgVec3 first, secnd;
  float dist, maxDist=SG_MAX;
  sgSetVec3( first, ref.getX(), ref.getY(), ref.getZ());
  SGGeoCoordVectorConstIterator i, nearest;
  for (i = data.begin(); i != data.end(); i++)
    {
      sgSetVec3(secnd, (*i)->getX(), (*i)->getY(), (*i)->getZ());
      dist = sgDistanceSquaredVec3(first, secnd);
      if (dist < maxDist)
	{
	  maxDist = dist;
	  nearest = i;
	}
    }
  return *nearest;
}


SGGeoCoordContainer::~SGGeoCoordContainer()
{
    SGGeoCoordVectorIterator i = data.begin();
  while (i != data.end())
    delete *i++;
}
