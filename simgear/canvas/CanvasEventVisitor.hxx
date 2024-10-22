// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Visitor for traversing a canvas element hierarchy similar to the traversal of the DOM Level 3 Event Model
 */

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
