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

#ifndef HLAObjectInstance_hxx
#define HLAObjectInstance_hxx

#include <vector>

#include <simgear/structure/SGWeakPtr.hxx>

#include "HLADataElement.hxx"
#include "HLATypes.hxx"

class SGTimeStamp;

namespace simgear {

class RTIObjectInstance;
class HLAFederate;
class HLAObjectClass;

class HLAObjectInstance : public SGWeakReferenced {
public:
    HLAObjectInstance(HLAObjectClass* objectClass = 0);
    virtual ~HLAObjectInstance();

    /// Return the name of this object instance
    const std::string& getName() const;

    /// return the federate this instance belongs to
    const SGWeakPtr<HLAFederate>& getFederate() const;

    /// Return the object class of this instance.
    /// Should always return a valid object class.
    const SGSharedPtr<HLAObjectClass>& getObjectClass() const;

    /// Return the number of attributes
    unsigned getNumAttributes() const;

    /// Return the attribute index for the attribute with the given name
    unsigned getAttributeIndex(const std::string& name) const;
    /// Return the attribute name for the attribute with the given index
    std::string getAttributeName(unsigned index) const;

    /// Return true if the attribute with the given index is owned by
    /// this federate
    bool getAttributeOwned(unsigned index) const;

    /// Return the data type of the attribute with the given index
    const HLADataType* getAttributeDataType(unsigned index) const;

    /// Return the data element of the attribute with the given index
    HLADataElement* getAttributeDataElement(unsigned index);
    const HLADataElement* getAttributeDataElement(unsigned index) const;

    /// Write the raw attribute data value into data, works only of the object
    /// is backed up with an rti object instance
    bool getAttributeData(unsigned index, RTIData& data) const;

    /// Sets the data element of the attribute with the given index to dataElement
    void setAttributeDataElement(unsigned index, const SGSharedPtr<HLADataElement>& dataElement);

    /// Retrieve the data element index for the given path.
    bool getDataElementIndex(HLADataElementIndex& index, const std::string& path) const;
    HLADataElementIndex getDataElementIndex(const std::string& path) const;

    /// Return the data element of the attribute with the given index
    HLADataElement* getAttributeDataElement(const HLADataElementIndex& index);
    const HLADataElement* getAttributeDataElement(const HLADataElementIndex& index) const;
    /// Set the data element of the attribute with the given index
    void setAttributeDataElement(const HLADataElementIndex& index, const SGSharedPtr<HLADataElement>& dataElement);

    /// Return the data element of the attribute with the given path.
    /// The method is only for convenience as parsing the path is expensive.
    /// Precompute the index and use the index instead if you use this method more often.
    HLADataElement* getAttributeDataElement(const std::string& path);
    const HLADataElement* getAttributeDataElement(const std::string& path) const;
    /// Set the data element of the attribute with the given path
    /// The method is only for convenience as parsing the path is expensive.
    /// Precompute the index and use the index instead if you use this method more often.
    void setAttributeDataElement(const std::string& path, const SGSharedPtr<HLADataElement>& dataElement);

    /// Gets called on discovering this object instance.
    virtual void discoverInstance(const RTIData& tag);
    /// Gets called on remove this object instance.
    virtual void removeInstance(const RTIData& tag);

    /// Call this to register the object instance at the rti and assign the object class to it.
    void registerInstance();
    virtual void registerInstance(HLAObjectClass* objectClass);
    /// Call this to delete the object instance from the rti.
    virtual void deleteInstance(const RTIData& tag);

    /// Is called when the instance is either registered or discovered.
    /// It creates data elements for each element that is not yet set and that has a data type attached.
    /// the default calls back into the object class createAttributeDataElements method.
    virtual void createAttributeDataElements();
    /// Create and set the data element with index. Called somewhere in the above callchain.
    void createAndSetAttributeDataElement(unsigned index);
    /// Create an individual data element, the default calls back into the object class
    /// createAttributeDataElement method.
    virtual HLADataElement* createAttributeDataElement(unsigned index);

    // Push the current values into the RTI
    virtual void updateAttributeValues(const RTIData& tag);
    virtual void updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag);
    // encode periodic and dirty attribute values for the next sendAttributeValues
    void encodeAttributeValues();
    // encode the attribute value at index i for the next sendAttributeValues
    void encodeAttributeValue(unsigned index);

    // Really sends the prepared attribute update values into the RTI
    void sendAttributeValues(const RTIData& tag);
    void sendAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag);

    class UpdateCallback : public SGReferenced {
    public:
        virtual ~UpdateCallback();

        virtual void updateAttributeValues(HLAObjectInstance&, const RTIData&) = 0;
        virtual void updateAttributeValues(HLAObjectInstance&, const SGTimeStamp&, const RTIData&) = 0;
    };

    void setUpdateCallback(const SGSharedPtr<UpdateCallback>& updateCallback)
    { _updateCallback = updateCallback; }
    const SGSharedPtr<UpdateCallback>& getUpdateCallback() const
    { return _updateCallback; }


    // Reflects the indices given in the index vector into the attributes HLADataElements.
    virtual void reflectAttributeValues(const HLAIndexList& indexList, const RTIData& tag);
    virtual void reflectAttributeValues(const HLAIndexList& indexList, const SGTimeStamp& timeStamp, const RTIData& tag);
    // Reflect a single attribute value at the given index into the attributes HLADataELement.
    virtual void reflectAttributeValue(unsigned index, const RTIData& tag);
    virtual void reflectAttributeValue(unsigned index, const SGTimeStamp& timeStamp, const RTIData& tag);

    class ReflectCallback : public SGReferenced {
    public:
        virtual ~ReflectCallback();

        virtual void reflectAttributeValues(HLAObjectInstance&, const HLAIndexList&, const RTIData&) = 0;
        virtual void reflectAttributeValues(HLAObjectInstance&, const HLAIndexList&, const SGTimeStamp&, const RTIData&) = 0;
    };

    void setReflectCallback(const SGSharedPtr<ReflectCallback>& reflectCallback)
    { _reflectCallback = reflectCallback; }
    const SGSharedPtr<ReflectCallback>& getReflectCallback() const
    { return _reflectCallback; }

private:
    void _setRTIObjectInstance(RTIObjectInstance* rtiObjectInstance);
    void _clearRTIObjectInstance();

    // The callback entry points from the RTI interface classes.
    void _removeInstance(const RTIData& tag);
    void _reflectAttributeValues(const HLAIndexList& indexList, const RTIData& tag);
    void _reflectAttributeValues(const HLAIndexList& indexList, const SGTimeStamp& timeStamp, const RTIData& tag);

    struct Attribute {
        Attribute() : _enabledUpdate(false), _unconditionalUpdate(false) {}
        SGSharedPtr<HLADataElement> _dataElement;
        bool _enabledUpdate;
        bool _unconditionalUpdate;
        // HLAIndexList::iterator _unconditionalUpdateAttributeIndexListIterator;
        // HLAIndexList::iterator _conditionalUpdateAttributeIndexListIterator;
    };
    typedef std::vector<Attribute> AttributeVector;

    // At some time we want these: Until then, use the _enabledUpdate and _unconditionalUpdate flags in the Attribute struct.
    // HLAIndexList _unconditionalUpdateAttributeIndexList;
    // HLAIndexList _conditionalUpdateAttributeIndexList;

    /// The parent Federate
    SGWeakPtr<HLAFederate> _federate;

    /// The ObjectClass
    SGSharedPtr<HLAObjectClass> _objectClass;

    /// The name as known in the RTI
    std::string _name;

    // /// The name as given by the local created instance
    // std::string _givenName;

    /// The underlying rti dispatcher class
    SGSharedPtr<RTIObjectInstance> _rtiObjectInstance;

    /// The attribute data
    AttributeVector _attributeVector;

    // Callback classes
    SGSharedPtr<UpdateCallback> _updateCallback;
    SGSharedPtr<ReflectCallback> _reflectCallback;

    friend class HLAFederate;
    friend class HLAObjectClass;
    friend class RTIObjectInstance;
};

} // namespace simgear

#endif
