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

#include "SGScaleTransform.hxx"

SGScaleTransform::SGScaleTransform() :
  _center(0, 0, 0),
  _scaleFactor(1, 1, 1),
  _boundScale(1)
{
  setReferenceFrame(RELATIVE_RF);
}

bool
SGScaleTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,
                                            osg::NodeVisitor* nv) const
{
  osg::Matrix transform;
  transform(0,0) = _scaleFactor[0];
  transform(1,1) = _scaleFactor[1];
  transform(2,2) = _scaleFactor[2];
  transform(3,0) = _center[0]*(1 - _scaleFactor[0]);
  transform(3,1) = _center[1]*(1 - _scaleFactor[1]);
  transform(3,2) = _center[2]*(1 - _scaleFactor[2]);
  if (_referenceFrame == RELATIVE_RF)
    matrix.preMult(transform);
  else
    matrix = transform;
  return true;
}

bool
SGScaleTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                            osg::NodeVisitor* nv) const
{
  if (fabs(_scaleFactor[0]) < SGLimitsd::min())
    return false;
  if (fabs(_scaleFactor[1]) < SGLimitsd::min())
    return false;
  if (fabs(_scaleFactor[2]) < SGLimitsd::min())
    return false;
  SGVec3d rScaleFactor(1/_scaleFactor[0],
                       1/_scaleFactor[1],
                       1/_scaleFactor[2]);
  osg::Matrix transform;
  transform(0,0) = rScaleFactor[0];
  transform(1,1) = rScaleFactor[1];
  transform(2,2) = rScaleFactor[2];
  transform(3,0) = _center[0]*(1 - rScaleFactor[0]);
  transform(3,1) = _center[1]*(1 - rScaleFactor[1]);
  transform(3,2) = _center[2]*(1 - rScaleFactor[2]);
  if (_referenceFrame == RELATIVE_RF)
    matrix.postMult(transform);
  else
    matrix = transform;
  return true;
}

osg::BoundingSphere
SGScaleTransform::computeBound() const
{
  osg::BoundingSphere bs = osg::Group::computeBound();
  _boundScale = normI(_scaleFactor);
  bs.radius() *= _boundScale;
  return bs;
}
