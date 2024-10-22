// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Input device event
 */

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
