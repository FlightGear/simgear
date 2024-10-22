// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas mouse event
 */

#ifndef CANVAS_MOUSE_EVENT_HXX_
#define CANVAS_MOUSE_EVENT_HXX_

#include "DeviceEvent.hxx"

namespace simgear
{
namespace canvas
{

  /**
   * Mouse (button/move/wheel) event
   */
  class MouseEvent:
    public DeviceEvent
  {
    public:
      MouseEvent();
      MouseEvent(const osgGA::GUIEventAdapter& ea);
      MouseEvent* clone(int type = 0) const override;

      bool canBubble() const override;

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

      int getCurrentClickCount() const { return click_count; }

      osg::Vec2f  screen_pos,   //!< Position in screen coordinates
                  client_pos,   //!< Position in window/canvas coordinates
                  local_pos,    //!< Position in local/element coordinates
                  delta;
      int         button,       //!< Button for this event
                  buttons,      //!< Current button state
                  click_count;  //!< Current click count
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_MOUSE_EVENT_HXX_ */
