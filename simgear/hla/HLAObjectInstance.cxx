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
    _federate(objectClass->_federate),
    _objectClass(objectClass)
{
}

HLAObjectInstance::~HLAObjectInstance()
{
    _clearRTIObjectInstance();
}

unsigned
HLAObjectInstance::getNumAttributes() const
{
    return _objectClass->getNumAttributes();
}

unsigned
HLAObjectInstance::getAttributeIndex(const std::string& name) const
{
    return _objectClass->getAttributeIndex(name);
}

std::string
HLAObjectInstance::getAttributeName(unsigned index) const
{
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
    if (getAttributeOwned(index))
        encodeAttributeValue(index);
}

class HLAObjectInstance::DataElementFactoryVisitor : public HLADataElementFactoryVisitor {
public:
    DataElementFactoryVisitor(const HLAPathElementMap& pathElementMap) :
        _pathElementMap(pathElementMap)
    { }
    DataElementFactoryVisitor(const HLADataElement::Path& path, const HLAPathElementMap& pathElementMap) :
        _pathElementMap(pathElementMap),
        _path(path)
    { }
    virtual ~DataElementFactoryVisitor() {}

    virtual void apply(const HLADataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Can not find a suitable data element for data type \""
               << dataType.getName() << "\"");
    }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }

    class ArrayDataElementFactory : public HLAArrayDataElement::DataElementFactory {
    public:
        ArrayDataElementFactory(const HLADataElement::Path& path, const HLAPathElementMap& pathElementMap) :
            _path(path)
        {
            for (HLAPathElementMap::const_iterator i = pathElementMap.lower_bound(path);
                 i != pathElementMap.end(); ++i) {
                if (i->first.begin() != std::search(i->first.begin(), i->first.end(),
                                                    path.begin(), path.end()))
                    break;
                _pathElementMap.insert(*i);
            }
        }
        virtual HLADataElement* createElement(const HLAArrayDataElement& element, unsigned index)
        {
            const HLADataType* dataType = element.getElementDataType();
            if (!dataType)
                return 0;
            HLADataElement::Path path = _path;
            path.push_back(HLADataElement::PathElement(index));
            DataElementFactoryVisitor visitor(path, _pathElementMap);
            dataType->accept(visitor);
            return visitor._dataElement.release();
        }
    private:
        HLADataElement::Path _path;
        HLAPathElementMap _pathElementMap;
    };

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        SGSharedPtr<HLAArrayDataElement> arrayDataElement;
        arrayDataElement = new HLAArrayDataElement(&dataType);
        arrayDataElement->setDataElementFactory(new ArrayDataElementFactory(_path, _pathElementMap));
        arrayDataElement->setNumElements(dataType.getNumElements());

        _dataElement = arrayDataElement;
    }

    virtual void apply(const HLAVariableArrayDataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        SGSharedPtr<HLAArrayDataElement> arrayDataElement;
        arrayDataElement = new HLAArrayDataElement(&dataType);
        arrayDataElement->setDataElementFactory(new ArrayDataElementFactory(_path, _pathElementMap));

        _dataElement = arrayDataElement;
    }

    virtual void apply(const HLAEnumeratedDataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        HLADataElementFactoryVisitor::apply(dataType);
    }

    virtual void apply(const HLAFixedRecordDataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        SGSharedPtr<HLAFixedRecordDataElement> recordDataElement;
        recordDataElement = new HLAFixedRecordDataElement(&dataType);

        unsigned numFields = dataType.getNumFields();
        for (unsigned i = 0; i < numFields; ++i) {

            _path.push_back(HLADataElement::PathElement(dataType.getFieldName(i)));

            dataType.getFieldDataType(i)->accept(*this);
            recordDataElement->setField(i, _dataElement.release());

            _path.pop_back();
        }
        _dataElement = recordDataElement;
    }

    class VariantRecordDataElementFactory : public HLAVariantRecordDataElement::DataElementFactory {
    public:
        VariantRecordDataElementFactory(const HLADataElement::Path& path, const HLAPathElementMap& pathElementMap) :
            _path(path)
        {
            for (HLAPathElementMap::const_iterator i = pathElementMap.lower_bound(path);
                 i != pathElementMap.end(); ++i) {
                if (i->first.begin() != std::search(i->first.begin(), i->first.end(),
                                                    path.begin(), path.end()))
                    break;
                _pathElementMap.insert(*i);
            }
        }
        virtual HLADataElement* createElement(const HLAVariantRecordDataElement& element, unsigned index)
        {
            const HLAVariantRecordDataType* dataType = element.getDataType();
            if (!dataType)
                return 0;
            const HLADataType* alternativeDataType = element.getAlternativeDataType();
            if (!alternativeDataType)
                return 0;
            HLADataElement::Path path = _path;
            path.push_back(HLADataElement::PathElement(dataType->getAlternativeName(index)));
            DataElementFactoryVisitor visitor(path, _pathElementMap);
            alternativeDataType->accept(visitor);
            return visitor._dataElement.release();
        }
    private:
        HLADataElement::Path _path;
        HLAPathElementMap _pathElementMap;
    };

    virtual void apply(const HLAVariantRecordDataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        SGSharedPtr<HLAVariantRecordDataElement> variantRecordDataElement;
        variantRecordDataElement = new HLAVariantRecordDataElement(&dataType);
        variantRecordDataElement->setDataElementFactory(new VariantRecordDataElementFactory(_path, _pathElementMap));

        _dataElement = variantRecordDataElement;
    }

private:
    SGSharedPtr<HLADataElement> createDataElement(const HLADataElement::Path& path, const HLADataType& dataType)
    {
        HLAPathElementMap::const_iterator i = _pathElementMap.find(path);
        if (i == _pathElementMap.end()) {
            SG_LOG(SG_IO, SG_WARN, "No dataElement provided for \""
                   << HLADataElement::toString(path) << "\".");

            return 0;
        }
        SGSharedPtr<HLADataElement> dataElement = i->second.getDataElement(path);
        if (!dataElement->setDataType(&dataType)) {
            SG_LOG(SG_IO, SG_ALERT, "Cannot set data type for data element at \""
                   << HLADataElement::toString(path) <<  "\"!");
            return 0;
        }
        SG_LOG(SG_IO, SG_DEBUG, "Using provided dataElement for \""
               << HLADataElement::toString(path) << "\".");
        return dataElement;
    }

    const HLAPathElementMap& _pathElementMap;
    HLADataElement::Path _path;
};

void
HLAObjectInstance::setAttribute(unsigned index, const HLAPathElementMap& pathElementMap)
{
    const HLADataType* dataType = getAttributeDataType(index);
    if (!dataType) {
        SG_LOG(SG_IO, SG_ALERT, "Cannot get attribute data type for setting attribute \""
               << getAttributeName(index) << "\" at index " << index << "!");
        return;
    }

    SG_LOG(SG_IO, SG_DEBUG, "Setting DataElement for attribute \""
           << getAttributeName(index) << "\".");

    DataElementFactoryVisitor visitor(pathElementMap);
    dataType->accept(visitor);
    setAttributeDataElement(index, visitor.getDataElement());
}

void
HLAObjectInstance::setAttributes(const HLAAttributePathElementMap& attributePathElementMap)
{
    for (HLAAttributePathElementMap::const_iterator i = attributePathElementMap.begin();
         i != attributePathElementMap.end(); ++i) {
        setAttribute(i->first, i->second);
    }
}

void
HLAObjectInstance::registerInstance()
{
    if (_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to register object " << getName() << " already known to the RTI!");
        return;
    }
    if (!_objectClass.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Could not register object with unknown object class!");
        return;
    }
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
HLAObjectInstance::updateAttributeValues(const RTIData& tag)
{
    if (_attributeCallback.valid())
        _attributeCallback->updateAttributeValues(*this, tag);
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
    if (_attributeCallback.valid())
        _attributeCallback->updateAttributeValues(*this, tag);
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
        if (!_attributeVector[i]._unconditionalUpdate)
            continue;
        encodeAttributeValue(i);
    }
}

void
HLAObjectInstance::encodeAttributeValue(unsigned index)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_INFO, "Not updating inactive object!");
        return;
    }
    const HLADataElement* dataElement = getAttributeDataElement(index);
    if (!dataElement)
        return;
    _rtiObjectInstance->encodeAttributeData(index, *dataElement);
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
    _rtiObjectInstance = rtiObjectInstance;
    _rtiObjectInstance->setObjectInstance(this);
    _name = _rtiObjectInstance->getName();

    unsigned numAttributes = getNumAttributes();
    _attributeVector.resize(numAttributes);
    for (unsigned i = 0; i < numAttributes; ++i) {
        HLAUpdateType updateType = getObjectClass()->getAttributeUpdateType(i);
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
        HLAUpdateType updateType = getObjectClass()->getAttributeUpdateType(i);
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
    } else if (_attributeCallback.valid()) {
        reflectAttributeValues(indexList, tag);

        RTIIndexDataPairList dataPairList;
        for (HLAIndexList::const_iterator i = indexList.begin(); i != indexList.end(); ++i) {
            dataPairList.push_back(RTIIndexDataPair());
            dataPairList.back().first = *i;
            getAttributeData(*i, dataPairList.back().second);
        }
        _attributeCallback->reflectAttributeValues(*this, dataPairList, tag);
    } else {
        reflectAttributeValues(indexList, tag);
    }
}

void
HLAObjectInstance::_reflectAttributeValues(const HLAIndexList& indexList, const SGTimeStamp& timeStamp, const RTIData& tag)
{
    if (_reflectCallback.valid()) {
        _reflectCallback->reflectAttributeValues(*this, indexList, timeStamp, tag);
    } else if (_attributeCallback.valid()) {
        reflectAttributeValues(indexList, timeStamp, tag);

        RTIIndexDataPairList dataPairList;
        for (HLAIndexList::const_iterator i = indexList.begin(); i != indexList.end(); ++i) {
            dataPairList.push_back(RTIIndexDataPair());
            dataPairList.back().first = *i;
            getAttributeData(*i, dataPairList.back().second);
        }
        _attributeCallback->reflectAttributeValues(*this, dataPairList, timeStamp, tag);
    } else {
        reflectAttributeValues(indexList, timeStamp, tag);
    }
}

} // namespace simgear
