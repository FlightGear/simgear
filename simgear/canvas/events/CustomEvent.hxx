///@file Canvas user defined event
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

#ifndef CANVAS_CUSTOM_EVENT_HXX_
#define CANVAS_CUSTOM_EVENT_HXX_

#include <simgear/canvas/CanvasEvent.hxx>
#include <simgear/structure/map.hxx>

namespace simgear
{
namespace canvas
{

  class CustomEvent:
    public Event
  {
    public:

      /**
       *
       * @param type_str    Event type name (if name does not exist yet it will
       *                    be registered as new event type)
       * @param data        Optional user data stored in event
       */
      CustomEvent( std::string const& type_str,
                   bool bubbles = false,
                   StringMap const& data = StringMap() );

      /**
       *
       * @param type_id     Event type id
       * @param data        Optional user data stored in event
       */
      CustomEvent( int type_id,
                   bool bubbles = false,
                   StringMap const& data = StringMap() );

      /**
       * Set user data
       */
      void setDetail(StringMap const& data);

      /**
       * Get user data
       */
      StringMap const& getDetail() const { return detail; }

      virtual bool canBubble() const { return bubbles; }

      StringMap detail; //<! user data map
      bool bubbles;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_CUSTOM_EVENT_HXX_ */
