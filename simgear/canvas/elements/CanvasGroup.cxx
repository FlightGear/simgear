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
   * Add canvas Element type to factory map
   */
  template<typename ElementType>
  void add(ElementFactories& factories)
  {
    factories[ElementType::TYPE_NAME] = &Element::create<ElementType>;
  }

  //----------------------------------------------------------------------------
  ElementFactories Group::_child_factories;
  const std::string Group::TYPE_NAME = "group";

  //----------------------------------------------------------------------------
  Group::Group( const CanvasWeakPtr& canvas,
                const SGPropertyNode_ptr& node,
                const Style& parent_style,
                Element* parent ):
    Element(canvas, node, parent_style, parent)
  {
    if( !isInit<Group>() )
    {
      add<Group>(_child_factories);
      add<Image>(_child_factories);
      add<Map  >(_child_factories);
      add<Path >(_child_factories);
      add<Text >(_child_factories);
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
    return findChild(node, "");
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::getChild(const std::string& id)
  {
    return findChild(0, id);
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::getOrCreateChild( const std::string& type,
                                      const std::string& id )
  {
    ElementPtr child = getChild(id);
    if( child )
    {
      if( child->getProps()->getNameString() == type )
        return child;

      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "Group::getOrCreateChild: type missmatch! "
        "('" << type << "' != '" << child->getProps()->getName() << "', "
        "id = '" << id << "')"
      );

      return ElementPtr();
    }

    return createChild(type, id);
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::getElementById(const std::string& id)
  {
    std::vector<GroupPtr> groups;

    for(size_t i = 0; i < _transform->getNumChildren(); ++i)
    {
      const ElementPtr& el = getChildByIndex(i);
      if( el->get<std::string>("id") == id )
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
  void Group::clearEventListener()
  {

    for(size_t i = 0; i < _transform->getNumChildren(); ++i)
      getChildByIndex(i)->clearEventListener();

    Element::clearEventListener();
  }

  //----------------------------------------------------------------------------
  void Group::update(double dt)
  {
    for(size_t i = 0; i < _transform->getNumChildren(); ++i)
      getChildByIndex(i)->update(dt);

    Element::update(dt);
  }

  //----------------------------------------------------------------------------
  bool Group::traverse(EventVisitor& visitor)
  {
    // Iterate in reverse order as last child is displayed on top
    for(size_t i = _transform->getNumChildren(); i --> 0;)
    {
      if( getChildByIndex(i)->accept(visitor) )
        return true;
    }
    return false;
  }

  //----------------------------------------------------------------------------
  bool Group::setStyle( const SGPropertyNode* style,
                        const StyleSetter* setter )
  {
    if( !canApplyStyle(style) )
      return false;

    // Don't propagate styles directly applicable to this group
    if( setStyleImpl(style, setter) )
      return true;

    bool handled = false;
    for(size_t i = 0; i < _transform->getNumChildren(); ++i)
    {
      if( getChildByIndex(i)->setStyle(style, setter) )
        handled = true;
    }

    return handled;
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Group::getTransformedBounds(const osg::Matrix& m) const
  {
    osg::BoundingBox bb;

    for(size_t i = 0; i < _transform->getNumChildren(); ++i)
    {
      const ElementPtr& child = getChildByIndex(i);
      if( !child->getMatrixTransform()->getNodeMask() )
        continue;

      bb.expandBy
      (
        child->getTransformedBounds
        (
          child->getMatrixTransform()->getMatrix() * m
        )
      );
    }

    return bb;
  }

  //----------------------------------------------------------------------------
  ElementFactory Group::getChildFactory(const std::string& type) const
  {
    ElementFactories::iterator child_factory = _child_factories.find(type);
    if( child_factory != _child_factories.end() )
      return child_factory->second;

    return ElementFactory();
  }

  //----------------------------------------------------------------------------
  void Group::childAdded(SGPropertyNode* child)
  {
    if( child->getParent() != _node )
      return;

    ElementFactory child_factory = getChildFactory( child->getNameString() );
    if( child_factory )
    {
      ElementPtr element = child_factory(_canvas, child, _style, this);

      // Add to osg scene graph...
      _transform->addChild( element->getMatrixTransform() );

      // ...and ensure correct ordering
      handleZIndexChanged(element);

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

    if( getChildFactory(node->getNameString()) )
    {
      if( !_transform.valid() )
        // If transform is destroyed also all children are destroyed, so we can
        // not do anything here.
        return;

      ElementPtr child = getChild(node);
      if( !child )
        SG_LOG
        (
          SG_GL,
          SG_WARN,
          "can't removed unknown child " << node->getDisplayName()
        );
      else
      {
        // Remove child from the scenegraph (this automatically invalidates the
        // reference on the element hold by the MatrixTransform)
        child->onDestroy();
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
      return handleZIndexChanged( getChild(node->getParent()),
                                  node->getIntValue() );
  }

  //----------------------------------------------------------------------------
  void Group::handleZIndexChanged(ElementPtr child, int z_index)
  {
    if( !child )
      return;

    osg::ref_ptr<osg::MatrixTransform> tf = child->getMatrixTransform();
    size_t index = _transform->getChildIndex(tf),
           index_new = index;

    for(;; ++index_new)
    {
      if( index_new + 1 == _transform->getNumChildren() )
        break;

      // Move to end of block with same index (= move upwards until the next
      // element has a higher index)
      if( getChildByIndex(index_new + 1)->get<int>("z-index", 0) > z_index )
        break;
    }

    if( index_new == index )
    {
      // We were not able to move upwards so now try downwards
      for(;; --index_new)
      {
        if( index_new == 0 )
          break;

        // Move to end of block with same index (= move downwards until the
        // previous element has the same or a lower index)
        if( getChildByIndex(index_new - 1)->get<int>("z-index", 0) <= z_index )
          break;
      }

      if( index == index_new )
        return;
    }

    _transform->removeChild(index);
    _transform->insertChild(index_new, tf);

    SG_LOG
    (
      SG_GENERAL,
      SG_INFO,
      "canvas::Group: Moved element " << index << " to position " << index_new
    );
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::getChildByIndex(size_t index) const
  {
    assert(_transform.valid());
    OSGUserData* ud =
      static_cast<OSGUserData*>(_transform->getChild(index)->getUserData());
    assert(ud);
    return ud->element;
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::findChild( const SGPropertyNode* node,
                               const std::string& id ) const
  {
    for(size_t i = 0; i < _transform->getNumChildren(); ++i)
    {
      ElementPtr el = getChildByIndex(i);

      if( node )
      {
        if( el->getProps() == node )
          return el;
      }
      else
      {
        if( el->get<std::string>("id") == id )
          return el;
      }
    }

    return ElementPtr();
  }

} // namespace canvas
} // namespace simgear
