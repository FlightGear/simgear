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


#ifndef _SG_PLACEMENTTRANS_HXX
#define _SG_PLACEMENTTRANS_HXX

#ifndef __cplusplus
# error This library requires C++
#endif

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/math/SGMath.hxx>

#include <osg/Transform>

class SGPlacementTransform : public osg::Transform
{
public:
  
  SGPlacementTransform(void);
  virtual ~SGPlacementTransform(void);

  void setTransform(const SGVec3d& off)
  { _placement_offset = off; dirtyBound(); }
  void setTransform(const SGVec3d& off, const SGMatrixd& rot)
  { _placement_offset = off; _rotation = rot; dirtyBound(); }
  void setSceneryCenter(const SGVec3d& center)
  { _scenery_center = center; dirtyBound(); }

  const SGVec3d& getGlobalPos() const
  { return _placement_offset; }

  virtual bool computeLocalToWorldMatrix(osg::Matrix&,osg::NodeVisitor*) const;
  virtual bool computeWorldToLocalMatrix(osg::Matrix&,osg::NodeVisitor*) const;

private:

  class UpdateCallback;

  //////////////////////////////////////////////////////////////////
  // private data                                                 //
  //////////////////////////////////////////////////////////////////
  
  SGVec3d _placement_offset;
  SGVec3d _scenery_center;
  SGMatrixd _rotation;
};

#endif // _SG_LOCATION_HXX
