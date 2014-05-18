// Canvas Event for event model similar to DOM Level 3 Event Model
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

  class Event:
    public SGReferenced
  {
    public:

      enum Type
      {
        UNKNOWN,
#       define ENUM_MAPPING(name, str) name,
#         include "CanvasEventTypes.hxx"
#       undef ENUM_MAPPING
        CUSTOM_EVENT ///<! all user defined event types share the same id. They
                     ///   are just differentiated by using the type string.
      };

      int               type;
      ElementWeakPtr    target,
                        current_target;
      double            time;
      bool              propagation_stopped;

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

      double getTime() const;

      void stopPropagation();

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
