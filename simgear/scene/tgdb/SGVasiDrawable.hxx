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

#ifndef _SG_VASI_DRAWABLE_HXX
#define _SG_VASI_DRAWABLE_HXX

#include <simgear/compiler.h>
#include <simgear/math/SGMath.hxx>

#include <osg/Drawable>
#include <osg/Version>

class SGVasiDrawable : public osg::Drawable {
  struct LightData;
public:
  META_Object(SimGear, SGVasiDrawable);
  SGVasiDrawable(const SGVasiDrawable&, const osg::CopyOp&);
  SGVasiDrawable(const SGVec4f& red = SGVec4f(1, 0, 0, 1),
                 const SGVec4f& white = SGVec4f(1, 1, 1, 1));

  /// Add a red/white switching light pointing into the direction that
  /// is computed to point in about the given normal with the given
  /// azimut angle upwards. The up direction is the world up direction
  /// that defines the horizontal plane.
  void addLight(const SGVec3f& position, const SGVec3f& normal,
                const SGVec3f& up, float azimutDeg);

  /// add a red/white switching light pointing towards normal
  /// at the given position with the given up vector. The up direction
  /// is the world up direction that defines the horizontal plane.
  void addLight(const SGVec3f& position, const SGVec3f& normal,
                const SGVec3f& up);

  virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
  virtual osg::BoundingBox
#if OSG_VERSION_LESS_THAN(3,3,2)
  computeBound()
#else
  computeBoundingBox()
#endif
  const;

private:
  SGVec4f getColor(float angleDeg) const;
  void draw(const SGVec3f& eyePoint, const LightData& light) const;
  
  std::vector<LightData> _lights;
  SGVec4f _red;
  SGVec4f _white;
};

#endif // _SG_VASI_LIGHT_HXX
