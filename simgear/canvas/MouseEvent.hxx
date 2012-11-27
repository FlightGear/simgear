// Mouse event
//
// Copyright (C) 2012  Thomas Geymayer <tomgey@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef CANVAS_MOUSE_EVENT_HXX_
#define CANVAS_MOUSE_EVENT_HXX_

#include "CanvasEvent.hxx"
#include <osgGA/GUIEventAdapter>

namespace simgear
{
namespace canvas
{

  struct MouseEvent:
    public Event
  {
    MouseEvent():
      button(-1),
      state(-1),
      mod(-1)
    {}

    osg::Vec2f getPos() const { return pos; }
    osg::Vec3f getPos3() const { return osg::Vec3f(pos, 0); }
    osg::Vec2f getDelta() const { return delta; }

    osg::Vec2f  pos,
                delta;
    int         button, //<! Button for this event
                state,  //<! Current button state
                mod;    //<! Keyboard modifier state
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_MOUSE_EVENT_HXX_ */
