// Visitor for traversing a canvas element hierarchy similar to the traversal
// of the DOM Level 2 Event Model
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

#include "CanvasEvent.hxx"
#include "CanvasEventVisitor.hxx"
#include <simgear/canvas/elements/CanvasElement.hxx>
#include <iostream>

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  EventVisitor::EventVisitor( TraverseMode mode,
                              const osg::Vec2f& pos,
                              const osg::Vec2f& delta ):
    _traverse_mode( mode )
  {
    if( mode == TRAVERSE_DOWN )
    {
      EventTarget target = {0, pos, delta};
      _target_path.push_back(target);
    }
  }

  //----------------------------------------------------------------------------
  EventVisitor::~EventVisitor()
  {

  }

  //----------------------------------------------------------------------------
  bool EventVisitor::traverse(Element& el)
  {
    if( _traverse_mode == TRAVERSE_UP )
      return el.ascend(*this);
    else
      return el.traverse(*this);
  }

  //----------------------------------------------------------------------------
  bool EventVisitor::apply(Element& el)
  {
    // We only need to check for hits while traversing down
    if( _traverse_mode == TRAVERSE_DOWN )
    {
      // Transform event to local coordinates
      const osg::Matrix& m = el.getMatrixTransform()->getInverseMatrix();
      const osg::Vec2f& pos = _target_path.back().local_pos;
      const osg::Vec2f local_pos
      (
        m(0, 0) * pos[0] + m(1, 0) * pos[1] + m(3, 0),
        m(0, 1) * pos[0] + m(1, 1) * pos[1] + m(3, 1)
      );

      if( !el.hitBound(local_pos) )
        return false;

      const osg::Vec2f& delta = _target_path.back().local_delta;
      const osg::Vec2f local_delta
      (
        m(0, 0) * delta[0] + m(1, 0) * delta[1],
        m(0, 1) * delta[0] + m(1, 1) * delta[1]
      );

      EventTarget target = {&el, local_pos, local_delta};
      _target_path.push_back(target);

      if( el.traverse(*this) )
        return true;

      _target_path.pop_back();
      return false;
    }
    else
      return el.ascend(*this);
  }

  //----------------------------------------------------------------------------
  bool EventVisitor::propagateEvent(const EventPtr& event)
  {
//    std::cout << "Propagate event " << event->getTypeString() << "\n";
//    for( EventTargets::iterator it = _target_path.begin();
//                                it != _target_path.end();
//                              ++it )
//    {
//      if( it->element )
//        std::cout << it->element->getProps()->getPath() << " "
//                  << "(" << it->local_pos.x() << "|" << it->local_pos.y() << ")\n";
//    }

    return true;
  }

} // namespace canvas
} // namespace simgear
