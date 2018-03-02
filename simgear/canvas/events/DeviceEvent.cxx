///@file
/// Input device event
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
