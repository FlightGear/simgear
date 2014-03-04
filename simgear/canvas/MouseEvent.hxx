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

  class MouseEvent:
    public Event
  {
    public:
      MouseEvent():
        button(0),
        buttons(0),
        modifiers(0),
        click_count(0)
      {}

      MouseEvent(const osgGA::GUIEventAdapter& ea):
        button(0),
        buttons(ea.getButtonMask()),
        modifiers(ea.getModKeyMask()),
        click_count(0)
      {
        time = ea.getTime();

        // Convert button mask to index
        int button_mask = ea.getButton();
        while( (button_mask >>= 1) > 0 )
          button += 1;
      }

      osg::Vec2f getScreenPos() const { return screen_pos; }
      osg::Vec2f getClientPos() const { return client_pos; }
      osg::Vec2f getLocalPos()  const { return  local_pos; }
      osg::Vec2f getDelta() const { return delta; }

      float getScreenX() const { return screen_pos.x(); }
      float getScreenY() const { return screen_pos.y(); }

      float getClientX() const { return client_pos.x(); }
      float getClientY() const { return client_pos.y(); }

      float getLocalX() const { return local_pos.x(); }
      float getLocalY() const { return local_pos.y(); }

      float getDeltaX() const { return delta.x(); }
      float getDeltaY() const { return delta.y(); }

      int getButton() const { return button; }
      int getButtonMask() const { return buttons; }
      int getModifiers() const { return modifiers; }

      int getCurrentClickCount() const { return click_count; }

      osg::Vec2f  screen_pos,   //<! Position in screen coordinates
                  client_pos,   //<! Position in window/canvas coordinates
                  local_pos,    //<! Position in local/element coordinates
                  delta;
      int         button,       //<! Button for this event
                  buttons,      //<! Current button state
                  modifiers,    //<! Keyboard modifier state
                  click_count;  //<! Current click count
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_MOUSE_EVENT_HXX_ */
