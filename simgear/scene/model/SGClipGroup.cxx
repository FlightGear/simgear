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

#include "SGClipGroup.hxx"

#include <osg/ClipPlane>
#include <osg/NodeCallback>
#include <osg/StateSet>
#include <osg/Version>

#include <osgUtil/RenderBin>
#include <osgUtil/RenderLeaf>
#include <osgUtil/CullVisitor>

class SGClipGroup::ClipRenderBin : public osgUtil::RenderBin {
public:
  virtual osg::Object* cloneType() const
  { return new ClipRenderBin(); }
  virtual osg::Object* clone(const osg::CopyOp& copyop) const
  { return new ClipRenderBin; }
  virtual bool isSameKindAs(const osg::Object* obj) const
  { return dynamic_cast<const ClipRenderBin*>(obj)!=0L; }
  virtual const char* libraryName() const
  { return "SimGear"; }
  virtual const char* className() const
  { return "ClipRenderBin"; }

  virtual void drawImplementation(osg::RenderInfo& renderInfo,
                                  osgUtil::RenderLeaf*& previous)
  {
    osg::State* state = renderInfo.getState();

    state->applyModelViewMatrix(mModelView.get());
    for (unsigned i = 0; i < mClipPlanes.size(); ++i) {
      osg::StateAttribute::GLMode planeNum;
      planeNum = GL_CLIP_PLANE0 + mClipPlanes[i]->getClipPlaneNum();
      state->applyMode(planeNum, false);
      glClipPlane(planeNum, mClipPlanes[i]->getClipPlane().ptr());
    }

    osgUtil::RenderBin::drawImplementation(renderInfo, previous);
  }

  virtual void reset()
  { mClipPlanes.resize(0); }
 
  std::vector<osg::ref_ptr<osg::ClipPlane> > mClipPlanes;
  osg::ref_ptr<osg::RefMatrix> mModelView;
};

struct SGClipGroup::ClipBinRegistrar
{
    ClipBinRegistrar()
    {
        osgUtil::RenderBin
            ::addRenderBinPrototype("ClipRenderBin",
                                    new SGClipGroup::ClipRenderBin);
    }
    static ClipBinRegistrar registrar;
};

SGClipGroup::ClipBinRegistrar SGClipGroup::ClipBinRegistrar::registrar;

class SGClipGroup::CullCallback : public osg::NodeCallback {
public:
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    osgUtil::CullVisitor* cullVisitor;
    cullVisitor = dynamic_cast<osgUtil::CullVisitor*>(nv);
    
    if (cullVisitor) {
      osgUtil::RenderBin* renderBin = cullVisitor->getCurrentRenderBin();
      ClipRenderBin* clipBin = dynamic_cast<ClipRenderBin*>(renderBin);
      SGClipGroup* clipGroup;
      clipGroup = dynamic_cast<SGClipGroup*>(node);
      if (clipGroup && clipBin) {
        clipBin->mClipPlanes = clipGroup->mClipPlanes;
        clipBin->mModelView = cullVisitor->getModelViewMatrix();
      }
    }
    
    // note, callback is responsible for scenegraph traversal so
    // they must call traverse(node,nv) to ensure that the
    // scene graph subtree (and associated callbacks) are traversed.
    traverse(node, nv);
  }
};

#if 0
static osg::Vec4d clipPlane(const osg::Vec2& p0, const osg::Vec2& p1)
{
  osg::Vec2d v(p1[0] - p0[0], p1[1] - p0[1]);
  return osg::Vec4d(v[1], -v[0], 0, v[0]*p0[1] - v[1]*p0[0]);
}
#endif

SGClipGroup::SGClipGroup()
{
  getOrCreateStateSet()->setRenderBinDetails(0, "ClipRenderBin");
  setCullCallback(new CullCallback);
}

SGClipGroup::SGClipGroup(const SGClipGroup& clip, const osg::CopyOp& copyop) :
  osg::Group(clip, copyop)
{
  for (unsigned i = 0; i < mClipPlanes.size(); ++i) {
    osg::StateAttribute* sa = copyop(mClipPlanes[i].get());
    mClipPlanes.push_back(static_cast<osg::ClipPlane*>(sa));
  }
}

osg::BoundingSphere
SGClipGroup::computeBound() const
{
  return _initialBound;
}

void
SGClipGroup::addClipPlane(unsigned num, const SGVec2d& p0,
                          const SGVec2d& p1)
{
  osg::Vec2d v(p1[0] - p0[0], p1[1] - p0[1]);
  osg::Vec4d planeEquation(v[1], -v[0], 0, v[0]*p0[1] - v[1]*p0[0]);
  osg::ClipPlane* clipPlane = new osg::ClipPlane(num, planeEquation);
  getStateSet()->setAssociatedModes(clipPlane, osg::StateAttribute::ON);
  mClipPlanes.push_back(clipPlane);
}

void
SGClipGroup::setDrawArea(const SGVec2d& lowerLeft,
                         const SGVec2d& upperRight)
{
  setDrawArea(lowerLeft,
              SGVec2d(lowerLeft[0], upperRight[1]),
              SGVec2d(upperRight[0], lowerLeft[1]),
              upperRight);
}

void
SGClipGroup::setDrawArea(const SGVec2d& bottomLeft,
                         const SGVec2d& topLeft,
                         const SGVec2d& bottomRight,
                         const SGVec2d& topRight)
{
#if (OPENSCENEGRAPH_MAJOR_VERSION > 2) || (OPENSCENEGRAPH_MINOR_VERSION > 2)
  for (unsigned i = 0; i < mClipPlanes.size(); ++i)
    getStateSet()->removeAssociatedModes(mClipPlanes[i].get());
#endif
  mClipPlanes.resize(0);
  addClipPlane(2, bottomLeft, topLeft);
  addClipPlane(3, topLeft, topRight);
  addClipPlane(4, topRight, bottomRight);
  addClipPlane(5, bottomRight, bottomLeft);
  _initialBound.init();
  _initialBound.expandBy(osg::Vec3(bottomLeft[0], bottomLeft[1], 0));
  _initialBound.expandBy(osg::Vec3(topLeft[0], topLeft[1], 0));
  _initialBound.expandBy(osg::Vec3(bottomRight[0], bottomRight[1], 0));
  _initialBound.expandBy(osg::Vec3(topRight[0], topRight[1], 0));
  _boundingSphere = _initialBound;
  _boundingSphereComputed = true;
}

