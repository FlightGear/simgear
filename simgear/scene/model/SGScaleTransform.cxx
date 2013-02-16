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

#include <osgDB/Registry>
#include <osgDB/Input>
#include <osgDB/Output>

#include <simgear/scene/util/OsgMath.hxx>

#include "SGScaleTransform.hxx"

SGScaleTransform::SGScaleTransform() :
  _center(0, 0, 0),
  _scaleFactor(1, 1, 1),
  _boundScale(1)
{
  setReferenceFrame(RELATIVE_RF);
  // see osg::Transform doc: If the transformation matrix scales the subgraph
  // then the normals of the underlying geometry will need to be renormalized
  // to be unit vectors once more.
  osg::StateSet* stateset = getOrCreateStateSet();
  stateset->setMode(GL_NORMALIZE, osg::StateAttribute::ON);
}

SGScaleTransform::SGScaleTransform(const SGScaleTransform& scale,
                                   const osg::CopyOp& copyop) :
    osg::Transform(scale, copyop),
    _center(scale._center),
    _scaleFactor(scale._scaleFactor),
    _boundScale(scale._boundScale)
{
}

bool
SGScaleTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,
                                            osg::NodeVisitor* nv) const
{
  osg::Matrix transform;
  transform(0,0) = _scaleFactor[0];
  transform(1,1) = _scaleFactor[1];
  transform(2,2) = _scaleFactor[2];
  transform(3,0) = _center[0]*(1 - _scaleFactor[0]);
  transform(3,1) = _center[1]*(1 - _scaleFactor[1]);
  transform(3,2) = _center[2]*(1 - _scaleFactor[2]);
  if (_referenceFrame == RELATIVE_RF)
    matrix.preMult(transform);
  else
    matrix = transform;
  return true;
}

bool
SGScaleTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                            osg::NodeVisitor* nv) const
{
  if (fabs(_scaleFactor[0]) < SGLimitsd::min())
    return false;
  if (fabs(_scaleFactor[1]) < SGLimitsd::min())
    return false;
  if (fabs(_scaleFactor[2]) < SGLimitsd::min())
    return false;
  SGVec3d rScaleFactor(1/_scaleFactor[0],
                       1/_scaleFactor[1],
                       1/_scaleFactor[2]);
  osg::Matrix transform;
  transform(0,0) = rScaleFactor[0];
  transform(1,1) = rScaleFactor[1];
  transform(2,2) = rScaleFactor[2];
  transform(3,0) = _center[0]*(1 - rScaleFactor[0]);
  transform(3,1) = _center[1]*(1 - rScaleFactor[1]);
  transform(3,2) = _center[2]*(1 - rScaleFactor[2]);
  if (_referenceFrame == RELATIVE_RF)
    matrix.postMult(transform);
  else
    matrix = transform;
  return true;
}

osg::BoundingSphere
SGScaleTransform::computeBound() const
{
  osg::BoundingSphere bs = osg::Group::computeBound();
  _boundScale = normI(_scaleFactor);
  bs.radius() *= _boundScale;
  return bs;
}

namespace {

bool ScaleTransform_readLocalData(osg::Object& obj, osgDB::Input& fr)
{
    SGScaleTransform& scale = static_cast<SGScaleTransform&>(obj);
    if (fr[0].matchWord("center")) {
        ++fr;
        osg::Vec3d center;
        if (fr.readSequence(center))
            fr += 3;
        else
            return false;
        scale.setCenter(toSG(center));
    }
    if (fr[0].matchWord("scaleFactor")) {
        ++fr;
        osg::Vec3d scaleFactor;
        if (fr.readSequence(scaleFactor))
            fr += 3;
        else
            return false;
        scale.setScaleFactor(toSG(scaleFactor));
    }
    return true;
}

bool ScaleTransform_writeLocalData(const osg::Object& obj, osgDB::Output& fw)
{
    const SGScaleTransform& scale = static_cast<const SGScaleTransform&>(obj);
    const SGVec3d& center = scale.getCenter();
    const SGVec3d& scaleFactor = scale.getScaleFactor();
    int prec = fw.precision();
    fw.precision(15);
    fw.indent() << "center ";
    for (int i = 0; i < 3; i++)
        fw << center(i) << " ";
    fw << std::endl;
    fw.precision(prec);
    fw.indent() << "scaleFactor ";
    for (int i = 0; i < 3; i++)
        fw << scaleFactor(i) << " ";
    fw << std::endl;
    return true;
}

}

osgDB::RegisterDotOsgWrapperProxy g_ScaleTransformProxy
(
    new SGScaleTransform,
    "SGScaleTransform",
    "Object Node Transform SGScaleTransform Group",
    &ScaleTransform_readLocalData,
    &ScaleTransform_writeLocalData
);
