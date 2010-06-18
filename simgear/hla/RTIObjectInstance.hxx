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

    // FIXME: factor out an ambassador base
    virtual void addToRequestQueue() = 0;

    virtual void deleteObjectInstance(const RTIData& tag) = 0;
    virtual void deleteObjectInstance(const SGTimeStamp& timeStamp, const RTIData& tag) = 0;
    virtual void localDeleteObjectInstance() = 0;

    virtual void requestObjectAttributeValueUpdate() = 0;

    virtual void updateAttributeValues(const RTIData& tag) = 0;
    virtual void updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag) = 0;

    void removeInstance(const RTIData& tag);
    // Call this if you want to roll up the queued timestamed updates
    // and reflect that into the attached data elements.
    void reflectQueuedAttributeValues(const SGTimeStamp& timeStamp)
    {
        // replay all updates up to the given timestamp
        UpdateListMap::iterator last = _updateListMap.upper_bound(timeStamp);
        for (UpdateListMap::iterator i = _updateListMap.begin(); i != last; ++i) {
            for (UpdateList::iterator j = i->second.begin(); j != i->second.end(); ++j) {
                // FIXME have a variant that takes the timestamp?
                reflectAttributeValues(j->_indexDataPairList, j->_tag);
            }
            putUpdateToPool(i->second);
        }
    }
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
            }
        }
        _attributeData.resize(numAttributes);
        for (; i < numAttributes; ++i) {
            if (getAttributePublished(i)) {
                _attributeData[i].setUpdateEnabled(true);
                _attributeData[i].setOwned(owned);
            } else {
                _attributeData[i].setUpdateEnabled(false);
                _attributeData[i].setOwned(false);
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
                addToRequestQueue();
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

    // Contains a full update as it came in from the RTI
    struct Update {
        RTIIndexDataPairList _indexDataPairList;
        RTIData _tag;
    };
    // A bunch of updates for the same timestamp
    typedef std::list<Update> UpdateList;
    // The timestamp sorted list of updates
    typedef std::map<SGTimeStamp, UpdateList> UpdateListMap;

    // The timestamped updates sorted by timestamp
    UpdateListMap _updateListMap;

    // The pool of unused updates so that we do not need to malloc/free each time
    UpdateList _updateList;

    void getUpdateFromPool(UpdateList& updateList)
    {
        if (_updateList.empty())
            updateList.push_back(Update());
        else
            updateList.splice(updateList.end(), _updateList, _updateList.begin());
    }
    void putUpdateToPool(UpdateList& updateList)
    {
        for (UpdateList::iterator i = updateList.begin(); i != updateList.end(); ++i)
            putDataToPool(i->_indexDataPairList);
        _updateList.splice(_updateList.end(), updateList);
    }

    // Appends the updates in the list to the given timestamps updates
    void scheduleUpdates(const SGTimeStamp& timeStamp, UpdateList& updateList)
    {
        UpdateListMap::iterator i = _updateListMap.find(timeStamp);
        if (i == _updateListMap.end())
            i = _updateListMap.insert(UpdateListMap::value_type(timeStamp, UpdateList())).first;
        i->second.splice(i->second.end(), updateList);
    }

    // This adds raw storage for attribute index i to the end of the dataPairList.
    void getDataFromPool(unsigned i, RTIIndexDataPairList& dataPairList)
    {
        if (_attributeData.size() <= i) {
            SG_LOG(SG_NETWORK, SG_WARN, "RTI: Invalid object attribute index!");
            return;
        }

        // Nothing left in the pool - so allocate something
        if (_attributeData[i]._indexDataPairList.empty()) {
            dataPairList.push_back(RTIIndexDataPairList::value_type());
            dataPairList.back().first = i;
            return;
        }

        // Take one from the pool
        dataPairList.splice(dataPairList.end(),
                            _attributeData[i]._indexDataPairList,
                            _attributeData[i]._indexDataPairList.begin());
    }

    void putDataToPool(RTIIndexDataPairList& dataPairList)
    {
        while (!dataPairList.empty()) {
            // Put back into the pool
            unsigned i = dataPairList.front().first;
            if (_attributeData.size() <= i) {
                // should not happen!!!
                SG_LOG(SG_NETWORK, SG_WARN, "RTI: Invalid object attribute index!");
                dataPairList.pop_front();
            } else {
                _attributeData[i]._indexDataPairList.splice(_attributeData[i]._indexDataPairList.begin(),
                                                            dataPairList, dataPairList.begin());
            }
        }
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
