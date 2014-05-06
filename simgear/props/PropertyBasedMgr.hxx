// Base class for property controlled subsystems
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

#ifndef SG_PROPERTY_BASED_MGR_HXX_
#define SG_PROPERTY_BASED_MGR_HXX_

#include "PropertyBasedElement.hxx"
#include <simgear/structure/subsystem_mgr.hxx>

#include <boost/function.hpp>
#include <vector>

namespace simgear
{

  class PropertyBasedMgr:
    public SGSubsystem,
    public SGPropertyChangeListener
  {
    public:
      virtual void init();
      virtual void shutdown();

      virtual void update (double delta_time_sec);

      /**
       * Create a new PropertyBasedElement
       *
       * @param name    Name of the new element
       */
      PropertyBasedElementPtr createElement(const std::string& name = "");

      /**
       * Get an existing PropertyBasedElement by its index
       *
       * @param index   Index of element node in property tree
       */
      PropertyBasedElementPtr getElement(size_t index) const;

      /**
       * Get an existing PropertyBasedElement by its name
       *
       * @param name    Name (value of child node "name" will be matched)
       */
      PropertyBasedElementPtr getElement(const std::string& name) const;

      virtual const SGPropertyNode* getPropertyRoot() const;

    protected:

      typedef boost::function<PropertyBasedElementPtr(SGPropertyNode*)>
              ElementFactory;

      /** Branch in the property tree for this property managed subsystem */
      SGPropertyNode_ptr      _props;

      /** Property name of managed elements */
      const std::string       _name_elements;

      /** The actually managed elements */
      std::vector<PropertyBasedElementPtr> _elements;

      /** Function object which creates a new element */
      ElementFactory          _element_factory;

      /**
       * @param props         Root node of property branch used for controlling
       *                      this subsystem
       * @param name_elements The name of the nodes for the managed elements
       */
      PropertyBasedMgr( SGPropertyNode_ptr props,
                        const std::string& name_elements,
                        ElementFactory element_factory );
      virtual ~PropertyBasedMgr() = 0;

      virtual void childAdded( SGPropertyNode * parent,
                               SGPropertyNode * child );
      virtual void childRemoved( SGPropertyNode * parent,
                                 SGPropertyNode * child );

      virtual void elementCreated(PropertyBasedElementPtr element) {}

  };

} // namespace simgear

#endif /* SG_PROPERTY_BASED_MGR_HXX_ */
