// placementtrans.hxx -- class for carrying transforms for placing models in the world
//
// Written by Mathias Froehlich, started April 2005.
//
// Copyright (C) 2005 Mathias Froehlich
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>
#include <simgear/constants.h>

#include "placementtrans.hxx"

SGPlacementTransform::SGPlacementTransform(void) :
  _placement_offset(0, 0, 0),
  _scenery_center(0, 0, 0),
  _rotation(1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1)
{
}

SGPlacementTransform::~SGPlacementTransform(void)
{
}

bool
SGPlacementTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,
                                                osg::NodeVisitor*) const
{
  osg::Matrix t;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      t(j, i) = _rotation(i, j);
    }
    t(3, i) = _placement_offset(i) - _scenery_center(i);
  }
  
  if (_referenceFrame == RELATIVE_RF)
    matrix.preMult(t);
  else
    matrix = t;
  return true;
}

bool
SGPlacementTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                                osg::NodeVisitor*) const
{
  osg::Matrix t;
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      t(j, i) = _rotation(i, j);
    }
    t(3, i) = _placement_offset(i) - _scenery_center(i);
  }
  t = osg::Matrix::inverse(t);

  if (_referenceFrame == RELATIVE_RF)
    matrix.postMult(t);
  else
    matrix = t;
  return true;
}
