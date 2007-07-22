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

#ifndef SG_OFFSET_TRASNFORM_HXX
#define SG_OFFSET_TRASNFORM_HXX

#include <osg/Transform>

class SGOffsetTransform : public osg::Transform {
public:
  SGOffsetTransform(double scaleFactor = 1.0);
  SGOffsetTransform(const SGOffsetTransform&,
                    const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

  META_Node(simgear, SGOffsetTransform);

  double getScaleFactor() const { return _scaleFactor; };

  void setScaleFactor(double scaleFactor)
  {
    _scaleFactor = scaleFactor;
    _rScaleFactor = 1.0 / scaleFactor;
  }
  
  virtual bool computeLocalToWorldMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const;
  virtual bool computeWorldToLocalMatrix(osg::Matrix& matrix,
                                         osg::NodeVisitor* nv) const;

private:
  double _scaleFactor;
  double _rScaleFactor;
};

#endif
