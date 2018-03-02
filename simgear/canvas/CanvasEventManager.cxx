///@file
/// Manage event handling inside a Canvas similar to the DOM Level 3 Event Model
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

#include <simgear_config.h>

#include "CanvasEventManager.hxx"
#include "elements/CanvasElement.hxx"
#include "events/MouseEvent.hxx"

#include <cmath>

namespace simgear
{
namespace canvas
{

  const unsigned int drag_threshold = 8;
  const double multi_click_timeout = 0.4;

  //----------------------------------------------------------------------------
  EventManager::StampedPropagationPath::StampedPropagationPath():
    time(0)
  {

  }

  //----------------------------------------------------------------------------

  EventManager::StampedPropagationPath::StampedPropagationPath(
    const EventPropagationPath& path,
    double time
  ):
    path(path),
    time(time)
  {

  }

  //----------------------------------------------------------------------------
  void EventManager::StampedPropagationPath::clear()
  {
    path.clear();
    time = 0;
  }

  //----------------------------------------------------------------------------
  bool EventManager::StampedPropagationPath::valid() const
  {
    return !path.empty() && time > 0;
  }

  //----------------------------------------------------------------------------
  void EventManager::MouseEventInfo::set( const MouseEventPtr& event,
                                          const EventPropagationPath& p )
  {
    path = p;
    time = event->time;
    button = event->button;
    pos = event->screen_pos;
  }

  //----------------------------------------------------------------------------
  EventManager::EventManager():
    _current_click_count(0)
  {

  }

  //----------------------------------------------------------------------------
  bool EventManager::handleEvent( const MouseEventPtr& event,
                                  const EventPropagationPath& path )
  {
    bool handled = false;
    switch( event->type )
    {
      case Event::MOUSE_DOWN:
        _last_mouse_down.set(event, path);
        break;
      case Event::MOUSE_UP:
      {
        // If the mouse has moved while a button was down (aka. dragging) we
        // need to notify the original element that the mouse has left it, and
        // the new element that it has been entered
        if( _last_mouse_down.path != path )
          handled |= handleMove(event, path);

        // normal mouseup
        handled |= propagateEvent(event, path);

        if( !_last_mouse_down.valid() )
          // Ignore mouse up without any previous mouse down
          return handled;

        // now handle click/dblclick
        if( checkClickDistance(event->screen_pos, _last_mouse_down.pos) )
          handled |=
            handleClick(event, getCommonAncestor(_last_mouse_down.path, path));

        _last_mouse_down.clear();

        return handled;
      }
      case Event::DRAG:
      case Event::DRAG_START:
      case Event::DRAG_END:
        if( !_last_mouse_down.valid() )
          return false;
        else
        {
          // OSG does not set button for drag events.
          event->button = _last_mouse_down.button;
          return propagateEvent(event, _last_mouse_down.path);
        }
      case Event::MOUSE_MOVE:
        handled |= handleMove(event, path);
        break;
      case Event::MOUSE_LEAVE:
        // Mouse leaves window and therefore also current mouseover element
        handleMove(event, EventPropagationPath());

        // Event is only send if mouse is moved outside the window or dragging
        // has ended somewhere outside the window. In both cases a mouse button
        // has been released, so no more mouse down or click...
        _last_mouse_down.clear();
        _last_click.clear();

        return true;
      case Event::WHEEL:
        break;
      default:
        return false;
    }

    return handled | propagateEvent(event, path);
  }

  //----------------------------------------------------------------------------
  bool EventManager::propagateEvent( const EventPtr& event,
                                     const EventPropagationPath& path )
  {
    event->target = path.back().element;
    MouseEventPtr mouse_event = dynamic_cast<MouseEvent*>(event.get());

    // Event propagation similar to DOM Level 3 event flow:
    // http://www.w3.org/TR/DOM-Level-3-Events/#event-flow

    // Position update only needed for drag event (as event needs to be
    // delivered to element of initial mousedown, but with update positions)
    if( mouse_event && mouse_event->type == MouseEvent::DRAG )
    {
      osg::Vec2f local_pos = mouse_event->client_pos;

      // Capturing phase (currently just update position)
      for( EventPropagationPath::const_iterator it = path.begin();
                                                it != path.end();
                                              ++it )
      {
        ElementPtr el = it->element.lock();
        if( !el )
          continue;

        it->local_pos = local_pos = el->posToLocal(local_pos);
      }
    }

    bool const do_bubble = event->canBubble();

    // Bubbling phase
    for( EventPropagationPath::const_reverse_iterator
           it = path.rbegin();
           it != path.rend();
         ++it )
    {
      ElementPtr el = it->element.lock();

      if( !el )
      {
        // Ignore element if it has been destroyed while traversing the event
        // (eg. removed by another event handler)
        if( do_bubble )
          continue;
        else
          break;
      }

      // TODO provide functions to convert delta to local coordinates on demand.
      //      Maybe also provide a clone method for events as local coordinates
      //      might differ between different elements receiving the same event.
      if( mouse_event )
        mouse_event->local_pos = it->local_pos;

      event->current_target = el;
      el->handleEvent(event);

      if( event->propagation_stopped || !do_bubble )
        return true;
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool EventManager::handleClick( const MouseEventPtr& event,
                                  const EventPropagationPath& path )
  {
    MouseEventPtr click(new MouseEvent(*event));
    click->type = Event::CLICK;

    if( event->getTime() > _last_click.time + multi_click_timeout )
      _current_click_count = 1;
    else
    {
      // Maximum current click count is 3
      _current_click_count = (_current_click_count % 3) + 1;

      if( _current_click_count > 1 )
      {
        // Reset current click count if moved too far or different button has
        // been clicked
        if(   !checkClickDistance(event->screen_pos, _last_click.pos)
            || _last_click.button != event->button )
          _current_click_count = 1;
      }
    }

    click->click_count = _current_click_count;

    MouseEventPtr dbl_click;
    if( _current_click_count == 2 )
    {
      dbl_click.reset(new MouseEvent(*click));
      dbl_click->type = Event::DBL_CLICK;
    }

    bool handled = propagateEvent(click, path);

    if( dbl_click )
      handled |= propagateEvent( dbl_click,
                                 getCommonAncestor(_last_click.path, path) );

    _last_click.set(event, path);

    return handled;
  }

  //----------------------------------------------------------------------------
  bool EventManager::handleMove( const MouseEventPtr& event,
                                 const EventPropagationPath& path )
  {
    EventPropagationPath& last_path = _last_mouse_over.path;
    if( last_path == path )
      return false;

    bool handled = false;

    // Leave old element
    if( !last_path.empty() )
    {
      MouseEventPtr mouseout(new MouseEvent(*event));
      mouseout->type = Event::MOUSE_OUT;
      handled |= propagateEvent(mouseout, last_path);

      // Send a mouseleave event to all ancestors of the currently left element
      // which are not ancestor of the new element currently entered
      EventPropagationPath path_leave = last_path;
      for(size_t i = path_leave.size() - 1; i > 0; --i)
      {
        if( i < path.size() && path[i] == path_leave[i] )
          break;

        MouseEventPtr mouseleave(new MouseEvent(*event));
        mouseleave->type = Event::MOUSE_LEAVE;
        handled |= propagateEvent(mouseleave, path_leave);

        path_leave.pop_back();
      }
    }

    // Enter new element
    if( !path.empty() )
    {
      MouseEventPtr mouseover(new MouseEvent(*event));
      mouseover->type = Event::MOUSE_OVER;
      handled |= propagateEvent(mouseover, path);

      // Send a mouseenter event to all ancestors of the currently entered
      // element which are not ancestor of the old element currently being
      // left
      EventPropagationPath path_enter;
      for(size_t i = 0; i < path.size(); ++i)
      {
        path_enter.push_back(path[i]);

        if( i < last_path.size() && path[i] == last_path[i] )
          continue;

        MouseEventPtr mouseenter(new MouseEvent(*event));
        mouseenter->type = Event::MOUSE_ENTER;
        handled |= propagateEvent(mouseenter, path_enter);
      }
    }

    _last_mouse_over.path = path;
    return handled;
  }

  //----------------------------------------------------------------------------
  bool
  EventManager::checkClickDistance( const osg::Vec2f& pos1,
                                    const osg::Vec2f& pos2 ) const
  {
    osg::Vec2 delta = pos1 - pos2;
    return std::fabs(delta.x()) < drag_threshold
        && std::fabs(delta.y()) < drag_threshold;
  }

  //----------------------------------------------------------------------------
  EventPropagationPath
  EventManager::getCommonAncestor( const EventPropagationPath& path1,
                                   const EventPropagationPath& path2 ) const
  {
    if( path1.empty() || path2.empty() )
      return EventPropagationPath();

    if( path1.back().element.lock() == path2.back().element.lock() )
      return path2;

    EventPropagationPath path;

    for( size_t i = 0; i < path1.size() && i < path2.size(); ++i )
    {
      if( path1[i].element.lock() != path2[i].element.lock() )
        break;

      path.push_back(path2[i]);
    }

    return path;
  }

} // namespace canvas
} // namespace simgear
