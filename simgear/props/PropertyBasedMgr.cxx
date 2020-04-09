///@file
/// Base class for property controlled subsystems
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
