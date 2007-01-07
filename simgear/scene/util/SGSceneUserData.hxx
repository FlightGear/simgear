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

#ifndef SG_SCENE_USERDATA_HXX
#define SG_SCENE_USERDATA_HXX

#include <vector>
#include <osg/Referenced>
#include <osg/Node>
#include "SGPickCallback.hxx"

class SGSceneUserData : public osg::Referenced {
public:
  static SGSceneUserData* getSceneUserData(osg::Node* node);
  static const SGSceneUserData* getSceneUserData(const osg::Node* node);
  static SGSceneUserData* getOrCreateSceneUserData(osg::Node* node);

  /// Access to the pick callbacks of a node.
  unsigned getNumPickCallbacks() const;
  SGPickCallback* getPickCallback(unsigned i = 0) const;
  void setPickCallback(SGPickCallback* pickCallback);
  void addPickCallback(SGPickCallback* pickCallback);
  
private:
  /// Scene interaction callbacks
  std::vector<SGSharedPtr<SGPickCallback> > _pickCallbacks;
};

#endif
