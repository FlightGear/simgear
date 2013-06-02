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

#ifndef SG_PROPERTY_BASED_ELEMENT_HXX_
#define SG_PROPERTY_BASED_ELEMENT_HXX_

#include <simgear/props/props.hxx>

#include <boost/call_traits.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


namespace simgear
{

  class PropertyBasedElement;
  typedef boost::shared_ptr<PropertyBasedElement> PropertyBasedElementPtr;
  typedef boost::weak_ptr<PropertyBasedElement> PropertyBasedElementWeakPtr;

  /**
   * Base class for a property controlled element
   */
  class PropertyBasedElement:
    public SGPropertyChangeListener
  {
    public:
      PropertyBasedElement(SGPropertyNode* node);
      virtual ~PropertyBasedElement();

      virtual void update(double delta_time_sec) = 0;

      SGConstPropertyNode_ptr getProps() const;
      SGPropertyNode_ptr getProps();

      template<class T>
      void set( const std::string& name,
                typename boost::call_traits<T>::param_type val )
      {
        setValue(_node->getNode(name, true), val);
      }

      template<class T>
      T get( const std::string& name,
             typename boost::call_traits<T>::param_type def = T() )
      {
        SGPropertyNode const* child = _node->getNode(name);
        if( !child )
          return def;

        return getValue<T>(child);
      }

      virtual void setSelf(const PropertyBasedElementPtr& self);

    protected:

      SGPropertyNode_ptr _node;
      PropertyBasedElementWeakPtr _self;
  };

} // namespace simgear

#endif /* SG_PROPERTY_BASED_ELEMENT_HXX_ */
