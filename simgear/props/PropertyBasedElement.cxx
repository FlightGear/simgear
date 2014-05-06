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

#include "PropertyBasedElement.hxx"

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

} // namespace simgear
