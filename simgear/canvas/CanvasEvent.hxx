/// @file
/// Canvas Event for event model similar to DOM Level 3 Event Model
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

#ifndef CANVAS_EVENT_HXX_
#define CANVAS_EVENT_HXX_

#include "canvas_fwd.hxx"
#include <boost/bimap.hpp>

namespace simgear
{
namespace canvas
{

  /**
   * Base class for all Canvas events.
   *
   * The event system is closely following the specification of the DOM Level 3
   * Event Model.
   */
  class Event:
    public SGReferenced
  {
    public:

      /// Event type identifier
      enum Type
      {
        UNKNOWN,
#       define ENUM_MAPPING(name, str, class_name)\
                 name, /*!< class_name (type=str) */
#         include "CanvasEventTypes.hxx"
#       undef ENUM_MAPPING
        CUSTOM_EVENT ///< First event type id available for user defined event
                     ///  type.
                     /// @see CustomEvent
      };

      int               type;
      ElementWeakPtr    target,
                        current_target;
      double            time;
      bool              propagation_stopped,
                        default_prevented;

      Event();

      // We need a vtable to allow nasal::Ghost to determine the dynamic type
      // of the actual event instances.
      virtual ~Event();

      /**
       * Get whether this events support bubbling
       */
      virtual bool canBubble() const;

      /**
       * Set type of event.
       *
       * If no such type exists it is registered.
       */
      void setType(const std::string& type);

      int getType() const;
      std::string getTypeString() const;

      ElementWeakPtr getTarget() const;
      ElementWeakPtr getCurrentTarget() const;

      /**
       * Get time at which the event was generated.
       */
      double getTime() const;

      /**
       * Prevent further propagation of the event during event flow.
       *
       * @note This does not prevent calling further event handlers registered
       *       on the current event target.
       */
      void stopPropagation();

      /**
       * Cancel any default action normally taken as result of this event.
       *
       * @note For event handlers registered on the DesktopGroup (Nasal:
       *       canvas.getDesktop()) this stops the event from being further
       *       propagated to the normal FlightGear input event handling code.
       */
      void preventDefault();

      /**
       * Get if preventDefault() has been called.
       */
      bool defaultPrevented() const;

      static int getOrRegisterType(const std::string& type);
      static int strToType(const std::string& type);
      static std::string typeToStr(int type);

    protected:
      struct name {};
      struct id {};
      typedef boost::bimaps::bimap<
        boost::bimaps::tagged<std::string, name>,
        boost::bimaps::tagged<int, id>
      > TypeMap;

      static TypeMap& getTypeMap();

  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_EVENT_HXX_ */
