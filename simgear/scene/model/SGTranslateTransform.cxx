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

#include "SGTranslateTransform.hxx"

SGTranslateTransform::SGTranslateTransform() :
  _axis(0, 0, 0),
  _value(0)
{
  setReferenceFrame(RELATIVE_RF);
}

SGTranslateTransform::SGTranslateTransform(const SGTranslateTransform& trans,
                                           const osg::CopyOp& copyop) :
  osg::Transform(trans, copyop),
  _axis(trans._axis),
  _value(trans._value)
{
}

bool
SGTranslateTransform::computeLocalToWorldMatrix(osg::Matrix& matrix,
                                                osg::NodeVisitor* nv) const 
{
  if (_referenceFrame == RELATIVE_RF) {
    matrix.preMultTranslate(toOsg(_value*_axis));
  } else {
    matrix.setTrans(toOsg(_value*_axis));
  }
  return true;
}

bool
SGTranslateTransform::computeWorldToLocalMatrix(osg::Matrix& matrix,
                                                osg::NodeVisitor* nv) const
{
  if (_referenceFrame == RELATIVE_RF) {
    matrix.postMultTranslate(toOsg(-_value*_axis));
  } else {
    matrix.setTrans(toOsg(-_value*_axis));
  }
  return true;
}

osg::BoundingSphere
SGTranslateTransform::computeBound() const
{
  osg::BoundingSphere bs = osg::Group::computeBound();
  bs._center += toOsg(_axis)*_value;
  return bs;
}

namespace {

bool TranslateTransform_readLocalData(osg::Object& obj, osgDB::Input& fr)
{
    SGTranslateTransform& trans = static_cast<SGTranslateTransform&>(obj);

    if (fr[0].matchWord("axis")) {
        ++fr;
        osg::Vec3d axis;
        if (fr.readSequence(axis))
            fr += 3;
        else
            return false;
        trans.setAxis(toSG(axis));
    }
    if (fr[0].matchWord("value")) {
        ++fr;
        double value;
        if (fr[0].getFloat(value))
            ++fr;
        else
            return false;
        trans.setValue(value);
    }
    return true;
}

bool TranslateTransform_writeLocalData(const osg::Object& obj,
                                       osgDB::Output& fw)
{
    const SGTranslateTransform& trans
        = static_cast<const SGTranslateTransform&>(obj);
    const SGVec3d& axis = trans.getAxis();
    double value = trans.getValue();
    
    fw.indent() << "axis ";
    for (int i = 0; i < 3; i++)
        fw << axis(i) << " ";
    fw << std::endl;
    fw.indent() << "value " << value << std::endl;
    return true;
}

}

osgDB::RegisterDotOsgWrapperProxy g_SGTranslateTransformProxy
(
    new SGTranslateTransform,
    "SGTranslateTransform",
    "Object Node Transform SGTranslateTransform Group",
    &TranslateTransform_readLocalData,
    &TranslateTransform_writeLocalData
);
