///@file
/// Keyboard event
//
// Copyright (C) 2014  Thomas Geymayer <tomgey@gmail.com>
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

#ifndef CANVAS_KEYBOARD_EVENT_HXX_
#define CANVAS_KEYBOARD_EVENT_HXX_

#include "DeviceEvent.hxx"

namespace simgear
{
namespace canvas
{

  /**
   * Keyboard (button up/down) event
   */
  class KeyboardEvent:
    public DeviceEvent
  {
    public:

      enum DOMKeyLocation
      {
        DOM_KEY_LOCATION_STANDARD = 0,
        DOM_KEY_LOCATION_LEFT,
        DOM_KEY_LOCATION_RIGHT,
        DOM_KEY_LOCATION_NUMPAD
      };

      KeyboardEvent();
      KeyboardEvent(const osgGA::GUIEventAdapter& ea);

      void setKey(uint32_t key);
      void setUnmodifiedKey(uint32_t key);
      void setRepeat(bool repeat);

      std::string key() const;
      DOMKeyLocation location() const;
      bool repeat() const { return _repeat; }

      uint32_t charCode() const { return _key; }
      uint32_t keyCode() const { return _unmodified_key; }

      /// Whether the key which has triggered this event is a modifier
      bool isModifier() const;

      /// Whether this events represents an input of a printable character
      bool isPrint() const;

    protected:
      uint32_t _key,            //!< Key identifier for this event
               _unmodified_key; //!< Virtual key identifier without any
                                //   modifiers applied
      bool     _repeat;         //!< If key has been depressed long enough to
                                //   generate key repetition

      mutable std::string _name; //!< Printable representation/name
      mutable uint8_t _location; //!< Location of the key on the keyboard

  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_KEYBOARD_EVENT_HXX_ */
