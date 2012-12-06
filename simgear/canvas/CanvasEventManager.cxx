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

#include "CanvasEventManager.hxx"
#include "MouseEvent.hxx"
#include <simgear/canvas/elements/CanvasElement.hxx>

namespace simgear
{
namespace canvas
{

  const unsigned int drag_threshold = 8;
  const double multi_click_timeout = 0.4;

  //----------------------------------------------------------------------------
  EventManager::EventManager():
    _current_click_count(0)
  {

  }

  //----------------------------------------------------------------------------
  bool EventManager::handleEvent( const MouseEventPtr& event,
                                  const EventPropagationPath& path )
  {
    propagateEvent(event, path);
    switch( event->type )
    {
      case Event::MOUSE_DOWN:
        _last_mouse_down = StampedPropagationPath(path, event->getTime());
        break;
      case Event::MOUSE_UP:
      {
        if( _last_mouse_down.path.empty() )
          // Ignore mouse up without any previous mouse down
          return false;

        if( checkClickDistance(path, _last_mouse_down.path) )
          handleClick(event, getCommonAncestor(_last_mouse_down.path, path));

        break;
      }
      default:
        return false;
    }

    return true;
  }

  //----------------------------------------------------------------------------
  void EventManager::handleClick( const MouseEventPtr& event,
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
        // Reset current click count if moved too far
        if( !checkClickDistance(path, _last_click.path) )
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

    propagateEvent(click, path);

    if( dbl_click )
      propagateEvent(dbl_click, getCommonAncestor(_last_click.path, path));

    _last_click = StampedPropagationPath(path, event->getTime());
  }

  //----------------------------------------------------------------------------
  bool EventManager::propagateEvent( const EventPtr& event,
                                     const EventPropagationPath& path )
  {
    event->target = path.back().element;
    MouseEventPtr mouse_event = boost::dynamic_pointer_cast<MouseEvent>(event);

    // Event propagation similar to DOM Level 3 event flow:
    // http://www.w3.org/TR/DOM-Level-3-Events/#event-flow

    // Capturing phase
//    for( EventTargets::iterator it = _target_path.begin();
//                                it != _target_path.end();
//                              ++it )
//    {
//      if( it->element )
//        std::cout << it->element->getProps()->getPath() << " "
//                  << "(" << it->local_pos.x() << "|" << it->local_pos.y() << ")\n";
//    }

    // Bubbling phase
    for( EventPropagationPath::const_reverse_iterator
           it = path.rbegin();
           it != path.rend();
         ++it )
    {
      ElementPtr el = it->element.lock();

      if( !el )
        // Ignore element if it has been destroyed while traversing the event
        // (eg. removed by another event handler)
        continue;

      if( mouse_event )
      {
        // Position and delta are specified in local coordinate system of
        // current element
        mouse_event->pos = it->local_pos;
        mouse_event->delta = it->local_delta;
      }

      el->callListeners(event);

      if( event->propagation_stopped )
        return true;
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool
  EventManager::checkClickDistance( const EventPropagationPath& path1,
                                    const EventPropagationPath& path2 ) const
  {
    osg::Vec2 delta = path1.front().local_pos - path2.front().local_pos;
    return delta.x() < drag_threshold
        && delta.y() < drag_threshold;
  }

  //----------------------------------------------------------------------------
  EventPropagationPath
  EventManager::getCommonAncestor( const EventPropagationPath& path1,
                                   const EventPropagationPath& path2 ) const
  {
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
