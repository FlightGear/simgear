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

#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Output>

#include <simgear/compiler.h>
#include <simgear/constants.h>

#include "placementtrans.hxx"

#include <simgear/scene/util/SGUpdateVisitor.hxx>

class SGPlacementTransform::UpdateCallback : public osg::NodeCallback {
public:
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    // short circuit updates if the model we place with that branch
    // out of sight.
    // Note that the transform is still correct.
    SGUpdateVisitor* updateVisitor = dynamic_cast<SGUpdateVisitor*>(nv);
    if (updateVisitor) {
      SGPlacementTransform* placementTransform;
      placementTransform = static_cast<SGPlacementTransform*>(node);
      double dist2 = distSqr(updateVisitor->getGlobalEyePos(),
                             placementTransform->getGlobalPos());
      if (updateVisitor->getSqrVisibility() < dist2)
        return;
    }
    // note, callback is responsible for scenegraph traversal so
    // should always include call traverse(node,nv) to ensure 
    // that the rest of cullbacks and the scene graph are traversed.
    traverse(node, nv);
  }
};


SGPlacementTransform::SGPlacementTransform(void) :
  _placement_offset(0, 0, 0),
  _scenery_center(0, 0, 0),
  _rotation(1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1)
{
  setUpdateCallback(new UpdateCallback);
}

SGPlacementTransform::SGPlacementTransform(const SGPlacementTransform& trans,
                                           const osg::CopyOp& copyop):
  osg::Transform(trans, copyop),
  _placement_offset(trans._placement_offset),
  _scenery_center(trans._scenery_center),
  _rotation(trans._rotation)
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

// Functions to read / write SGPlacementTrans from / to a .osg file,
// mostly for debugging purposes.

namespace {

bool PlacementTrans_readLocalData(osg::Object& obj, osgDB::Input& fr)
{
    SGPlacementTransform& trans = static_cast<SGPlacementTransform&>(obj);
    SGMatrixd rotation(1, 0, 0, 0,
                       0, 1, 0, 0,
                       0, 0, 1, 0,
                       0, 0, 0, 1);
    SGVec3d placementOffset(0, 0, 0);
    SGVec3d sceneryCenter(0, 0, 0);
    
    if (fr[0].matchWord("rotation") && fr[1].isOpenBracket()) {
        fr += 2;
        for (int i = 0; i < 3; i++) {
            SGVec3d scratch;
            if (!fr.readSequence(scratch.osg()))
                return false;
            fr += 3;
            for (int j = 0; j < 3; j++)
                rotation(j, i) = scratch[j];
        }
        if (fr[0].isCloseBracket())
            ++fr;
        else
            return false;
    }
    if (fr[0].matchWord("placement")) {
        ++fr;
        if (fr.readSequence(placementOffset.osg()))
            fr += 3;
        else
            return false;
    }
    if (fr[0].matchWord("sceneryCenter")) {
        ++fr;
        if (fr.readSequence(sceneryCenter.osg()))
            fr += 3;
        else
            return false;
    }
    trans.setTransform(placementOffset, rotation);
    trans.setSceneryCenter(sceneryCenter);
    return true;
}

bool PlacementTrans_writeLocalData(const osg::Object& obj, osgDB::Output& fw)
{
    const SGPlacementTransform& trans
        = static_cast<const SGPlacementTransform&>(obj);
    const SGMatrixd& rotation = trans.getRotation();
    const SGVec3d& placement = trans.getGlobalPos();
    const SGVec3d& sceneryCenter = trans.getSceneryCenter();
    
    fw.indent() << "rotation {" << std::endl;
    fw.moveIn();
    for (int i = 0; i < 3; i++) {
        fw.indent();
        for (int j = 0; j < 3; j++) {
            fw << rotation(j, i) << " ";
        }
        fw << std::endl;
    }
    fw.moveOut();
    fw.indent() << "}" << std::endl;
    int prec = fw.precision();
    fw.precision(15);
    fw.indent() << "placement ";
    for (int i = 0; i < 3; i++) {
        fw << placement(i) << " ";
    }
    fw << std::endl;
    fw.indent() << "sceneryCenter ";
    for (int i = 0; i < 3; i++) {
        fw << sceneryCenter(i) << " ";
    }
    fw << std::endl;
    fw.precision(prec);
    return true;
}
}

osgDB::RegisterDotOsgWrapperProxy g_SGPlacementTransProxy
(
    new SGPlacementTransform,
    "SGPlacementTransform",
    "Object Node Transform SGPlacementTransform Group",
    &PlacementTrans_readLocalData,
    &PlacementTrans_writeLocalData
);
