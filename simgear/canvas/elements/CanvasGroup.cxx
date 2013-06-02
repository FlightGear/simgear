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
#include <simgear/canvas/CanvasEventVisitor.hxx>
#include <simgear/canvas/MouseEvent.hxx>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/core.hpp>

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
                            const Style& style,
                            Element* parent )
  {
    ElementPtr el( new T(canvas, node, style, parent) );
    el->setSelf(el);
    return el;
  }

  //----------------------------------------------------------------------------
  ElementFactories Group::_child_factories;

  //----------------------------------------------------------------------------
  Group::Group( const CanvasWeakPtr& canvas,
                const SGPropertyNode_ptr& node,
                const Style& parent_style,
                Element* parent ):
    Element(canvas, node, parent_style, parent)
  {
    if( !isInit<Group>() )
    {
      _child_factories["group"] = &createElement<Group>;
      _child_factories["image"] = &createElement<Image>;
      _child_factories["map"  ] = &createElement<Map  >;
      _child_factories["path" ] = &createElement<Path >;
      _child_factories["text" ] = &createElement<Text >;
    }
  }

  //----------------------------------------------------------------------------
  Group::~Group()
  {

  }

  //----------------------------------------------------------------------------
  ElementPtr Group::createChild( const std::string& type,
                                 const std::string& id )
  {
    SGPropertyNode* node = _node->addChild(type, 0, false);
    if( !id.empty() )
      node->setStringValue("id", id);

    return getChild(node);
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::getChild(const SGPropertyNode* node)
  {
    ChildList::iterator child = findChild(node);
    if( child == _children.end() )
      return ElementPtr();

    return child->second;
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::getChild(const std::string& id)
  {
    for( ChildList::iterator child = _children.begin();
                             child != _children.end();
                           ++child )
    {
      if( child->second->get<std::string>("id") == id )
        return child->second;
    }

    return ElementPtr();
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::getElementById(const std::string& id)
  {
    std::vector<GroupPtr> groups;

    BOOST_FOREACH( ChildList::value_type child, _children )
    {
      const ElementPtr& el = child.second;
      if( el->getProps()->getStringValue("id") == id )
        return el;

      GroupPtr group = boost::dynamic_pointer_cast<Group>(el);
      if( group )
        groups.push_back(group);
    }

    BOOST_FOREACH( GroupPtr group, groups )
    {
      ElementPtr el = group->getElementById(id);
      if( el )
        return el;
    }

    return ElementPtr();
  }

  //----------------------------------------------------------------------------
  void Group::update(double dt)
  {
    BOOST_FOREACH( ChildList::value_type child, _children )
      child.second->update(dt);

    Element::update(dt);
  }

  //----------------------------------------------------------------------------
  bool Group::traverse(EventVisitor& visitor)
  {
    // Iterate in reverse order as last child is displayed on top
    BOOST_REVERSE_FOREACH( ChildList::value_type child, _children )
    {
      if( child.second->accept(visitor) )
        return true;
    }
    return false;
  }

  //----------------------------------------------------------------------------
  bool Group::setStyle(const SGPropertyNode* style)
  {
    // Don't propagate styles directly applicable to this group
    if( Element::setStyle(style) )
      return true;

    if(    style->getParent() != _node
        && _style.find(style->getNameString()) != _style.end() )
      return false;

    bool handled = false;
    BOOST_FOREACH( ChildList::value_type child, _children )
    {
      if( child.second->setStyle(style) )
        handled = true;
    }

    return handled;
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Group::getTransformedBounds(const osg::Matrix& m) const
  {
    osg::BoundingBox bb;

    BOOST_FOREACH( ChildList::value_type child, _children )
    {
      if( !child.second->getMatrixTransform()->getNodeMask() )
        continue;

      bb.expandBy
      (
        child.second->getTransformedBounds
        (
          child.second->getMatrixTransform()->getMatrix() * m
        )
      );
    }

    return bb;
  }

  //----------------------------------------------------------------------------
  void Group::childAdded(SGPropertyNode* child)
  {
    if( child->getParent() != _node )
      return;

    ElementFactories::iterator child_factory =
      _child_factories.find( child->getNameString() );
    if( child_factory != _child_factories.end() )
    {
      ElementPtr element = child_factory->second(_canvas, child, _style, this);

      // Add to osg scene graph...
      _transform->addChild( element->getMatrixTransform() );
      _children.push_back( ChildList::value_type(child, element) );

      // ...and ensure correct ordering
      handleZIndexChanged( --_children.end() );

      return;
    }

    if( !Element::setStyle(child) )
    {
      // Only add style if not applicable to group itself
      _style[ child->getNameString() ] = child;
      setStyle(child);
    }
  }

  //----------------------------------------------------------------------------
  void Group::childRemoved(SGPropertyNode* node)
  {
    if( node->getParent() != _node )
      return;

    if( _child_factories.find(node->getNameString()) != _child_factories.end() )
    {
      ChildList::iterator child = findChild(node);
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
      return handleZIndexChanged( findChild(node->getParent()),
                                  node->getIntValue() );
  }

  //----------------------------------------------------------------------------
  void Group::handleZIndexChanged(ChildList::iterator child, int z_index)
  {
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

  //----------------------------------------------------------------------------
  Group::ChildList::iterator Group::findChild(const SGPropertyNode* node)
  {
    return std::find_if
    (
      _children.begin(),
      _children.end(),
      boost::bind(&ChildList::value_type::first, _1) == node
    );
  }

} // namespace canvas
} // namespace simgear
