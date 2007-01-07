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
