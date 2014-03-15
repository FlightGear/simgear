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

#ifndef CANVAS_EVENT_VISITOR_HXX_
#define CANVAS_EVENT_VISITOR_HXX_

#include "canvas_fwd.hxx"
#include "CanvasEventManager.hxx"

namespace simgear
{
namespace canvas
{

  class EventVisitor
  {
    public:

      enum TraverseMode
      {
        TRAVERSE_UP,
        TRAVERSE_DOWN
      };

      /**
       *
       * @param mode
       * @param pos     Mouse position
       * @param root    Element to dispatch events to if no element is hit
       */
      EventVisitor( TraverseMode mode,
                    const osg::Vec2f& pos,
                    const ElementPtr& root = ElementPtr() );
      virtual ~EventVisitor();
      virtual bool traverse(Element& el);
      virtual bool apply(Element& el);

      const EventPropagationPath& getPropagationPath() const;

    protected:

      TraverseMode          _traverse_mode;
      EventPropagationPath  _target_path;
      ElementPtr            _root;

  };

} // namespace canvas
} // namespace simgear


#endif /* CANVAS_EVENTVISITOR_HXX_ */
