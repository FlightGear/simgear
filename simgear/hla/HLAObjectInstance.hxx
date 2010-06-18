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

#ifndef HLAObjectInstance_hxx
#define HLAObjectInstance_hxx

#include <simgear/structure/SGWeakPtr.hxx>

#include "HLADataElement.hxx"

class SGTimeStamp;

namespace simgear {

class RTIObjectInstance;
class HLAObjectClass;

class HLAObjectInstance : public SGWeakReferenced {
public:
    HLAObjectInstance(HLAObjectClass* objectClass);
    HLAObjectInstance(HLAObjectClass* objectClass, RTIObjectInstance* rtiObjectInstance);
    virtual ~HLAObjectInstance();

    const std::string& getName() const
    { return _name; }

    SGSharedPtr<HLAObjectClass> getObjectClass() const;

    unsigned getNumAttributes() const;
    unsigned getAttributeIndex(const std::string& name) const;
    std::string getAttributeName(unsigned index) const;

    const HLADataType* getAttributeDataType(unsigned index) const;

    void setAttributeDataElement(unsigned index, SGSharedPtr<HLADataElement> dataElement);
    HLADataElement* getAttributeDataElement(unsigned index);
    const HLADataElement* getAttributeDataElement(unsigned index) const;
    void setAttribute(unsigned index, const HLAPathElementMap& pathElementMap);
    void setAttributes(const HLAAttributePathElementMap& attributePathElementMap);

    // Ask the rti to provide the attribute at index
    void requestAttributeUpdate(unsigned index);
    void requestAttributeUpdate();

    void registerInstance();
    void deleteInstance(const RTIData& tag);
    void localDeleteInstance();

    class AttributeCallback : public SGReferenced {
    public:
        virtual ~AttributeCallback() {}
        // Notification about reflect and whatever TBD
        // Hmm, don't know yet how this should look like
        virtual void updateAttributeValues(HLAObjectInstance& objectInstance, const RTIData& tag)
        { }

        virtual void reflectAttributeValues(HLAObjectInstance& objectInstance,
                                            const RTIIndexDataPairList& dataPairList, const RTIData& tag)
        { }
        virtual void reflectAttributeValues(HLAObjectInstance& objectInstance, const RTIIndexDataPairList& dataPairList,
                                            const SGTimeStamp& timeStamp, const RTIData& tag)
        { reflectAttributeValues(objectInstance, dataPairList, tag); }
    };

    void setAttributeCallback(const SGSharedPtr<AttributeCallback>& attributeCallback)
    { _attributeCallback = attributeCallback; }
    const SGSharedPtr<AttributeCallback>& getAttributeCallback() const
    { return _attributeCallback; }

    // Push the current values into the RTI
    void updateAttributeValues(const RTIData& tag);
    void updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag);

    // Retrieve queued up updates up to and including timestamp,
    // Note that this only applies to timestamped updates.
    // The unordered updates are reflected as they arrive
    void reflectQueuedAttributeValues(const SGTimeStamp& timeStamp);

private:
    void removeInstance(const RTIData& tag);
    void reflectAttributeValues(const RTIIndexDataPairList& dataPairList, const RTIData& tag);
    void reflectAttributeValues(const RTIIndexDataPairList& dataPairList, const SGTimeStamp& timeStamp, const RTIData& tag);
    friend class RTIObjectInstance;
    friend class HLAObjectClass;

    class DataElementFactoryVisitor;

    std::string _name;

    SGWeakPtr<HLAObjectClass> _objectClass;
    SGSharedPtr<RTIObjectInstance> _rtiObjectInstance;
    SGSharedPtr<AttributeCallback> _attributeCallback;
};

} // namespace simgear

#endif
