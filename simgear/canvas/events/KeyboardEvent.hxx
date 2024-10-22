// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas keyboard event
 */

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
      KeyboardEvent* clone(int type = 0) const override;

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
