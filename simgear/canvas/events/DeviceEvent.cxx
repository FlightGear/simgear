// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Input device event
 */

#include <simgear_config.h>

#include "DeviceEvent.hxx"
#include <osgGA/GUIEventAdapter>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  DeviceEvent::DeviceEvent():
    modifiers(0)
  {

  }

  //----------------------------------------------------------------------------
  DeviceEvent::DeviceEvent(const osgGA::GUIEventAdapter& ea):
    modifiers(ea.getModKeyMask())
  {
    time = ea.getTime();
  }

  //----------------------------------------------------------------------------
  bool DeviceEvent::ctrlKey() const
  {
    return (modifiers & osgGA::GUIEventAdapter::MODKEY_CTRL) != 0;
  }

  //----------------------------------------------------------------------------
  bool DeviceEvent::shiftKey() const
  {
    return (modifiers & osgGA::GUIEventAdapter::MODKEY_SHIFT) != 0;
  }

  //----------------------------------------------------------------------------
  bool DeviceEvent::altKey() const
  {
    return (modifiers & osgGA::GUIEventAdapter::MODKEY_ALT) != 0;
  }

  //----------------------------------------------------------------------------
  bool DeviceEvent::metaKey() const
  {
    return (modifiers & osgGA::GUIEventAdapter::MODKEY_META) != 0;
  }

} // namespace canvas
} // namespace simgear
