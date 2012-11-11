// Copyright (C) 2009 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef HLAObjectClass_hxx
#define HLAObjectClass_hxx

#include <string>
#include <vector>

#include "HLADataType.hxx"
#include "HLAObjectInstance.hxx"
#include "HLATypes.hxx"

namespace simgear {

class RTIObjectClass;
class HLAFederate;

class HLAObjectClass : public SGWeakReferenced {
public:
    HLAObjectClass(const std::string& name, HLAFederate* federate);
    virtual ~HLAObjectClass();

    /// Return the name of this object class
    const std::string& getName() const;

    /// return the federate this class belongs to
    const SGWeakPtr<HLAFederate>& getFederate() const;

    /// Return the number of attributes in this object class
    unsigned getNumAttributes() const;

    /// Adds a new attribute to this object class, return the index
    unsigned addAttribute(const std::string& name);

    /// Return the attribute index for the attribute with the given name
    unsigned getAttributeIndex(const std::string& name) const;
    /// Return the attribute name for the attribute with the given index
    std::string getAttributeName(unsigned index) const;

    /// Return the data type of the attribute with the given index
    const HLADataType* getAttributeDataType(unsigned index) const;
    /// Sets the data type of the attribute with the given index to dataType
    void setAttributeDataType(unsigned index, const HLADataType* dataType);

    /// Return the update type of the attribute with the given index
    HLAUpdateType getAttributeUpdateType(unsigned index) const;
    /// Sets the update type of the attribute with the given index to updateType
    void setAttributeUpdateType(unsigned index, HLAUpdateType updateType);

    /// Return the subscription type of the attribute with the given index
    HLASubscriptionType getAttributeSubscriptionType(unsigned index) const;
    /// Sets the subscription type of the attribute with the given index to subscriptionType
    void setAttributeSubscriptionType(unsigned index, HLASubscriptionType subscriptionType);

    /// Return the publication type of the attribute with the given index
    HLAPublicationType getAttributePublicationType(unsigned index) const;
    /// Sets the publication type of the attribute with the given index to publicationType
    void setAttributePublicationType(unsigned index, HLAPublicationType publicationType);

    /// Get the attribute data element index for the given path, return true if successful
    bool getDataElementIndex(HLADataElementIndex& dataElementIndex, const std::string& path) const;
    HLADataElementIndex getDataElementIndex(const std::string& path) const;

    virtual bool subscribe();
    virtual bool unsubscribe();

    virtual bool publish();
    virtual bool unpublish();

    // Object instance creation and destruction
    class InstanceCallback : public SGReferenced {
    public:
        virtual ~InstanceCallback();

        virtual void discoverInstance(const HLAObjectClass& objectClass, HLAObjectInstance& objectInstance, const RTIData& tag);
        virtual void removeInstance(const HLAObjectClass& objectClass, HLAObjectInstance& objectInstance, const RTIData& tag);

        virtual void registerInstance(const HLAObjectClass& objectClass, HLAObjectInstance& objectInstance);
        virtual void deleteInstance(const HLAObjectClass& objectClass, HLAObjectInstance& objectInstance);
    };

    void setInstanceCallback(const SGSharedPtr<InstanceCallback>& instanceCallback)
    { _instanceCallback = instanceCallback; }
    const SGSharedPtr<InstanceCallback>& getInstanceCallback() const
    { return _instanceCallback; }

    virtual void discoverInstance(HLAObjectInstance& objectInstance, const RTIData& tag);
    virtual void removeInstance(HLAObjectInstance& objectInstance, const RTIData& tag);
    virtual void registerInstance(HLAObjectInstance& objectInstance);
    virtual void deleteInstance(HLAObjectInstance& objectInstance);

    // Is called by the default registration callback if installed
    // Should register the already known object instances of this class.
    virtual void startRegistration() const;
    virtual void stopRegistration() const;

    // Handles startRegistrationForObjectClass and stopRegistrationForObjectClass events
    class RegistrationCallback : public SGReferenced {
    public:
        virtual ~RegistrationCallback();
        virtual void startRegistration(HLAObjectClass& objectClass) = 0;
        virtual void stopRegistration(HLAObjectClass& objectClass) = 0;
    };

    void setRegistrationCallback(const SGSharedPtr<RegistrationCallback>& registrationCallback)
    { _registrationCallback = registrationCallback; }
    const SGSharedPtr<RegistrationCallback>& getRegistrationCallback() const
    { return _registrationCallback; }

    /// Create a new instance of this class.
    virtual HLAObjectInstance* createObjectInstance(const std::string& name);

    virtual void createAttributeDataElements(HLAObjectInstance& objectInstance);
    virtual HLADataElement* createAttributeDataElement(HLAObjectInstance& objectInstance, unsigned index);

private:
    HLAObjectClass(const HLAObjectClass&);
    HLAObjectClass& operator=(const HLAObjectClass&);

    void _setRTIObjectClass(RTIObjectClass* objectClass);
    void _resolveAttributeIndex(const std::string& name, unsigned index);
    void _clearRTIObjectClass();

    // The internal entry points from the RTILObjectClass callback functions
    void _discoverInstance(RTIObjectInstance* objectInstance, const RTIData& tag);
    void _removeInstance(HLAObjectInstance& objectInstance, const RTIData& tag);
    void _registerInstance(HLAObjectInstance* objectInstance);
    void _deleteInstance(HLAObjectInstance& objectInstance);

    void _startRegistration();
    void _stopRegistration();

    friend class HLAObjectInstance;
    friend class RTIObjectClass;

    struct Attribute {
        Attribute() : _subscriptionType(HLAUnsubscribed), _publicationType(HLAUnpublished), _updateType(HLAUndefinedUpdate) {}
        Attribute(const std::string& name) : _name(name), _subscriptionType(HLAUnsubscribed), _publicationType(HLAUnpublished), _updateType(HLAUndefinedUpdate) {}
        std::string _name;
        SGSharedPtr<const HLADataType> _dataType;
        HLASubscriptionType _subscriptionType;
        HLAPublicationType _publicationType;
        HLAUpdateType _updateType;
    };
    typedef std::vector<Attribute> AttributeVector;
    typedef std::map<std::string,unsigned> NameIndexMap;

    /// The parent federate.
    SGWeakPtr<HLAFederate> _federate;

    /// The object class name
    std::string _name;

    /// The underlying rti dispatcher class
    SGSharedPtr<RTIObjectClass> _rtiObjectClass;

    /// The attribute data
    AttributeVector _attributeVector;
    /// The mapping from attribute names to attribute indices
    NameIndexMap _nameIndexMap;

    // Callback classes
    SGSharedPtr<InstanceCallback> _instanceCallback;
    SGSharedPtr<RegistrationCallback> _registrationCallback;

    friend class HLAFederate;
};

} // namespace simgear

#endif
