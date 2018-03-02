///@file
/// Visitor for traversing a canvas element hierarchy similar to the traversal
/// of the DOM Level 3 Event Model
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

#include "CanvasEvent.hxx"
#include "CanvasEventVisitor.hxx"
#include "elements/CanvasElement.hxx"

namespace simgear
{
namespace canvas
{

  //----------------------------------------------------------------------------
  EventVisitor::EventVisitor( TraverseMode mode,
                              const osg::Vec2f& pos,
                              const ElementPtr& root ):
    _traverse_mode( mode ),
    _root(root)
  {
    if( mode == TRAVERSE_DOWN )
      _target_path.push_back( EventTarget(NULL, pos) );
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
      const osg::Vec2f& pos = _target_path.back().local_pos;
      const osg::Vec2f local_pos = el.posToLocal(pos);

      // Don't check specified root element for collision, as its purpose is to
      // catch all events which have no target. This allows for example calling
      // event listeners attached to the canvas itself (its root group) even if
      // no element has been hit.
      if(    _root.get() != &el
          && !el.hitBound(_target_path.front().local_pos, pos, local_pos) )
        return false;

      _target_path.push_back( EventTarget(&el, local_pos) );

      if( el.traverse(*this) || &el == _root.get() )
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
