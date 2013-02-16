// Visitor for traversing a canvas element hierarchy similar to the traversal
// of the DOM Level 3 Event Model
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
      EventTarget target = {ElementWeakPtr(), pos, delta};
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

      // Don't check collision with root element (2nd element in _target_path)
      // do event listeners attached to the canvas itself (its root group)
      // always get called even if no element has been hit.
      if( _target_path.size() > 2 && !el.hitBound(pos, local_pos) )
        return false;

      const osg::Vec2f& delta = _target_path.back().local_delta;
      const osg::Vec2f local_delta
      (
        m(0, 0) * delta[0] + m(1, 0) * delta[1],
        m(0, 1) * delta[0] + m(1, 1) * delta[1]
      );

      EventTarget target = {el.getWeakPtr(), local_pos, local_delta};
      _target_path.push_back(target);

      if( el.traverse(*this) || _target_path.size() <= 2 )
        return true;

      _target_path.pop_back();
      return false;
    }
    else
      return el.ascend(*this);
  }

  //----------------------------------------------------------------------------
  const EventPropagationPath& EventVisitor::getPropagationPath() const
  {
    return _target_path;
  }

} // namespace canvas
} // namespace simgear
