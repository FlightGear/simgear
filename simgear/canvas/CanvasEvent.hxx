// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Canvas Event event model similar to DOM Level 3 Event Model
 */

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
       * Clone event and set to the given type (Same type if not specified)
       */
      virtual Event* clone(int type = 0) const = 0;

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

      bool isPropagationStopped() const;

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

      /**
       * Register a new type string or get the id of an existing type string
       *
       * @param type    Type string
       * @return Id of the given @a type
       */
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
