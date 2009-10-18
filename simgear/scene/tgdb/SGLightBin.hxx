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

#ifndef SG_LIGHT_BIN_HXX
#define SG_LIGHT_BIN_HXX

#include <simgear/math/SGMath.hxx>

class SGLightBin {
public:
  struct Light {
    Light(const SGVec3f& p, const SGVec4f& c) :
      position(p), color(c)
    { }
    SGVec3f position;
    SGVec4f color;
  };
  typedef std::vector<Light> LightList;

  void insert(const Light& light)
  { _lights.push_back(light); }
  void insert(const SGVec3f& p, const SGVec4f& c)
  { insert(Light(p, c)); }

  unsigned getNumLights() const
  { return _lights.size(); }
  const Light& getLight(unsigned i) const
  { return _lights[i]; }

private:
  LightList _lights;
};

#endif
