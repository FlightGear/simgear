// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Base class for property controlled subsystems
 */

#include <simgear_config.h>

#include "PropertyBasedMgr.hxx"

#include <stdexcept>
#include <string>

namespace simgear
{

  //----------------------------------------------------------------------------
  void PropertyBasedMgr::init()
  {
    _props->addChangeListener(this);
    _props->fireCreatedRecursive();
  }

  //----------------------------------------------------------------------------
  void PropertyBasedMgr::shutdown()
  {
    _props->removeAllChildren();
    _props->removeChangeListener(this);
  }

  //----------------------------------------------------------------------------
  void PropertyBasedMgr::update(double delta_time_sec)
  {
    for( size_t i = 0; i < _elements.size(); ++i )
      if( _elements[i] )
        _elements[i]->update(delta_time_sec);
  }

  //----------------------------------------------------------------------------
  PropertyBasedElementPtr
  PropertyBasedMgr::createElement(const std::string& name)
  {
    SGPropertyNode* node = _props->addChild(_name_elements, 0, false);
    if( !name.empty() )
      node->setStringValue("name", name);

    return getElement( node->getIndex() );
  }

  //----------------------------------------------------------------------------
  PropertyBasedElementPtr PropertyBasedMgr::getElement(size_t index) const
  {
    if( index >= _elements.size() )
      return PropertyBasedElementPtr();

    return _elements[index];
  }

  //----------------------------------------------------------------------------
  PropertyBasedElementPtr
  PropertyBasedMgr::getElement(const std::string& name) const
  {
    if( name.empty() )
      return PropertyBasedElementPtr();

    for (auto el : _elements)
      if( el && el->getProps()->getStringValue("name") == name )
        return el;

    return PropertyBasedElementPtr();
  }

  //----------------------------------------------------------------------------
  const SGPropertyNode* PropertyBasedMgr::getPropertyRoot() const
  {
    return _props;
  }

  //----------------------------------------------------------------------------
  PropertyBasedMgr::PropertyBasedMgr( SGPropertyNode_ptr props,
                                      const std::string& name_elements,
                                      ElementFactory element_factory ):
    _props( props ),
    _name_elements( name_elements ),
    _element_factory( element_factory )
  {

  }

  //----------------------------------------------------------------------------
  PropertyBasedMgr::~PropertyBasedMgr()
  {

  }

  //----------------------------------------------------------------------------
  void PropertyBasedMgr::childAdded( SGPropertyNode * parent,
                                     SGPropertyNode * child )
  {
    if( parent != _props || child->getNameString() != _name_elements )
      return;

    size_t index = child->getIndex();

    if( index >= _elements.size() )
    {
      if( index > _elements.size() )
        SG_LOG
        (
          SG_GENERAL,
          SG_WARN,
          "Skipping unused " << _name_elements << " slot(s)!"
        );

      _elements.resize(index + 1);
    }
    else if( _elements[index] )
    {
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        _name_elements << "[" << index << "] already exists!"
      );

      // Give anything holding a reference to this element to release it
      _elements[index]->onDestroy();
    }

    PropertyBasedElementPtr el = _element_factory(child);
    _elements[index] = el;
    elementCreated( el );
  }

  //----------------------------------------------------------------------------
  void PropertyBasedMgr::childRemoved( SGPropertyNode * parent,
                                       SGPropertyNode * child )
  {
    if( parent != _props )
      return child->fireChildrenRemovedRecursive();
    else if( child->getNameString() != _name_elements )
      return;

    size_t index = child->getIndex();

    if( index >= _elements.size() )
      SG_LOG
      (
        SG_GENERAL,
        SG_WARN,
        "can't removed unknown " << _name_elements << "[" << index << "]!"
      );
    else
    {
      // remove the element...
      _elements[index]->onDestroy();
      _elements[index].reset();
    }
  }

} // namespace simgear
