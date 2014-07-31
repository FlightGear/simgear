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
#include <simgear/structure/SGWeakReferenced.hxx>

namespace simgear
{

  /**
   * Base class for a property controlled element
   */
  class PropertyBasedElement:
    public SGPropertyChangeListener,
    public virtual SGVirtualWeakReferenced
  {
    public:
      PropertyBasedElement(SGPropertyNode* node);
      virtual ~PropertyBasedElement();

      /**
       * Remove the property listener of the element.
       *
       * You will need to call the appropriate methods (childAdded(),
       * childRemoved(), valueChanged()) yourself to ensure the element still
       * receives the needed events.
       */
      void removeListener();

      /**
       * Destroys this element (removes node from property tree)
       */
      void destroy();

      virtual void update(double delta_time_sec) = 0;

      SGConstPropertyNode_ptr getProps() const;
      SGPropertyNode_ptr getProps();

      template<class T>
      void set( const std::string& name,
                const T& val )
      {
        setValue(_node->getNode(name, true), val);
      }

      template<class T>
      T get( const std::string& name,
             const T& def = T() )
      {
        SGPropertyNode const* child = _node->getNode(name);
        if( !child )
          return def;

        return getValue<T>(child);
      }

      // Unshadow what we have just hidden...
      using SGWeakReferenced::get;


      /** @brief Set a HTML5 like data property on this element.
       *
       * Set data-* properties on this element. A camel-case @a name will be
       * converted to a hyphenated name with 'data-' prefixed. Setting a value
       * with this method does not trigger an update of the canvas and is meant
       * to store data related to this element (used eg. inside scripts).
       *
       * @code{cpp}
       * // Set value
       * my_element->setDataProp("mySpecialInt", 3);
       *
       * // Get value (with default value)
       * int val = my_element->getDataProp<int>("mySpecialInt");   // val == 3
       *     val = my_element->getDataProp<int>("notExisting", 5); // val == 5
       *
       * // Check if value exists
       * SGPropertyNode* node =
       *   my_element->getDataProp<SGPropertyNode*>("mySpecialInt");
       * if( node )
       *   val = node->getIntValue(); // node != NULL, val == 3
       *
       * node = my_element->getDataProp<SGPropertyNode*>("notExisting");
       * // node == NULL
       * @endcode
       */
      template<class T>
      void setDataProp( const std::string& name,
                        const T& val )
      {
        const std::string& attr = dataPropToAttrName(name);
        if( !attr.empty() )
          set<T>(attr, val);
        else
          SG_LOG(SG_GENERAL, SG_WARN, "Invalid data-prop name: " << name);
      }

      /** @brief Get a HTML5 like data property on this element.
       *
       * Get value or default value.
       *
       * @see setDataProp
       */
      template<class T>
      typename boost::disable_if<
        boost::is_same<T, SGPropertyNode*>,
        T
      >::type getDataProp( const std::string& name,
                           const T& def = T() ) const
      {
        SGPropertyNode* node = getDataProp<SGPropertyNode*>(name);
        if( node )
          return getValue<T>(node);

        return def;
      }

      /** @brief Get a HTML5 like data property on this element.
       *
       * Use this variant to check if a property exists.
       *
       * @see setDataProp
       */
      template<class T>
      typename boost::enable_if<
        boost::is_same<T, SGPropertyNode*>,
        T
      >::type getDataProp( const std::string& name,
                           SGPropertyNode* = NULL ) const
      {
        const std::string& attr = dataPropToAttrName(name);
        if( attr.empty() )
        {
          SG_LOG(SG_GENERAL, SG_WARN, "Invalid data-prop name: " << name);
          return NULL;
        }

        return _node->getNode(attr);
      }

      /** @brief Check whether a HTML5 like data property exists on this
       *         element.
       *
       */
      bool hasDataProp(const std::string& name) const;

      /** @brief Remove a HTML5 like data property (if it exists).
       *
       */
      void removeDataProp(const std::string& name);

      virtual void onDestroy() {};

      static std::string dataPropToAttrName(const std::string& name);
      static std::string attrToDataPropName(const std::string& name);

    protected:

      SGPropertyNode_ptr _node;

  };

  typedef SGSharedPtr<PropertyBasedElement> PropertyBasedElementPtr;
  typedef SGWeakPtr<PropertyBasedElement> PropertyBasedElementWeakPtr;

} // namespace simgear

#endif /* SG_PROPERTY_BASED_ELEMENT_HXX_ */
