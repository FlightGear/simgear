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

#include "SGTranslateTransform.hxx"

static inline void
set_translation (osg::Matrix &matrix, double position_m, const SGVec3d &axis)
{
  SGVec3d xyz = axis * position_m;
  matrix.makeIdentity();
  matrix(3, 0) = xyz[0];
  matrix(3, 1) = xyz[1];
  matrix(3, 2) = xyz[2];
}

SGTranslateTransform::SGTranslateTransform() :
  _axis(0, 0, 0),
  _value(0)
{
  setReferenceFrame(RELATIVE_RF);
}

bool
SGTranslateTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,
                                                osg::NodeVisitor* nv) const 
{
  if (_referenceFrame == RELATIVE_RF) {
    osg::Matrix tmp;
    set_translation(tmp, _value, _axis);
    matrix.preMult(tmp);
  } else {
    osg::Matrix tmp;
    set_translation(tmp, _value, _axis);
    matrix = tmp;
  }
  return true;
}

bool
SGTranslateTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                                osg::NodeVisitor* nv) const
{
  if (_referenceFrame == RELATIVE_RF) {
    osg::Matrix tmp;
    set_translation(tmp, -_value, _axis);
    matrix.postMult(tmp);
  } else {
    osg::Matrix tmp;
    set_translation(tmp, -_value, _axis);
    matrix = tmp;
  }
  return true;
}

osg::BoundingSphere
SGTranslateTransform::computeBound() const
{
  osg::BoundingSphere bs = osg::Group::computeBound();
  bs._center += _axis.osg()*_value;
  return bs;
}
