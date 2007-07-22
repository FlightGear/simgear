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

#include "SGOffsetTransform.hxx"

SGOffsetTransform::SGOffsetTransform(double scaleFactor) :
  _scaleFactor(scaleFactor),
  _rScaleFactor(1/scaleFactor)
{
}

SGOffsetTransform::SGOffsetTransform(const SGOffsetTransform& offset,
                                     const osg::CopyOp& copyop) :
    osg::Transform(offset, copyop),
    _scaleFactor(offset._scaleFactor),
    _rScaleFactor(offset._rScaleFactor)
{
}

bool
SGOffsetTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,
                                             osg::NodeVisitor* nv) const
{
  if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR) {
    osg::Vec3 center = nv->getEyePoint();
    osg::Matrix transform;
    transform(0,0) = _scaleFactor;
    transform(1,1) = _scaleFactor;
    transform(2,2) = _scaleFactor;
    transform(3,0) = center[0]*(1 - _scaleFactor);
    transform(3,1) = center[1]*(1 - _scaleFactor);
    transform(3,2) = center[2]*(1 - _scaleFactor);
    matrix.preMult(transform);
  }
  return true;
}

bool
SGOffsetTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                             osg::NodeVisitor* nv) const
{
  if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR) {
    osg::Vec3 center = nv->getEyePoint();
    osg::Matrix transform;
    transform(0,0) = _rScaleFactor;
    transform(1,1) = _rScaleFactor;
    transform(2,2) = _rScaleFactor;
    transform(3,0) = center[0]*(1 - _rScaleFactor);
    transform(3,1) = center[1]*(1 - _rScaleFactor);
    transform(3,2) = center[2]*(1 - _rScaleFactor);
    matrix.postMult(transform);
  }
  return true;
}

namespace {

bool OffsetTransform_readLocalData(osg::Object& obj, osgDB::Input& fr)
{
    SGOffsetTransform& offset = static_cast<SGOffsetTransform&>(obj);
    if (fr[0].matchWord("scaleFactor")) {
        ++fr;
        double scaleFactor;
        if (fr[0].getFloat(scaleFactor))
            ++fr;
        else
            return false;
        offset.setScaleFactor(scaleFactor);
    }
    return true;
}

bool OffsetTransform_writeLocalData(const osg::Object& obj, osgDB::Output& fw)
{
    const SGOffsetTransform& offset
        = static_cast<const SGOffsetTransform&>(obj);
    fw.indent() << "scaleFactor " << offset.getScaleFactor() << std::endl;
    return true;
}

}

osgDB::RegisterDotOsgWrapperProxy g_SGOffsetTransformProxy
(
    new SGOffsetTransform,
    "SGOffsetTransform",
    "Object Node Transform SGOffsetTransform Group",
    &OffsetTransform_readLocalData,
    &OffsetTransform_writeLocalData
);
