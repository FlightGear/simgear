// A group of 2D Canvas elements
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

#ifndef CANVAS_GROUP_HXX_
#define CANVAS_GROUP_HXX_

#include "CanvasElement.hxx"

#include <boost/shared_ptr.hpp>
#include <list>
#include <map>

namespace simgear
{
namespace canvas
{

  typedef boost::shared_ptr<Element> ElementPtr;

  class Group:
    public Element
  {
    public:
      typedef std::list< std::pair< const SGPropertyNode*,
                                    ElementPtr
                                  >
                       > ChildList;

      Group( const CanvasWeakPtr& canvas,
             SGPropertyNode_ptr node,
             const Style& parent_style = Style() );
      virtual ~Group();

      virtual void update(double dt);

    protected:

      ChildList _children;

      virtual bool handleLocalMouseEvent(const canvas::MouseEvent& event);

      virtual void childAdded(SGPropertyNode * child);
      virtual void childRemoved(SGPropertyNode * child);
      virtual void childChanged(SGPropertyNode * child);

      void handleZIndexChanged(SGPropertyNode* node, int z_index);
  };

} // namespace canvas
} // namespace simgear

#endif /* CANVAS_GROUP_HXX_ */
