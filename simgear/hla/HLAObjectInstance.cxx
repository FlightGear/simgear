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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include "HLAObjectInstance.hxx"

#include <algorithm>
#include "simgear/debug/logstream.hxx"
#include "HLAArrayDataElement.hxx"
#include "HLABasicDataElement.hxx"
#include "HLADataElement.hxx"
#include "HLAEnumeratedDataElement.hxx"
#include "HLAFederate.hxx"
#include "HLAFixedRecordDataElement.hxx"
#include "HLAObjectClass.hxx"
#include "HLAVariantRecordDataElement.hxx"
#include "RTIObjectClass.hxx"
#include "RTIObjectInstance.hxx"

namespace simgear {

HLAObjectInstance::UpdateCallback::~UpdateCallback()
{
}

HLAObjectInstance::ReflectCallback::~ReflectCallback()
{
}

HLAObjectInstance::HLAObjectInstance(HLAObjectClass* objectClass) :
    _objectClass(objectClass)
{
    if (objectClass)
        _federate = objectClass->_federate;
}

HLAObjectInstance::~HLAObjectInstance()
{
    _clearRTIObjectInstance();
}

const std::string&
HLAObjectInstance::getName() const
{
    return _name;
}

const SGWeakPtr<HLAFederate>&
HLAObjectInstance::getFederate() const
{
    return _federate;
}

const SGSharedPtr<HLAObjectClass>&
HLAObjectInstance::getObjectClass() const
{
    return _objectClass;
}

unsigned
HLAObjectInstance::getNumAttributes() const
{
    if (!_objectClass.valid())
        return 0;
    return _objectClass->getNumAttributes();
}

unsigned
HLAObjectInstance::getAttributeIndex(const std::string& name) const
{
    if (!_objectClass.valid())
        return ~0u;
    return _objectClass->getAttributeIndex(name);
}

std::string
HLAObjectInstance::getAttributeName(unsigned index) const
{
    if (!_objectClass.valid())
        return std::string();
    return _objectClass->getAttributeName(index);
}

bool
HLAObjectInstance::getAttributeOwned(unsigned index) const
{
    if (!_rtiObjectInstance.valid())
        return false;
    return _rtiObjectInstance->getAttributeOwned(index);
}

const HLADataType*
HLAObjectInstance::getAttributeDataType(unsigned index) const
{
    if (!_objectClass.valid())
        return 0;
    return _objectClass->getAttributeDataType(index);
}

HLADataElement*
HLAObjectInstance::getAttributeDataElement(unsigned index)
{
    if (_attributeVector.size() <= index)
        return 0;
    return _attributeVector[index]._dataElement.get();
}

const HLADataElement*
HLAObjectInstance::getAttributeDataElement(unsigned index) const
{
    if (_attributeVector.size() <= index)
        return 0;
    return _attributeVector[index]._dataElement.get();
}

bool
HLAObjectInstance::getAttributeData(unsigned index, RTIData& data) const
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to get raw attribute data without rti object instance for \"" << getName() << "\"!");
        return false;
    }
    return _rtiObjectInstance->getAttributeData(index, data);
}

void
HLAObjectInstance::setAttributeDataElement(unsigned index, const SGSharedPtr<HLADataElement>& dataElement)
{
    unsigned numAttributes = getNumAttributes();
    if (numAttributes <= index)
        return;
    _attributeVector.resize(numAttributes);
    if (_attributeVector[index]._dataElement.valid())
        _attributeVector[index]._dataElement->clearStamp();
    _attributeVector[index]._dataElement = dataElement;
    if (_attributeVector[index]._dataElement.valid())
        _attributeVector[index]._dataElement->createStamp();
}

bool
HLAObjectInstance::getDataElementIndex(HLADataElementIndex& index, const std::string& path) const
{
    HLAObjectClass* objectClass = getObjectClass().get();
    if (!objectClass) {
        SG_LOG(SG_IO, SG_ALERT, "Could not get the data element index of an object instance with unknown class!");
        return false;
    }
    return objectClass->getDataElementIndex(index, path);
}

HLADataElementIndex
HLAObjectInstance::getDataElementIndex(const std::string& path) const
{
    HLADataElementIndex dataElementIndex;
    getDataElementIndex(dataElementIndex, path);
    return dataElementIndex;
}

HLADataElement*
HLAObjectInstance::getAttributeDataElement(const HLADataElementIndex& index)
{
    if (index.empty())
        return 0;
    HLADataElement* dataElement = getAttributeDataElement(index[0]);
    if (!dataElement)
        return 0;
    return dataElement->getDataElement(index.begin() + 1, index.end());
}

const HLADataElement*
HLAObjectInstance::getAttributeDataElement(const HLADataElementIndex& index) const
{
    if (index.empty())
        return 0;
    const HLADataElement* dataElement = getAttributeDataElement(index[0]);
    if (!dataElement)
        return 0;
    return dataElement->getDataElement(index.begin() + 1, index.end());
}

void
HLAObjectInstance::setAttributeDataElement(const HLADataElementIndex& index, const SGSharedPtr<HLADataElement>& dataElement)
{
    if (index.empty())
        return;
    if (index.size() == 1) {
        if (!getAttributeDataType(index[0]))
            return;
        if (dataElement.valid() && !dataElement->setDataType(getAttributeDataType(index[0])))
            return;
        setAttributeDataElement(index[0], dataElement);
    } else {
        SGSharedPtr<HLADataElement> attributeDataElement = getAttributeDataElement(index[0]);
        if (!attributeDataElement.valid()) {
            createAndSetAttributeDataElement(index[0]);
            attributeDataElement = getAttributeDataElement(index[0]);
        }
        if (!attributeDataElement.valid())
            return;
        attributeDataElement->setDataElement(index.begin() + 1, index.end(), dataElement.get());
    }
}

HLADataElement*
HLAObjectInstance::getAttributeDataElement(const std::string& path)
{
    HLADataElementIndex index;
    if (!getDataElementIndex(index, path))
        return 0;
    return getAttributeDataElement(index); 
}

const HLADataElement*
HLAObjectInstance::getAttributeDataElement(const std::string& path) const
{
    HLADataElementIndex index;
    if (!getDataElementIndex(index, path))
        return 0;
    return getAttributeDataElement(index); 
}

void
HLAObjectInstance::setAttributeDataElement(const std::string& path, const SGSharedPtr<HLADataElement>& dataElement)
{
    HLADataElementIndex index;
    if (!getDataElementIndex(index, path))
        return;
    setAttributeDataElement(index, dataElement); 
}

void
HLAObjectInstance::discoverInstance(const RTIData& tag)
{
    HLAObjectClass* objectClass = getObjectClass().get();
    if (!objectClass) {
        SG_LOG(SG_IO, SG_ALERT, "Could not discover instance of unknown object class!");
        return;
    }
    objectClass->discoverInstance(*this, tag);
}

void
HLAObjectInstance::removeInstance(const RTIData& tag)
{
    HLAObjectClass* objectClass = getObjectClass().get();
    if (!objectClass) {
        SG_LOG(SG_IO, SG_ALERT, "Could not remove instance of unknown object class!");
        return;
    }
    objectClass->removeInstance(*this, tag);
}

void
HLAObjectInstance::registerInstance()
{
    registerInstance(_objectClass.get());
}

void
HLAObjectInstance::registerInstance(HLAObjectClass* objectClass)
{
    if (_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to register object " << getName() << " already known to the RTI!");
        return;
    }
    if (!objectClass) {
        SG_LOG(SG_IO, SG_ALERT, "Could not register object with unknown object class!");
        return;
    }
    if (_objectClass.valid() && objectClass != _objectClass.get()) {
        SG_LOG(SG_IO, SG_ALERT, "Could not change object class while registering!");
        return;
    }
    _objectClass = objectClass;
    _federate = _objectClass->_federate;
    // This error must have been flagged before
    if (!_objectClass->_rtiObjectClass.valid())
        return;
    _setRTIObjectInstance(_objectClass->_rtiObjectClass->registerObjectInstance(this));
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Could not register object at the RTI!");
        return;
    }
    _objectClass->_registerInstance(this);
}

void
HLAObjectInstance::deleteInstance(const RTIData& tag)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to delete inactive object!");
        return;
    }
    if (!_objectClass.valid())
        return;
    _objectClass->_deleteInstance(*this);
    _rtiObjectInstance->deleteObjectInstance(tag);
}

void
HLAObjectInstance::createAttributeDataElements()
{
    HLAObjectClass* objectClass = getObjectClass().get();
    if (!objectClass) {
        SG_LOG(SG_IO, SG_ALERT, "Could not create data elements for instance of unknown object class!");
        return;
    }
    objectClass->createAttributeDataElements(*this);
}

void
HLAObjectInstance::createAndSetAttributeDataElement(unsigned index)
{
    if (getAttributeDataElement(index)) {
        SG_LOG(SG_IO, SG_DEBUG, "Attribute data element for attribute \""
               << getAttributeName(index) << "\" is already set.");
        return;
    }
    SGSharedPtr<HLADataElement> dataElement = createAttributeDataElement(index);
    setAttributeDataElement(index, dataElement);
}

HLADataElement*
HLAObjectInstance::createAttributeDataElement(unsigned index)
{
    HLAObjectClass* objectClass = getObjectClass().get();
    if (!objectClass) {
        SG_LOG(SG_IO, SG_ALERT, "Could not create data element for instance of unknown object class!");
        return 0;
    }
    return objectClass->createAttributeDataElement(*this, index);
}

void
HLAObjectInstance::updateAttributeValues(const RTIData& tag)
{
    if (_updateCallback.valid()) {
        _updateCallback->updateAttributeValues(*this, tag);
    } else {
        encodeAttributeValues();
        sendAttributeValues(tag);
    }
}

void
HLAObjectInstance::updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag)
{
    if (_updateCallback.valid()) {
        _updateCallback->updateAttributeValues(*this, timeStamp, tag);
    } else {
        encodeAttributeValues();
        sendAttributeValues(timeStamp, tag);
    }
}

void
HLAObjectInstance::encodeAttributeValues()
{
    unsigned numAttributes = _attributeVector.size();
    for (unsigned i = 0; i < numAttributes;++i) {
        if (_attributeVector[i]._unconditionalUpdate) {
            encodeAttributeValue(i);
        } else if (_attributeVector[i]._enabledUpdate) {
            const HLADataElement* dataElement = getAttributeDataElement(i);
            if (dataElement && dataElement->getDirty())
                encodeAttributeValue(i);
        }
    }
}

void
HLAObjectInstance::encodeAttributeValue(unsigned index)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_INFO, "Not updating inactive object!");
        return;
    }
    HLADataElement* dataElement = getAttributeDataElement(index);
    if (!dataElement)
        return;
    _rtiObjectInstance->encodeAttributeData(index, *dataElement);
    dataElement->setDirty(false);
}

void
HLAObjectInstance::sendAttributeValues(const RTIData& tag)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_INFO, "Not updating inactive object!");
        return;
    }
    _rtiObjectInstance->updateAttributeValues(tag);
}

void
HLAObjectInstance::sendAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_INFO, "Not updating inactive object!");
        return;
    }
    _rtiObjectInstance->updateAttributeValues(timeStamp, tag);
}

void
HLAObjectInstance::reflectAttributeValues(const HLAIndexList& indexList, const RTIData& tag)
{
    for (HLAIndexList::const_iterator i = indexList.begin(); i != indexList.end(); ++i)
        reflectAttributeValue(*i, tag);
}

void
HLAObjectInstance::reflectAttributeValues(const HLAIndexList& indexList,
                                          const SGTimeStamp& timeStamp, const RTIData& tag)
{
    for (HLAIndexList::const_iterator i = indexList.begin(); i != indexList.end(); ++i)
        reflectAttributeValue(*i, timeStamp, tag);
}

void
HLAObjectInstance::reflectAttributeValue(unsigned index, const RTIData& tag)
{
    HLADataElement* dataElement = getAttributeDataElement(index);
    if (!dataElement)
        return;
    dataElement->setTimeStampValid(false);
    _rtiObjectInstance->decodeAttributeData(index, *dataElement);
}

void
HLAObjectInstance::reflectAttributeValue(unsigned index, const SGTimeStamp& timeStamp, const RTIData& tag)
{
    HLADataElement* dataElement = getAttributeDataElement(index);
    if (!dataElement)
        return;
    dataElement->setTimeStamp(timeStamp);
    dataElement->setTimeStampValid(true);
    _rtiObjectInstance->decodeAttributeData(index, *dataElement);
}

void
HLAObjectInstance::_setRTIObjectInstance(RTIObjectInstance* rtiObjectInstance)
{
    if (!_objectClass.valid())
        return;

    _rtiObjectInstance = rtiObjectInstance;
    _rtiObjectInstance->setObjectInstance(this);
    _name = _rtiObjectInstance->getName();

    unsigned numAttributes = getNumAttributes();
    _attributeVector.resize(numAttributes);
    for (unsigned i = 0; i < numAttributes; ++i) {
        HLAUpdateType updateType = _objectClass->getAttributeUpdateType(i);
        if (getAttributeOwned(i) && updateType != HLAUndefinedUpdate) {
            _attributeVector[i]._enabledUpdate = true;
            _attributeVector[i]._unconditionalUpdate = (updateType == HLAPeriodicUpdate);
            // In case of an owned attribute, now encode its value
            encodeAttributeValue(i);
        } else {
            _attributeVector[i]._enabledUpdate = false;
            _attributeVector[i]._unconditionalUpdate = false;
        }
    }

    // This makes sense with any new object. Even if we registered one, there might be unpublished attributes.
    HLAIndexList indexList;
    for (unsigned i = 0; i < numAttributes; ++i) {
        HLAUpdateType updateType = _objectClass->getAttributeUpdateType(i);
        if (getAttributeOwned(i))
            continue;
        if (updateType == HLAUndefinedUpdate)
            continue;
        if (updateType == HLAPeriodicUpdate)
            continue;
        indexList.push_back(i);
    }
    _rtiObjectInstance->requestObjectAttributeValueUpdate(indexList);
}

void
HLAObjectInstance::_clearRTIObjectInstance()
{
    if (!_rtiObjectInstance.valid())
        return;

    for (unsigned i = 0; i < _attributeVector.size(); ++i) {
        _attributeVector[i]._enabledUpdate = false;
        _attributeVector[i]._unconditionalUpdate = false;
    }

    _rtiObjectInstance->setObjectInstance(0);
    _rtiObjectInstance = 0;
}

void
HLAObjectInstance::_removeInstance(const RTIData& tag)
{
    if (!_objectClass.valid())
        return;
    _objectClass->_removeInstance(*this, tag);
}

void
HLAObjectInstance::_reflectAttributeValues(const HLAIndexList& indexList, const RTIData& tag)
{
    if (_reflectCallback.valid()) {
        _reflectCallback->reflectAttributeValues(*this, indexList, tag);
    } else {
        reflectAttributeValues(indexList, tag);
    }
}

void
HLAObjectInstance::_reflectAttributeValues(const HLAIndexList& indexList, const SGTimeStamp& timeStamp, const RTIData& tag)
{
    if (_reflectCallback.valid()) {
        _reflectCallback->reflectAttributeValues(*this, indexList, timeStamp, tag);
    } else {
        reflectAttributeValues(indexList, timeStamp, tag);
    }
}

} // namespace simgear
