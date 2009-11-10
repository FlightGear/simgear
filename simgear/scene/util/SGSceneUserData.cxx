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
#include <osgDB/ParameterOutput>

#include "SGSceneUserData.hxx"

SGSceneUserData*
SGSceneUserData::getSceneUserData(osg::Node* node)
{
  if (!node)
    return 0;
  osg::Referenced* referenced = node->getUserData();
  if (!referenced)
    return 0;
  return dynamic_cast<SGSceneUserData*>(referenced);
}

const SGSceneUserData*
SGSceneUserData::getSceneUserData(const osg::Node* node)
{
  if (!node)
    return 0;
  const osg::Referenced* referenced = node->getUserData();
  if (!referenced)
    return 0;
  return dynamic_cast<const SGSceneUserData*>(referenced);
}

SGSceneUserData*
SGSceneUserData::getOrCreateSceneUserData(osg::Node* node)
{
  SGSceneUserData* userData = getSceneUserData(node);
  if (userData)
    return userData;
  userData = new SGSceneUserData;
  node->setUserData(userData);
  return userData;
}

unsigned
SGSceneUserData::getNumPickCallbacks() const
{
  return _pickCallbacks.size();
}

SGPickCallback*
SGSceneUserData::getPickCallback(unsigned i) const
{
  if (_pickCallbacks.size() <= i)
    return 0;
  return _pickCallbacks[i];
}

void
SGSceneUserData::setPickCallback(SGPickCallback* pickCallback)
{
  _pickCallbacks.clear();
  addPickCallback(pickCallback);
}

void
SGSceneUserData::addPickCallback(SGPickCallback* pickCallback)
{
  if (!pickCallback)
    return;
  _pickCallbacks.push_back(pickCallback);
}

bool SGSceneUserData_writeLocalData(const osg::Object& obj, osgDB::Output& fw)
{
    const SGSceneUserData& data = static_cast<const SGSceneUserData&>(obj);

    unsigned numPickCallbacks = data.getNumPickCallbacks();
    if (numPickCallbacks > 0)
        fw.indent() << "num_pickCallbacks " << numPickCallbacks << "\n";
    if (data.getBVHNode())
        fw.indent() << "hasBVH true\n";
    const SGSceneUserData::Velocity* vel = data.getVelocity();
    if (vel) {
        fw.indent() << "velocity {\n";
        fw.moveIn();
        fw.indent() << "linear " << vel->linear << "\n";
        fw.indent() << "angular " << vel->angular << "\n";
        fw.indent() << "referenceTime " << vel->referenceTime << "\n";
        fw.indent() << "id " << static_cast<unsigned>(vel->id) << "\n";
        fw.moveOut();
        fw.indent() << "}\n";
    }
    return true;
}

namespace
{
osgDB::RegisterDotOsgWrapperProxy SGSceneUserDataProxy
(
    new SGSceneUserData,
    "simgear::SGSceneUserData",
    "Object simgear::SGSceneUserData",
    0,
    &SGSceneUserData_writeLocalData
    );
}
