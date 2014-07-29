///@file
/// Input device event (keyboard/mouse)
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

#ifndef CANVAS_DEVICE_EVENT_HXX_
#define CANVAS_DEVICE_EVENT_HXX_

#include <simgear/canvas/CanvasEvent.hxx>

namespace osgGA { class GUIEventAdapter; }

namespace simgear
{
namespace canvas
{

  /**
   * Common interface for input device events.
   */
  class DeviceEvent:
    public Event
  {
    public:
      /// Default initialization (no active keyboard modifier).
      DeviceEvent();

      /// Initialize from an OpenSceneGraph event.
      DeviceEvent(const osgGA::GUIEventAdapter& ea);

      /// Get mask of active keyboard modifiers at the time of the event.
      int getModifiers() const { return modifiers; }

      /// Get if a Ctrl modifier was active.
      bool ctrlKey() const;

      /// Get if a Shift modifier was active.
      bool shiftKey() const;

      /// Get if an Alt modifier was active.
      bool altKey() const;

      /// Get if a Meta modifier was active.
      bool metaKey() const;

    protected:
      int modifiers;  //!< Keyboard modifier state
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_DEVICE_EVENT_HXX_ */
