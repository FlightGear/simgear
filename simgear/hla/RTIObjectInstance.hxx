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

#include <list>
#include <map>
#include <string>
#include <vector>
#include "simgear/debug/logstream.hxx"
#include "simgear/structure/SGReferenced.hxx"
#include "simgear/structure/SGWeakPtr.hxx"
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
    RTIObjectInstance(HLAObjectInstance* hlaObjectInstance);
    virtual ~RTIObjectInstance();

    virtual const RTIObjectClass* getObjectClass() const = 0;

    virtual std::string getName() const = 0;

    unsigned getNumAttributes() const;
    unsigned getAttributeIndex(const std::string& name) const;
    std::string getAttributeName(unsigned index) const;

    virtual void deleteObjectInstance(const RTIData& tag) = 0;
    virtual void deleteObjectInstance(const SGTimeStamp& timeStamp, const RTIData& tag) = 0;
    virtual void localDeleteObjectInstance() = 0;

    virtual void requestObjectAttributeValueUpdate() = 0;

    virtual void updateAttributeValues(const RTIData& tag) = 0;
    virtual void updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag) = 0;

    void removeInstance(const RTIData& tag);
    void reflectAttributeValues(const RTIIndexDataPairList& dataPairList, const RTIData& tag);
    void reflectAttributeValues(const RTIIndexDataPairList& dataPairList, const SGTimeStamp& timeStamp, const RTIData& tag);
    void reflectAttributeValue(unsigned i, const RTIData& data)
    {
        if (_attributeData.size() <= i)
            return;
        HLADataElement* dataElement = _attributeData[i]._dataElement.get();
        if (!dataElement)
            return;
        HLADecodeStream stream(data);
        dataElement->decode(stream);
    }

    const HLADataType* getAttributeDataType(unsigned i) const
    {
        return getObjectClass()->getAttributeDataType(i);
    }
    HLAUpdateType getAttributeUpdateType(unsigned i) const
    {
        return getObjectClass()->getAttributeUpdateType(i);
    }
    bool getAttributeSubscribed(unsigned i) const
    {
        return getObjectClass()->getAttributeSubscribed(i);
    }
    bool getAttributePublished(unsigned i) const
    {
        return getObjectClass()->getAttributePublished(i);
    }

    HLADataElement* getDataElement(unsigned i)
    {
        if (_attributeData.size() <= i)
            return 0;
        return _attributeData[i]._dataElement.get();
    }
    const HLADataElement* getDataElement(unsigned i) const
    {
        if (_attributeData.size() <= i)
            return 0;
        return _attributeData[i]._dataElement.get();
    }
    void setDataElement(unsigned i, HLADataElement* dataElement)
    {
        if (_attributeData.size() <= i)
            return;
        _attributeData[i]._dataElement = dataElement;
    }

    void updateAttributesFromClass(bool owned)
    {
        // FIXME: rethink that!!!
        unsigned numAttributes = getNumAttributes();
        unsigned i = 0;
        for (; i < _attributeData.size(); ++i) {
            if (getAttributePublished(i)) {
            } else {
                _attributeData[i].setUpdateEnabled(false);
                _attributeData[i].setOwned(false);
                if (getAttributeSubscribed(i))
                    _attributeData[i].setRequestUpdate(true);
            }
        }
        _attributeData.resize(numAttributes);
        for (; i < numAttributes; ++i) {
            if (getAttributePublished(i)) {
                _attributeData[i].setUpdateEnabled(true);
                _attributeData[i].setOwned(owned);
                if (!owned && getAttributeSubscribed(i))
                    _attributeData[i].setRequestUpdate(true);
            } else {
                _attributeData[i].setUpdateEnabled(false);
                _attributeData[i].setOwned(false);
                if (getAttributeSubscribed(i))
                    _attributeData[i].setRequestUpdate(true);
            }
        }
    }

    void setAttributeForceUpdate(unsigned i)
    {
        if (_attributeData.size() <= i)
            return;
        _attributeData[i].setForceUpdate(true);
    }
    void setAttributeInScope(unsigned i, bool inScope)
    {
        if (_attributeData.size() <= i)
            return;
        _attributeData[i].setInScope(inScope);
    }
    void setAttributeUpdateEnabled(unsigned i, bool enabled)
    {
        if (_attributeData.size() <= i)
            return;
        _attributeData[i].setUpdateEnabled(enabled);
    }
    void setAttributeUpdated(unsigned i)
    {
        if (_attributeData.size() <= i)
            return;
        _attributeData[i].setForceUpdate(false);
    }
    bool getAttributeEffectiveUpdateEnabled(unsigned i)
    {
        if (_attributeData.size() <= i)
            return false;
        if (!getAttributePublished(i))
            return false;
        if (!_attributeData[i]._updateEnabled)
            return false;
        if (!_attributeData[i]._inScope)
            return false;
        if (_attributeData[i]._forceUpdate)
            return true;
        switch (getAttributeUpdateType(i)) {
        case HLAPeriodicUpdate:
            return true;
        case HLAConditionalUpdate:
            return true; // FIXME
        case HLAStaticUpdate:
            return false;
        default:
            return false;
        }
    }
    void setRequestAttributeUpdate(bool request)
    {
        for (unsigned i = 0; i < getNumAttributes(); ++i) {
            if (getAttributeUpdateType(i) == HLAPeriodicUpdate)
                continue;
            setRequestAttributeUpdate(i, request);
        }
    }
    void setRequestAttributeUpdate(unsigned i, bool request)
    {
        if (_attributeData.size() <= i)
            return;
        _attributeData[i].setRequestUpdate(request);
        if (request) {
            if (!_pendingAttributeUpdateRequest) {
                _pendingAttributeUpdateRequest = true;
            }
        }
    }
    bool getRequestAttributeUpdate(unsigned i) const
    {
        if (_attributeData.size() <= i)
            return false;
        return _attributeData[i]._requestUpdate;
    }

    void flushPendingRequests()
    {
        if (_pendingAttributeUpdateRequest) {
            requestObjectAttributeValueUpdate();
            _pendingAttributeUpdateRequest = false;
        }
    }

protected:
    // The backward reference to the user visible object
    SGWeakPtr<HLAObjectInstance> _hlaObjectInstance;

    // Is true if we should emit a requestattr
    bool _pendingAttributeUpdateRequest;

    // Pool of update list entries
    RTIIndexDataPairList _indexDataPairList;

    // This adds raw storage for attribute index i to the end of the dataPairList.
    void getDataFromPool(RTIIndexDataPairList& dataPairList)
    {
        // Nothing left in the pool - so allocate something
        if (_indexDataPairList.empty()) {
            dataPairList.push_back(RTIIndexDataPairList::value_type());
            return;
        }

        // Take one from the pool
        dataPairList.splice(dataPairList.end(), _indexDataPairList, _indexDataPairList.begin());
    }

    void putDataToPool(RTIIndexDataPairList& dataPairList)
    {
        _indexDataPairList.splice(_indexDataPairList.begin(), dataPairList);
    }

    struct AttributeData {
        AttributeData() : _owned(false), _inScope(true), _updateEnabled(true), _forceUpdate(false), _requestUpdate(false)
        { }

        // The hla level data element with tha actual local program
        // accessible data.
        SGSharedPtr<HLADataElement> _dataElement;
        // SGSharedPtr<HLADataElement::TimeStamp> _timeStamp;

        // Pool of already allocated raw data used for reflection of updates
        RTIIndexDataPairList _indexDataPairList;

        void setOwned(bool owned)
        { _owned = owned; }
        void setInScope(bool inScope)
        { _inScope = inScope; }
        void setUpdateEnabled(bool updateEnabled)
        { _updateEnabled = updateEnabled; }
        void setForceUpdate(bool forceUpdate)
        { _forceUpdate = forceUpdate; }
        void setRequestUpdate(bool requestUpdate)
        { _requestUpdate = requestUpdate; }

        bool _owned;
        bool _inScope;
        bool _updateEnabled;
        bool _forceUpdate;
        bool _requestUpdate;
    };
    std::vector<AttributeData> _attributeData;

    friend class HLAObjectInstance;
};

}

#endif
