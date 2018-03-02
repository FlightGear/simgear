///@file
/// A group of 2D Canvas elements
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

#include "CanvasGroup.hxx"
#include "CanvasImage.hxx"
#include "CanvasMap.hxx"
#include "CanvasPath.hxx"
#include "CanvasText.hxx"

#include <simgear/canvas/CanvasEventVisitor.hxx>
#include <simgear/canvas/events/MouseEvent.hxx>

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
    ElementType::staticInit();
    factories[ElementType::TYPE_NAME] = &Element::create<ElementType>;
  }

  //----------------------------------------------------------------------------
  ElementFactories Group::_child_factories;
  const std::string Group::TYPE_NAME = "group";

  void warnSceneGroupExpired(const char* member_name)
  {
    SG_LOG( SG_GENERAL,
            SG_WARN,
            "canvas::Group::" << member_name << ": Group has expired." );
  }

  //----------------------------------------------------------------------------
  void Group::staticInit()
  {
    if( isInit<Group>() )
      return;

    add<Group>(_child_factories);
    add<Image>(_child_factories);
    add<Map  >(_child_factories);
    add<Path >(_child_factories);
    add<Text >(_child_factories);
  }

  //----------------------------------------------------------------------------
  Group::Group( const CanvasWeakPtr& canvas,
                const SGPropertyNode_ptr& node,
                const Style& parent_style,
                ElementWeakPtr parent ):
    Element(canvas, node, parent_style, parent)
  {
    staticInit();
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
    if( !_scene_group.valid() )
    {
      warnSceneGroupExpired("getElementById");
      return {};
    }

    // TODO check search algorithm. Not completely breadth-first and might be
    //      possible with using less dynamic memory
    std::vector<GroupPtr> child_groups;
    for(size_t i = 0; i < _scene_group->getNumChildren(); ++i)
    {
      const ElementPtr& el = getChildByIndex(i);
      if( el->get<std::string>("id") == id )
        return el;

      if( Group* child_group = dynamic_cast<Group*>(el.get()) )
        child_groups.push_back(child_group);
    }

    for(auto group: child_groups)
    {
      if( ElementPtr el = group->getElementById(id) )
        return el;
    }

    return {};
  }

  //----------------------------------------------------------------------------
  void Group::clearEventListener()
  {
    Element::clearEventListener();

    if( !_scene_group.valid() )
      return warnSceneGroupExpired("clearEventListener");

    // TODO should this be recursive?
    for(size_t i = 0; i < _scene_group->getNumChildren(); ++i)
      getChildByIndex(i)->clearEventListener();
  }

  //----------------------------------------------------------------------------
  bool Group::traverse(EventVisitor& visitor)
  {
    if( _scene_group.valid() )
    {
      // Iterate in reverse order as last child is displayed on top
      for(size_t i = _scene_group->getNumChildren(); i --> 0;)
      {
        if( getChildByIndex(i)->accept(visitor) )
          return true;
      }
    }
    return false;
  }

  //----------------------------------------------------------------------------
  bool Group::setStyle( const SGPropertyNode* style,
                        const StyleInfo* style_info )
  {
    if( !canApplyStyle(style) )
      return false;

    bool handled = setStyleImpl(style, style_info);
    if( style_info->inheritable )
    {
      if( !_scene_group.valid() )
      {
        warnSceneGroupExpired("setStyle");
        return false;
      }

      for(size_t i = 0; i < _scene_group->getNumChildren(); ++i)
        handled |= getChildByIndex(i)->setStyle(style, style_info);
    }

    return handled;
  }

  //----------------------------------------------------------------------------
  osg::BoundingBox Group::getTransformedBounds(const osg::Matrix& m) const
  {
    if( !_scene_group.valid() )
    {
      warnSceneGroupExpired("getTransformedBounds");
      return {};
    }

    osg::BoundingBox bb;
    for(size_t i = 0; i < _scene_group->getNumChildren(); ++i)
    {
      auto child = getChildByIndex(i);
      if( !child || !child->isVisible() )
        continue;

      bb.expandBy( child->getTransformedBounds(child->getMatrix() * m) );
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
  void Group::updateImpl(double dt)
  {
    Element::updateImpl(dt);

    for(size_t i = 0; i < _scene_group->getNumChildren(); ++i)
      getChildByIndex(i)->update(dt);
  }

  //----------------------------------------------------------------------------
  void Group::childAdded(SGPropertyNode* child)
  {
    if( child->getParent() != _node )
      return;

    ElementFactory child_factory = getChildFactory( child->getNameString() );
    if( child_factory )
    {
      if( !_scene_group.valid() )
        return warnSceneGroupExpired("childAdded");

      ElementPtr element = child_factory(_canvas, child, _style, this);

      // Add to osg scene graph...
      _scene_group->addChild(element->getSceneGroup());

      // ...and ensure correct ordering
      handleZIndexChanged(element);

      return;
    }

    StyleInfo const* style = getStyleInfo(child->getNameString());
    if( style && style->inheritable )
      _style[ child->getNameString() ] = child;
  }

  //----------------------------------------------------------------------------
  void Group::childRemoved(SGPropertyNode* node)
  {
    if( node->getParent() != _node )
      return;

    if( getChildFactory(node->getNameString()) )
    {
      if( !_scene_group.valid() )
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
      _style.erase( node->getNameString() );
    }
  }

  //----------------------------------------------------------------------------
  void Group::childChanged(SGPropertyNode* node)
  {
    SGPropertyNode* parent = node->getParent();
    SGPropertyNode* grand_parent = parent ? parent->getParent() : nullptr;

    if(    grand_parent == _node
        && node->getNameString() == "z-index" )
      return handleZIndexChanged(getChild(parent), node->getIntValue());
  }

  //----------------------------------------------------------------------------
  void Group::handleZIndexChanged(ElementPtr child, int z_index)
  {
    if( !child || !_scene_group.valid() )
      return;

    // Keep reference to prevent deleting while removing and re-inserting later
    osg::ref_ptr<osg::MatrixTransform> tf = child->getSceneGroup();

    size_t index = _scene_group->getChildIndex(tf),
           index_new = index;

    for(;; ++index_new)
    {
      if( index_new + 1 == _scene_group->getNumChildren() )
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

    _scene_group->removeChild(index);
    _scene_group->insertChild(index_new, tf);

    SG_LOG
    (
      SG_GENERAL,
      SG_DEBUG,
      "canvas::Group: Moved element " << index << " to position " << index_new
    );
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::getChildByIndex(size_t index) const
  {
    assert( _scene_group.valid() );

    auto child = _scene_group->getChild(index);
    if( !child )
      return {};

    auto ud = static_cast<OSGUserData*>(child->getUserData());
    return ud ? ud->element : ElementPtr();
  }

  //----------------------------------------------------------------------------
  ElementPtr Group::findChild( const SGPropertyNode* node,
                               const std::string& id ) const
  {
    if( !_scene_group.valid() )
    {
      warnSceneGroupExpired("findChild");
      return {};
    }

    for(size_t i = 0; i < _scene_group->getNumChildren(); ++i)
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

    return {};
  }

} // namespace canvas
} // namespace simgear
