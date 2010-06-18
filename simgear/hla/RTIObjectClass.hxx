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

#ifndef RTIObjectClass_hxx
#define RTIObjectClass_hxx

#include <string>
#include <vector>
#include "simgear/structure/SGReferenced.hxx"
#include "simgear/structure/SGSharedPtr.hxx"
#include "simgear/structure/SGWeakPtr.hxx"
#include "RTIData.hxx"
#include "HLAObjectClass.hxx"

namespace simgear {

class RTIObjectInstance;
class HLAObjectClass;

class RTIObjectClass : public SGReferenced {
public:
    RTIObjectClass(HLAObjectClass* hlaObjectClass);
    virtual ~RTIObjectClass();

    virtual std::string getName() const = 0;

    virtual unsigned getNumAttributes() const = 0;
    virtual unsigned getAttributeIndex(const std::string& name) const = 0;
    virtual unsigned getOrCreateAttributeIndex(const std::string& name) = 0;

    virtual bool publish(const std::set<unsigned>& indexSet) = 0;
    virtual bool unpublish() = 0;

    virtual bool subscribe(const std::set<unsigned>& indexSet, bool) = 0;
    virtual bool unsubscribe() = 0;

    // Factory to create an object instance that can be used in this current federate
    virtual RTIObjectInstance* registerObjectInstance(HLAObjectInstance*) = 0;

    void discoverInstance(RTIObjectInstance* objectInstance, const RTIData& tag) const;

    void startRegistration() const;
    void stopRegistration() const;

    void setAttributeDataType(unsigned index, SGSharedPtr<const HLADataType> dataType)
    {
        if (_attributeDataVector.size() <= index)
            return;
        _attributeDataVector[index]._dataType = dataType;
    }
    const HLADataType* getAttributeDataType(unsigned index) const
    {
        if (_attributeDataVector.size() <= index)
            return 0;
        return _attributeDataVector[index]._dataType.get();
    }

    HLAUpdateType getAttributeUpdateType(unsigned index) const
    {
        if (_attributeDataVector.size() <= index)
            return HLAUndefinedUpdate;
        return _attributeDataVector[index]._updateType;
    }
    void setAttributeUpdateType(unsigned index, HLAUpdateType updateType)
    {
        if (_attributeDataVector.size() <= index)
            return;
        _attributeDataVector[index]._updateType = updateType;
    }

    bool getAttributeSubscribed(unsigned index) const
    {
        if (_attributeDataVector.size() <= index)
            return false;
        return _attributeDataVector[index]._subscribed;
    }
    bool getAttributePublished(unsigned index) const
    {
        if (_attributeDataVector.size() <= index)
            return false;
        return _attributeDataVector[index]._published;
    }
    std::string getAttributeName(unsigned index) const
    {
        if (_attributeDataVector.size() <= index)
            return std::string();
        return _attributeDataVector[index]._name;
    }

protected:
    struct AttributeData {
        AttributeData(const std::string& name) : _name(name), _subscribed(false), _published(false), _updateType(HLAUndefinedUpdate) {}
        std::string _name;
        SGSharedPtr<const HLADataType> _dataType;
        bool _subscribed;
        bool _published;
        HLAUpdateType _updateType;
    };
    typedef std::vector<AttributeData> AttributeDataVector;
    AttributeDataVector _attributeDataVector;

private:
    SGWeakPtr<HLAObjectClass> _hlaObjectClass;
};

}

#endif
