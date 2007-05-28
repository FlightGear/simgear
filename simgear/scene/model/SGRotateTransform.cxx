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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGRotateTransform.hxx"

static void
set_rotation (osg::Matrix &matrix, double position_rad,
              const SGVec3d &center, const SGVec3d &axis)
{
  double temp_angle = -position_rad;
  
  double s = sin(temp_angle);
  double c = cos(temp_angle);
  double t = 1 - c;
  
  // axis was normalized at load time 
  // hint to the compiler to put these into FP registers
  double x = axis[0];
  double y = axis[1];
  double z = axis[2];
  
  matrix(0, 0) = t * x * x + c ;
  matrix(0, 1) = t * y * x - s * z ;
  matrix(0, 2) = t * z * x + s * y ;
  matrix(0, 3) = 0;
  
  matrix(1, 0) = t * x * y + s * z ;
  matrix(1, 1) = t * y * y + c ;
  matrix(1, 2) = t * z * y - s * x ;
  matrix(1, 3) = 0;
  
  matrix(2, 0) = t * x * z - s * y ;
  matrix(2, 1) = t * y * z + s * x ;
  matrix(2, 2) = t * z * z + c ;
  matrix(2, 3) = 0;
  
  // hint to the compiler to put these into FP registers
  x = center[0];
  y = center[1];
  z = center[2];
  
  matrix(3, 0) = x - x*matrix(0, 0) - y*matrix(1, 0) - z*matrix(2, 0);
  matrix(3, 1) = y - x*matrix(0, 1) - y*matrix(1, 1) - z*matrix(2, 1);
  matrix(3, 2) = z - x*matrix(0, 2) - y*matrix(1, 2) - z*matrix(2, 2);
  matrix(3, 3) = 1;
}

SGRotateTransform::SGRotateTransform() :
  _center(0, 0, 0),
  _axis(0, 0, 0),
  _angleRad(0)
{
  setReferenceFrame(RELATIVE_RF);
}

bool
SGRotateTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,
                                             osg::NodeVisitor* nv) const
{
  // This is the fast path, optimize a bit
  if (_referenceFrame == RELATIVE_RF) {
    // FIXME optimize
    osg::Matrix tmp;
    set_rotation(tmp, _angleRad, _center, _axis);
    matrix.preMult(tmp);
  } else {
    osg::Matrix tmp;
    set_rotation(tmp, _angleRad, _center, _axis);
    matrix = tmp;
  }
  return true;
}

bool
SGRotateTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                             osg::NodeVisitor* nv) const
{
  if (_referenceFrame == RELATIVE_RF) {
    // FIXME optimize
    osg::Matrix tmp;
    set_rotation(tmp, -_angleRad, _center, _axis);
    matrix.postMult(tmp);
  } else {
    // FIXME optimize
    osg::Matrix tmp;
    set_rotation(tmp, -_angleRad, _center, _axis);
    matrix = tmp;
  }
  return true;
}

osg::BoundingSphere
SGRotateTransform::computeBound() const
{
  osg::BoundingSphere bs = osg::Group::computeBound();
  osg::BoundingSphere centerbs(_center.osg(), bs.radius());
  centerbs.expandBy(bs);
  return centerbs;
}

