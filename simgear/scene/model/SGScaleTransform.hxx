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

#ifndef SG_SCALE_TRANSFORM_HXX
#define SG_SCALE_TRANSFORM_HXX

#include <osg/Transform>
#include <simgear/math/SGMath.hxx>

class SGScaleTransform : public osg::Transform {
public:
  SGScaleTransform();
  SGScaleTransform(const SGScaleTransform&,
                   const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

  META_Node(simgear, SGScaleTransform);

  void setCenter(const SGVec3f& center)
  { setCenter(toVec3d(center)); }
  void setCenter(const SGVec3d& center)
  { _center = center; dirtyBound(); }
  const SGVec3d& getCenter() const
  { return _center; }

  void setScaleFactor(const SGVec3d& scaleFactor)
  {
    double boundScale = normI(scaleFactor);
    if (_boundScale < boundScale || 5*boundScale < _boundScale) {
      _boundScale = boundScale;
      dirtyBound();
    }
    _scaleFactor = scaleFactor;
  }
  void setScaleFactor(double scaleFactor)
  { 
    double boundScale = fabs(scaleFactor);
    if (_boundScale < boundScale || 5*boundScale < _boundScale) {
      _boundScale = boundScale;
      dirtyBound();
    }
    _scaleFactor = SGVec3d(scaleFactor, scaleFactor, scaleFactor);
  }
  const SGVec3d& getScaleFactor() const
  { return _scaleFactor; }


  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const;
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const;
  virtual osg::BoundingSphere computeBound() const;

private:
  SGVec3d _center;
  SGVec3d _scaleFactor;
  mutable double _boundScale;
};

#endif
