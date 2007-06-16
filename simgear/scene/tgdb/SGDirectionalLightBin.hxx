/* -*-c++-*-
 *
 * Copyright (C) 2006-2007 Mathias Froehlich 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef SG_DIRECTIONAL_LIGHT_BIN_HXX
#define SG_DIRECTIONAL_LIGHT_BIN_HXX

class SGDirectionalLightBin {
public:
  struct Light {
    Light(const SGVec3f& p, const SGVec3f& n, const SGVec4f& c) :
      position(p), normal(n), color(c)
    { }
    SGVec3f position;
    SGVec3f normal;
    SGVec4f color;
  };
  typedef std::vector<Light> LightList;

  void insert(const Light& light)
  { _lights.push_back(light); }
  void insert(const SGVec3f& p, const SGVec3f& n, const SGVec4f& c)
  { insert(Light(p, n, c)); }

  unsigned getNumLights() const
  { return _lights.size(); }
  const Light& getLight(unsigned i) const
  { return _lights[i]; }

  // helper for sorting lights back to front ...
  struct DirectionLess {
    DirectionLess(const SGVec3f& direction) :
      _direction(direction)
    { }
    inline bool operator() (const Light& l, const Light& r) const
    { return dot(_direction, l.position) < dot(_direction, r.position); }
  private:
    SGVec3f _direction;
  };
  typedef std::multiset<Light,DirectionLess> LightSet;

  LightList getSortedLights(const SGVec3f& sortDirection) const
  {
    LightSet lightSet = LightSet(DirectionLess(sortDirection));
    for (unsigned i = 0; i < _lights.size(); ++i)
      lightSet.insert(_lights[i]);

    LightList sortedLights;
    sortedLights.reserve(_lights.size());
    for (LightSet::const_iterator i = lightSet.begin(); i != lightSet.end(); ++i)
      sortedLights.push_back(*i);

    return sortedLights;
  }
  
private:
  LightList _lights;
};

#endif
