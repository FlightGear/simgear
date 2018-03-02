///@file
/// Mouse event
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

#include "MouseEvent.hxx"
#include <osgGA/GUIEventAdapter>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  MouseEvent::MouseEvent():
    button(0),
    buttons(0),
    click_count(0)
  {

  }

  //----------------------------------------------------------------------------
  MouseEvent::MouseEvent(const osgGA::GUIEventAdapter& ea):
    DeviceEvent(ea),
    button(0),
    buttons(ea.getButtonMask()),
    click_count(0)
  {
    // Convert button mask to index
    int button_mask = ea.getButton();
    while( (button_mask >>= 1) > 0 )
      button += 1;
  }

  //----------------------------------------------------------------------------
  MouseEvent* MouseEvent::clone(int type) const
  {
    auto event = new MouseEvent(*this);
    event->type = type;
    return event;
  }

  //----------------------------------------------------------------------------
  bool MouseEvent::canBubble() const
  {
    // Check if event supports bubbling
    const Event::Type types_no_bubbling[] = {
      Event::MOUSE_ENTER,
      Event::MOUSE_LEAVE,
    };
    const size_t num_types_no_bubbling = sizeof(types_no_bubbling)
                                       / sizeof(types_no_bubbling[0]);

    for( size_t i = 0; i < num_types_no_bubbling; ++i )
      if( type == types_no_bubbling[i] )
        return false;

    return true;
  }

} // namespace canvas
} // namespace simgear
