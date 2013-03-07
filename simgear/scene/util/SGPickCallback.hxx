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

#ifndef SG_SCENE_PICKCALLBACK_HXX
#define SG_SCENE_PICKCALLBACK_HXX

#include <osg/Vec2d>
   
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMath.hxx>

namespace osgGA { class GUIEventAdapter; }

// Used to implement scenery interaction.
// The interface is still under development
class SGPickCallback : public SGReferenced {
public:
  enum Priority {
    PriorityGUI = 0,
    PriorityPanel = 1,
    PriorityOther = 2,
    PriorityScenery = 3
  };

  struct Info {
    SGVec3d wgs84;
    SGVec3d local;
  };

  SGPickCallback(Priority priority = PriorityOther) :
    _priority(priority)
  { }

  virtual ~SGPickCallback() {}
  virtual bool buttonPressed(int button, const osgGA::GUIEventAdapter* event, const Info& info)
  { return false; }
  
  virtual void update(double dt, int keyModState)
  { }
    
  virtual void buttonReleased(int keyModState)
  { }

  virtual void mouseMoved(const osgGA::GUIEventAdapter* event)
  { }

  virtual bool hover(const osg::Vec2d& windowPos, const Info& info)
  {  return false; }

  Priority getPriority() const
  { return _priority; }

private:
  Priority _priority;
};

struct SGSceneryPick {
  SGPickCallback::Info info;
  SGSharedPtr<SGPickCallback> callback;
};

#endif
