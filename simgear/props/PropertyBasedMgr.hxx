// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2012 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Base class for property controlled subsystems
 */

#ifndef SG_PROPERTY_BASED_MGR_HXX_
#define SG_PROPERTY_BASED_MGR_HXX_

#include "PropertyBasedElement.hxx"
#include <simgear/structure/subsystem_mgr.hxx>

#include <vector>

namespace simgear
{

class PropertyBasedMgr : public SGSubsystem,
                         public SGPropertyChangeListener
{
public:
    // Subsystem API.
    void init() override;
    void shutdown() override;
    void update(double delta_time_sec) override;

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
    typedef std::function<PropertyBasedElementPtr(SGPropertyNode*)>
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

    void childAdded( SGPropertyNode * parent,
                     SGPropertyNode * child ) override;
    void childRemoved( SGPropertyNode * parent,
                       SGPropertyNode * child ) override;

    virtual void elementCreated(PropertyBasedElementPtr element) {}
};

} // namespace simgear

#endif /* SG_PROPERTY_BASED_MGR_HXX_ */
