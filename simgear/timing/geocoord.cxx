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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/math/SGMath.hxx>
#include "geocoord.h"

SGGeoCoord::SGGeoCoord(const SGGeoCoord& other)
{
  lat = other.lat;
  lon = other.lon;
}

SGGeoCoord* SGGeoCoordContainer::getNearest(const SGGeoCoord& ref) const
{
  if (data.empty())
    return 0;

  float maxCosAng = -2;
  SGVec3f refVec(ref.getX(), ref.getY(), ref.getZ());
  SGGeoCoordVectorConstIterator i, nearest;
  for (i = data.begin(); i != data.end(); ++i)
    {
      float cosAng = dot(refVec, SGVec3f((*i)->getX(), (*i)->getY(), (*i)->getZ()));
      if (maxCosAng < cosAng)
	{
	  maxCosAng = cosAng;
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
