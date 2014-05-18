// Manage event handling inside a Canvas similar to the DOM Level 3 Event Model
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

#ifndef CANVAS_EVENT_MANAGER_HXX_
#define CANVAS_EVENT_MANAGER_HXX_

#include "canvas_fwd.hxx"
#include <deque>

namespace simgear
{
namespace canvas
{

  struct EventTarget
  {
    ElementWeakPtr      element;

    // Used as storage by EventManager during event propagation
    mutable osg::Vec2f  local_pos;

    EventTarget( Element* el,
                 const osg::Vec2f pos = osg::Vec2f() ):
      element(el),
      local_pos(pos)
    {}
  };

  inline bool operator==(const EventTarget& t1, const EventTarget& t2)
  {
    return t1.element.lock() == t2.element.lock();
  }

  class EventManager
  {
    public:
      EventManager();

      bool handleEvent( const MouseEventPtr& event,
                        const EventPropagationPath& path );

      bool propagateEvent( const EventPtr& event,
                           const EventPropagationPath& path );

    protected:
      struct StampedPropagationPath
      {
        StampedPropagationPath();
        StampedPropagationPath(const EventPropagationPath& path, double time);

        void clear();
        bool valid() const;

        EventPropagationPath path;
        double time;
      };

      // TODO if we really need the paths modify to not copy around the paths
      //      that much.
      StampedPropagationPath _last_mouse_over;
      size_t _current_click_count;

      struct MouseEventInfo:
        public StampedPropagationPath
      {
        int button;
        osg::Vec2f pos;

        void set( const MouseEventPtr& event,
                  const EventPropagationPath& path );
      } _last_mouse_down,
        _last_click;

      /**
       * Propagate click event and handle multi-click (eg. create dblclick)
       */
      bool handleClick( const MouseEventPtr& event,
                        const EventPropagationPath& path );

      /**
       * Handle mouseover/enter/out/leave
       */
      bool handleMove( const MouseEventPtr& event,
                       const EventPropagationPath& path );

      /**
       * Check if two click events (either mousedown/up or two consecutive
       * clicks) are inside a maximum distance to still create a click or
       * dblclick event respectively.
       */
      bool checkClickDistance( const osg::Vec2f& pos1,
                               const osg::Vec2f& pos2 ) const;
      EventPropagationPath
      getCommonAncestor( const EventPropagationPath& path1,
                         const EventPropagationPath& path2 ) const;
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_EVENT_MANAGER_HXX_ */
