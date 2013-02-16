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

#ifndef SG_ROTATE_TRANSFORM_HXX
#define SG_ROTATE_TRANSFORM_HXX

#include <osg/Transform>
#include <simgear/math/SGMath.hxx>

class SGRotateTransform : public osg::Transform {
public:
  SGRotateTransform();
  SGRotateTransform(const SGRotateTransform&,
                    const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

  META_Node(simgear, SGRotateTransform);
  
  void setCenter(const SGVec3f& center)
  { setCenter(toVec3d(center)); }
  void setCenter(const SGVec3d& center)
  { _center = center; dirtyBound(); }
  const SGVec3d& getCenter() const
  { return _center; }

  void setAxis(const SGVec3f& axis)
  { setAxis(toVec3d(axis)); }
  void setAxis(const SGVec3d& axis)
  { _axis = axis; dirtyBound(); }
  const SGVec3d& getAxis() const
  { return _axis; }

  void setAngleRad(double angle)
  { _angleRad = angle; }
  double getAngleRad() const
  { return _angleRad; }

  void setAngleDeg(double angle)
  { _angleRad = SGMiscd::deg2rad(angle); }
  double getAngleDeg() const
  { return SGMiscd::rad2deg(_angleRad); }

  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const;
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const;

  virtual osg::BoundingSphere computeBound() const;

  // Useful for other classes too.
  static void set_rotation (osg::Matrix &matrix, double position_rad,
                            const SGVec3d &center, const SGVec3d &axis);
private:
  SGVec3d _center;
  SGVec3d _axis;
  double _angleRad;
};

#endif
