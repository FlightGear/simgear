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

#ifndef SG_TRANSLATE_TRANSFORM_HXX
#define SG_TRANSLATE_TRANSFORM_HXX

#include <osg/Transform>
#include <simgear/math/SGMath.hxx>

class SGTranslateTransform : public osg::Transform {
public:
  SGTranslateTransform();
  SGTranslateTransform(const SGTranslateTransform&,
                       const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

  META_Node(simgear, SGTranslateTransform);
  
  void setAxis(const SGVec3d& axis)
  { _axis = axis; dirtyBound(); }
  const SGVec3d& getAxis() const
  { return _axis; }

  void setValue(double value)
  { _value = value; dirtyBound(); }
  double getValue() const
  { return _value; }

  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const;
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const;
  virtual osg::BoundingSphere computeBound() const;

private:
  SGVec3d _axis;
  double _value;
};

#endif
