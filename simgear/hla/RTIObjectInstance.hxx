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

#ifndef RTIObjectInstance_hxx
#define RTIObjectInstance_hxx

#include <string>
#include <vector>
#include "simgear/structure/SGReferenced.hxx"
#include "simgear/timing/timestamp.hxx"
#include "RTIData.hxx"
#include "RTIObjectClass.hxx"
#include "HLADataElement.hxx"

class SGTimeStamp;

namespace simgear {

class RTIObjectClass;
class HLAObjectInstance;

class RTIObjectInstance : public SGReferenced {
public:
    RTIObjectInstance(HLAObjectInstance* objectInstance);
    virtual ~RTIObjectInstance();

    void setObjectInstance(HLAObjectInstance* objectInstance)
    { _objectInstance = objectInstance; }

    virtual const RTIObjectClass* getObjectClass() const = 0;

    virtual std::string getName() const = 0;

    unsigned getNumAttributes() const;

    virtual void deleteObjectInstance(const RTIData& tag) = 0;
    virtual void deleteObjectInstance(const SGTimeStamp& timeStamp, const RTIData& tag) = 0;
    virtual void localDeleteObjectInstance() = 0;

    virtual void requestObjectAttributeValueUpdate(const HLAIndexList& indexList) = 0;

    virtual void updateAttributeValues(const RTIData& tag) = 0;
    virtual void updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag) = 0;

    virtual bool isAttributeOwnedByFederate(unsigned index) const = 0;

    void removeInstance(const RTIData& tag);
    void reflectAttributeValues(const HLAIndexList& indexList, const RTIData& tag);
    void reflectAttributeValues(const HLAIndexList& indexList, const SGTimeStamp& timeStamp, const RTIData& tag);

    bool encodeAttributeData(unsigned index, const HLADataElement& dataElement)
    {
        if (_attributeData.size() <= index)
            return false;
        return _attributeData[index].encodeAttributeData(dataElement);
    }

    bool decodeAttributeData(unsigned index, HLADataElement& dataElement) const
    {
        if (_attributeData.size() <= index)
            return false;
        return _attributeData[index].decodeAttributeData(dataElement);
    }

    bool getAttributeData(unsigned index, RTIData& data) const
    {
        if (_attributeData.size() <= index)
            return false;
        data = _attributeData[index]._data;
        return true;
    }

    bool getAttributeOwned(unsigned index) const
    {
        if (_attributeData.size() <= index)
            return false;
        return _attributeData[index]._owned;
    }

    void setAttributeInScope(unsigned i, bool inScope)
    {
        if (_attributeData.size() <= i)
            return;
        _attributeData[i]._inScope = inScope;
    }
    void setAttributeUpdateEnabled(unsigned i, bool enabled)
    {
        if (_attributeData.size() <= i)
            return;
        _attributeData[i]._updateEnabled = enabled;
    }

protected:
    // Initially set the number of attributes, do an initial query for the attribute ownership
    void _setNumAttributes(unsigned numAttributes)
    {
        _attributeData.resize(numAttributes);
        for (unsigned i = 0; i < numAttributes; ++i)
            _attributeData[i]._owned = isAttributeOwnedByFederate(i);
    }

    // The backward reference to the user visible object
    HLAObjectInstance* _objectInstance;

    struct AttributeData {
        AttributeData() : _owned(false), _inScope(true), _updateEnabled(true), _dirty(false)
        { }

        bool encodeAttributeData(const HLADataElement& dataElement)
        {
            _dirty = true;
            _data.resize(0);
            HLAEncodeStream stream(_data);
            return dataElement.encode(stream);
        }

        bool decodeAttributeData(HLADataElement& dataElement) const
        {
            HLADecodeStream stream(_data);
            return dataElement.decode(stream);
        }

        // The rti level raw data element
        RTIData _data;

        // The state of the attribute as tracked from the rti.
        bool _owned;
        bool _inScope;
        bool _updateEnabled;

        // Is set to true if _data has be reencoded
        bool _dirty;
    };
    std::vector<AttributeData> _attributeData;
};

}

#endif
