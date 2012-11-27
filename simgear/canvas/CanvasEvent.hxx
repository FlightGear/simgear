// Canvas Event for event model similar to DOM Level 2 Event Model
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

namespace simgear
{
namespace canvas
{

  struct Event
  {
    public:

      enum Type
      {
        UNKNOWN,
#       define ENUM_MAPPING(name, str) name,
#         include "CanvasEventTypes.hxx"
#       undef ENUM_MAPPING
        USER_TYPE ///<! first unused id to be used for user defined types (not
                  ///   implemented yet)
      };

      Type              type;
      ElementWeakPtr    target;

      Event();
      Type getType() const;
      std::string getTypeString() const;
      ElementWeakPtr getTarget() const;

  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_EVENT_HXX_ */
