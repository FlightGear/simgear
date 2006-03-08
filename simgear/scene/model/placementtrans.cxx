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


#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>
#include <simgear/constants.h>

#include <plib/sg.h>
#include <plib/ssg.h>

#include "placementtrans.hxx"

ssgPlacementTransform::ssgPlacementTransform(void)
{
  sgdSetVec3(_placement_offset, 0, 0, 0);
  sgdSetVec3(_scenery_center, 0, 0, 0);
}

ssgPlacementTransform::~ssgPlacementTransform(void)
{
}

ssgBase *ssgPlacementTransform::clone(int clone_flags)
{
  ssgPlacementTransform *b = new ssgPlacementTransform;
  b->copy_from(this, clone_flags);
  return b;
}

void
ssgPlacementTransform::copy_from(ssgPlacementTransform *src, int clone_flags)
{
  ssgBaseTransform::copy_from(src, clone_flags);
  sgdCopyVec3(_placement_offset, src->_placement_offset);
  sgdCopyVec3(_scenery_center,  src->_scenery_center);
}

void ssgPlacementTransform::setTransform(sgdVec3 off)
{
  sgdCopyVec3(_placement_offset, off);
  sgdVec3 tmp;
  sgdSubVec3(tmp, _placement_offset, _scenery_center);
  sgMat4 tmat;
  sgZeroVec4(tmat[0]);
  tmat[0][0] = 1;
  sgZeroVec4(tmat[1]);
  tmat[1][1] = 1;
  sgZeroVec4(tmat[2]);
  tmat[2][2] = 1;
  sgSetVec3(tmat[3], tmp);
  tmat[3][3] = 1;
  ssgTransform::setTransform(tmat);
}

void ssgPlacementTransform::setTransform(sgdVec3 off, sgMat4 rot)
{
  sgdCopyVec3(_placement_offset, off);
  sgdVec3 tmp;
  sgdSubVec3(tmp, _placement_offset, _scenery_center);
  sgMat4 tmat;
  sgCopyVec4(tmat[0], rot[0]);
  sgCopyVec4(tmat[1], rot[1]);
  sgCopyVec4(tmat[2], rot[2]);
  sgSetVec3(tmat[3], tmp);
  tmat[3][3] = 1;
  ssgTransform::setTransform(tmat);
}

void ssgPlacementTransform::setSceneryCenter(sgdVec3 xyz)
{
  sgdCopyVec3(_scenery_center, xyz);
  sgdVec3 tmp;
  sgdSubVec3(tmp, _placement_offset, _scenery_center);
  sgMat4 tmat;
  getTransform(tmat);
  sgSetVec3(tmat[3], tmp);
  ssgTransform::setTransform(tmat);
}
