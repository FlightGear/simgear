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

#include "CanvasGroup.hxx"
#include "CanvasImage.hxx"
#include "CanvasMap.hxx"
#include "CanvasPath.hxx"
#include "CanvasText.hxx"

#include <boost/foreach.hpp>

namespace simgear
{
namespace canvas
{
  /**
   * Create an canvas Element of type T
   */
  template<typename T>
  ElementPtr createElement( const CanvasWeakPtr& canvas,
                            const SGPropertyNode_ptr& node,
                            const Style& style )
  {
    return ElementPtr( new T(canvas, node, style) );
  }

  //----------------------------------------------------------------------------
  Group::Group( const CanvasWeakPtr& canvas,
                const SGPropertyNode_ptr& node,
                const Style& parent_style ):
    Element(canvas, node, parent_style)
  {
    _child_factories["group"] = &createElement<Group>;
    _child_factories["image"] = &createElement<Image>;
    _child_factories["map"  ] = &createElement<Map  >;
    _child_factories["path" ] = &createElement<Path >;
    _child_factories["text" ] = &createElement<Text >;
  }

  //----------------------------------------------------------------------------
  Group::~Group()
  {

  }

  //----------------------------------------------------------------------------
  void Group::update(double dt)
  {
    BOOST_FOREACH( ChildList::value_type child, _children )
      child.second->update(dt);

    Element::update(dt);
  }

  //----------------------------------------------------------------------------
  bool Group::handleLocalMouseEvent(const canvas::MouseEvent& event)
  {
    // Iterate in reverse order as last child is displayed on top
    BOOST_REVERSE_FOREACH( ChildList::value_type child, _children )
    {
      if( child.second->handleMouseEvent(event) )
        return true;
    }
    return false;
  }

  //----------------------------------------------------------------------------
  void Group::childAdded(SGPropertyNode* child)
  {
    if( child->getParent() != _node )
      return;

    ChildFactories::iterator child_factory =
      _child_factories.find( child->getNameString() );
    if( child_factory != _child_factories.end() )
    {
      ElementPtr element = child_factory->second(_canvas, child, _style);

      // Add to osg scene graph...
      _transform->addChild( element->getMatrixTransform() );
      _children.push_back( ChildList::value_type(child, element) );

      return;
    }

    _style[ child->getNameString() ] = child;
  }

  //----------------------------------------------------------------------------
  struct ChildFinder
  {
    public:
      ChildFinder(SGPropertyNode *node):
        _node(node)
      {}

      bool operator()(const Group::ChildList::value_type& el) const
      {
        return el.first == _node;
      }

    private:
      SGPropertyNode *_node;
  };

  //----------------------------------------------------------------------------
  void Group::childRemoved(SGPropertyNode* node)
  {
    if( node->getParent() != _node )
      return;

    if( _child_factories.find(node->getNameString()) != _child_factories.end() )
    {
      ChildFinder pred(node);
      ChildList::iterator child =
        std::find_if(_children.begin(), _children.end(), pred);

      if( child == _children.end() )
        SG_LOG
        (
          SG_GL,
          SG_WARN,
          "can't removed unknown child " << node->getDisplayName()
        );
      else
      {
        _transform->removeChild( child->second->getMatrixTransform() );
        _children.erase(child);
      }
    }
    else
    {
      Style::iterator style = _style.find(node->getNameString());
      if( style != _style.end() )
        _style.erase(style);
    }
  }

  //----------------------------------------------------------------------------
  void Group::childChanged(SGPropertyNode* node)
  {
    if(    node->getParent()->getParent() == _node
        && node->getNameString() == "z-index" )
      return handleZIndexChanged(node->getParent(), node->getIntValue());
  }

  //----------------------------------------------------------------------------
  void Group::handleZIndexChanged(SGPropertyNode* node, int z_index)
  {
    ChildFinder pred(node);
    ChildList::iterator child =
      std::find_if(_children.begin(), _children.end(), pred);

    if( child == _children.end() )
      return;

    osg::Node* tf = child->second->getMatrixTransform();
    int index = _transform->getChildIndex(tf),
        index_new = index;

    ChildList::iterator next = child;
    ++next;

    while(    next != _children.end()
           && next->first->getIntValue("z-index", 0) <= z_index )
    {
      ++index_new;
      ++next;
    }

    if( index_new != index )
    {
      _children.insert(next, *child);
    }
    else
    {
      ChildList::iterator prev = child;
      while(    prev != _children.begin()
             && (--prev)->first->getIntValue("z-index", 0) > z_index)
      {
        --index_new;
      }

      if( index == index_new )
        return;

      _children.insert(prev, *child);
    }

    _transform->removeChild(index);
    _transform->insertChild(index_new, tf);

    _children.erase(child);

    SG_LOG
    (
      SG_GENERAL,
      SG_INFO,
      "canvas::Group: Moved element " << index << " to position " << index_new
    );
  }

} // namespace canvas
} // namespace simgear
