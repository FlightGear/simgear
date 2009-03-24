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
  _rotation(SGQuatd::unit())
{
  setUpdateCallback(new UpdateCallback);
}

SGPlacementTransform::SGPlacementTransform(const SGPlacementTransform& trans,
                                           const osg::CopyOp& copyop):
  osg::Transform(trans, copyop),
  _placement_offset(trans._placement_offset),
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
  if (_referenceFrame == RELATIVE_RF) {
    matrix.preMultTranslate(_placement_offset.osg());
    matrix.preMultRotate(_rotation.osg());
  } else {
    matrix.makeRotate(_rotation.osg());
    matrix.postMultTranslate(_placement_offset.osg());
  }
  return true;
}

bool
SGPlacementTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                                osg::NodeVisitor*) const
{
  if (_referenceFrame == RELATIVE_RF) {
    matrix.postMultTranslate(-_placement_offset.osg());
    matrix.postMultRotate(inverse(_rotation).osg());
  } else {
    matrix.makeRotate(inverse(_rotation).osg());
    matrix.preMultTranslate(-_placement_offset.osg());
  }
  return true;
}

// Functions to read / write SGPlacementTrans from / to a .osg file,
// mostly for debugging purposes.

namespace {

bool PlacementTrans_readLocalData(osg::Object& obj, osgDB::Input& fr)
{
    SGPlacementTransform& trans = static_cast<SGPlacementTransform&>(obj);
    SGQuatd rotation = SGQuatd::unit();
    SGVec3d placementOffset(0, 0, 0);
    
    if (fr[0].matchWord("rotation")) {
        ++fr;
        osg::Vec4d vec4;
        if (fr.readSequence(vec4)) {
            rotation = SGQuatd(vec4[0], vec4[1], vec4[2], vec4[3]);
            fr += 4;
        } else
            return false;
    }
    if (fr[0].matchWord("placement")) {
        ++fr;
        if (fr.readSequence(placementOffset.osg()))
            fr += 3;
        else
            return false;
    }
    trans.setTransform(placementOffset, rotation);
    return true;
}

bool PlacementTrans_writeLocalData(const osg::Object& obj, osgDB::Output& fw)
{
    const SGPlacementTransform& trans
        = static_cast<const SGPlacementTransform&>(obj);
    const SGQuatd& rotation = trans.getRotation();
    const SGVec3d& placement = trans.getGlobalPos();
    
    fw.indent() << "rotation ";
    for (int i = 0; i < 4; i++) {
        fw << rotation(i) << " ";
    }
    fw << std::endl;
    int prec = fw.precision();
    fw.precision(15);
    fw.indent() << "placement ";
    for (int i = 0; i < 3; i++) {
        fw << placement(i) << " ";
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
