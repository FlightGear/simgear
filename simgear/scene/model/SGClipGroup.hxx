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

#ifndef SG_CLIP_GROUP_HXX
#define SG_CLIP_GROUP_HXX

#include <vector>
#include <osg/ref_ptr>
#include <osg/ClipPlane>
#include <osg/Group>
#include <osgUtil/RenderBin>

#include <simgear/math/SGMath.hxx>

class SGClipGroup : public osg::Group {
public:
  SGClipGroup();
  SGClipGroup(const SGClipGroup&,
              const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);
  META_Node(simgear, SGClipGroup);

  virtual osg::BoundingSphere computeBound() const;

  void addClipPlane(unsigned num, const SGVec2d& p0, const SGVec2d& p1);
  void setDrawArea(const SGVec2d& lowerLeft, const SGVec2d& upperRight);
  void setDrawArea(const SGVec2d& bottomLeft,
                   const SGVec2d& topLeft,
                   const SGVec2d& bottomRight,
                   const SGVec2d& topRight);

protected:
  class CullCallback;
  class ClipRenderBin;
  struct ClipBinRegistrar;
  std::vector<osg::ref_ptr<osg::ClipPlane> > mClipPlanes;
};

#endif
