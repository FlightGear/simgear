// Base class for elements of property controlled subsystems
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
#include "PropertyBasedElement.hxx"
#include <simgear/misc/strutils.hxx>

namespace simgear
{

  //----------------------------------------------------------------------------
  PropertyBasedElement::PropertyBasedElement(SGPropertyNode* node):
    _node(node)
  {
    _node->addChangeListener(this);
  }

  //----------------------------------------------------------------------------
  PropertyBasedElement::~PropertyBasedElement()
  {
    onDestroy();
    removeListener();
  }

  //----------------------------------------------------------------------------
  void PropertyBasedElement::removeListener()
  {
    if( _node )
      _node->removeChangeListener(this);
  }

  //----------------------------------------------------------------------------
  void PropertyBasedElement::destroy()
  {
    if( !_node )
      return;

    // TODO check if really not in use anymore
    if( _node->getParent() )
      _node->getParent()
           ->removeChild(_node->getName(), _node->getIndex());
  }

  //----------------------------------------------------------------------------
  SGConstPropertyNode_ptr PropertyBasedElement::getProps() const
  {
    return _node;
  }

  //----------------------------------------------------------------------------
  SGPropertyNode_ptr PropertyBasedElement::getProps()
  {
    return _node;
  }

  //----------------------------------------------------------------------------
  bool PropertyBasedElement::hasDataProp(const std::string& name) const
  {
    return getDataProp<SGPropertyNode*>(name) != NULL;
  }

  //----------------------------------------------------------------------------
  void PropertyBasedElement::removeDataProp(const std::string& name)
  {
    SGPropertyNode* node = getDataProp<SGPropertyNode*>(name);
    if( node )
      _node->removeChild(node);
  }

  //----------------------------------------------------------------------------
  static const std::string DATA_PREFIX("data-");

  //----------------------------------------------------------------------------
  std::string PropertyBasedElement::dataPropToAttrName(const std::string& name)
  {
    // http://www.w3.org/TR/html5/dom.html#attr-data-*
    //
    // 3. Insert the string data- at the front of name.

    std::string attr_name;
    for( std::string::const_iterator cur = name.begin();
                                     cur != name.end();
                                   ++cur )
    {
      // If name contains a "-" (U+002D) character followed by a lowercase ASCII
      // letter, throw a SyntaxError exception and abort these steps.
      if( *cur == '-' )
      {
        std::string::const_iterator next = cur + 1;
        if( next != name.end() && islower(*next) )
          return std::string();
      }

      // For each uppercase ASCII letter in name, insert a "-" (U+002D)
      // character before the character and replace the character with the same
      // character converted to ASCII lowercase.
      if( isupper(*cur) )
      {
        attr_name.push_back('-');
        attr_name.push_back( tolower(*cur) );
      }
      else
        attr_name.push_back( *cur );
    }
    return DATA_PREFIX + attr_name;
  }

  //----------------------------------------------------------------------------
  std::string PropertyBasedElement::attrToDataPropName(const std::string& name)
  {
    // http://www.w3.org/TR/html5/dom.html#attr-data-*
    //
    // For each "-" (U+002D) character in the name that is followed by a
    // lowercase ASCII letter, remove the "-" (U+002D) character and replace the
    // character that followed it by the same character converted to ASCII
    // uppercase.

    if( !strutils::starts_with(name, DATA_PREFIX) )
      return std::string();

    std::string data_name;
    for( std::string::const_iterator cur = name.begin() + DATA_PREFIX.length();
                                     cur != name.end();
                                   ++cur )
    {
      std::string::const_iterator next = cur + 1;
      if( *cur == '-' && next != name.end() && islower(*next) )
      {
        data_name.push_back( toupper(*next) );
        cur = next;
      }
      else
        data_name.push_back(*cur);
    }
    return data_name;
  }

} // namespace simgear
