// Copyright (C) 2009 - 2010  Mathias Froehlich - Mathias.Froehlich@web.de
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

#include <set>
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
    HLAObjectClass(const std::string& name, HLAFederate& federate);
    virtual ~HLAObjectClass();

    const std::string& getName() const
    { return _name; }

    unsigned getNumAttributes() const;
    unsigned getAttributeIndex(const std::string& name) const;
    std::string getAttributeName(unsigned index) const;

    const HLADataType* getAttributeDataType(unsigned index) const;
    void setAttributeDataType(unsigned index, const HLADataType*);

    HLAUpdateType getAttributeUpdateType(unsigned index) const;
    void setAttributeUpdateType(unsigned index, HLAUpdateType updateType);

    HLADataElement::IndexPathPair getIndexPathPair(const HLADataElement::StringPathPair&) const;
    HLADataElement::IndexPathPair getIndexPathPair(const std::string& path) const;

    bool subscribe(const std::set<unsigned>& indexSet, bool active);
    bool unsubscribe();

    bool publish(const std::set<unsigned>& indexSet);
    bool unpublish();

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

    // Is called by the default registration callback if installed
    void startRegistration() const;
    void stopRegistration() const;

protected:
    virtual HLAObjectInstance* createObjectInstance(RTIObjectInstance* rtiObjectInstance);

private:
    HLAObjectClass(const HLAObjectClass&);
    HLAObjectClass& operator=(const HLAObjectClass&);

    // The internal entry points from the RTILObjectClass callback functions
    void discoverInstance(RTIObjectInstance* objectInstance, const RTIData& tag);
    void removeInstance(HLAObjectInstance& objectInstance, const RTIData& tag);
    void registerInstance(HLAObjectInstance& objectInstance);
    void deleteInstance(HLAObjectInstance& objectInstance);

    void discoverInstanceCallback(HLAObjectInstance& objectInstance, const RTIData& tag) const;
    void removeInstanceCallback(HLAObjectInstance& objectInstance, const RTIData& tag) const;
    void registerInstanceCallback(HLAObjectInstance& objectInstance) const;
    void deleteInstanceCallback(HLAObjectInstance& objectInstance) const;

    void startRegistrationCallback();
    void stopRegistrationCallback();
    friend class HLAObjectInstance;
    friend class RTIObjectClass;

    // The object class name
    std::string _name;

    // The underlying rti dispatcher class
    SGSharedPtr<RTIObjectClass> _rtiObjectClass;

    // Callback classes
    SGSharedPtr<InstanceCallback> _instanceCallback;
    SGSharedPtr<RegistrationCallback> _registrationCallback;

    // The set of active objects
    typedef std::set<SGSharedPtr<HLAObjectInstance> > ObjectInstanceSet;
    ObjectInstanceSet _objectInstanceSet;
};

} // namespace simgear

#endif
