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

#include "HLAObjectInstance.hxx"

#include <algorithm>
#include "HLAArrayDataElement.hxx"
#include "HLABasicDataElement.hxx"
#include "HLADataElement.hxx"
#include "HLAEnumeratedDataElement.hxx"
#include "HLAFixedRecordDataElement.hxx"
#include "HLAObjectClass.hxx"
#include "HLAVariantDataElement.hxx"
#include "RTIObjectClass.hxx"
#include "RTIObjectInstance.hxx"

namespace simgear {

HLAObjectInstance::HLAObjectInstance(HLAObjectClass* objectClass) :
    _objectClass(objectClass)
{
}

HLAObjectInstance::HLAObjectInstance(HLAObjectClass* objectClass, RTIObjectInstance* rtiObjectInstance) :
    _objectClass(objectClass),
    _rtiObjectInstance(rtiObjectInstance)
{
    _rtiObjectInstance->_hlaObjectInstance = this;
    _name = _rtiObjectInstance->getName();
}

HLAObjectInstance::~HLAObjectInstance()
{
}

SGSharedPtr<HLAObjectClass>
HLAObjectInstance::getObjectClass() const
{
    return _objectClass.lock();
}

unsigned
HLAObjectInstance::getNumAttributes() const
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to get number of attributes for inactive object!");
        return 0;
    }
    return _rtiObjectInstance->getNumAttributes();
}

unsigned
HLAObjectInstance::getAttributeIndex(const std::string& name) const
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to get attribute index for inactive object!");
        return 0;
    }
    return _rtiObjectInstance->getAttributeIndex(name);
}

std::string
HLAObjectInstance::getAttributeName(unsigned index) const
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to get attribute name for inactive object!");
        return std::string();
    }
    return _rtiObjectInstance->getAttributeName(index);
}

const HLADataType*
HLAObjectInstance::getAttributeDataType(unsigned index) const
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to get attribute index for inactive object!");
        return 0;
    }
    return _rtiObjectInstance->getAttributeDataType(index);
}

void
HLAObjectInstance::setAttributeDataElement(unsigned index, SGSharedPtr<HLADataElement> dataElement)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to set data element for inactive object!");
        return;
    }
    _rtiObjectInstance->setDataElement(index, dataElement);
}

HLADataElement*
HLAObjectInstance::getAttributeDataElement(unsigned index)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to set data element for inactive object!");
        return 0;
    }
    return _rtiObjectInstance->getDataElement(index);
}

const HLADataElement*
HLAObjectInstance::getAttributeDataElement(unsigned index) const
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to set data element for inactive object!");
        return 0;
    }
    return _rtiObjectInstance->getDataElement(index);
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

    class VariantDataElementFactory : public HLAVariantDataElement::DataElementFactory {
    public:
        VariantDataElementFactory(const HLADataElement::Path& path, const HLAPathElementMap& pathElementMap) :
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
        virtual HLADataElement* createElement(const HLAVariantDataElement& element, unsigned index)
        {
            const HLAVariantDataType* dataType = element.getDataType();
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

    virtual void apply(const HLAVariantDataType& dataType)
    {
        _dataElement = createDataElement(_path, dataType);
        if (_dataElement.valid())
            return;

        SGSharedPtr<HLAVariantDataElement> variantDataElement;
        variantDataElement = new HLAVariantDataElement(&dataType);
        variantDataElement->setDataElementFactory(new VariantDataElementFactory(_path, _pathElementMap));

        _dataElement = variantDataElement;
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
        SG_LOG(SG_IO, SG_ALERT, "Cannot get attribute data type for setting attribute at index "
               << index << "!");
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
    SGSharedPtr<HLAObjectClass> objectClass = _objectClass.lock();
    if (!objectClass.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Could not register object with unknown object class!");
        return;
    }
    // This error must have been flagged before
    if (!objectClass->_rtiObjectClass.valid())
        return;
    _rtiObjectInstance = objectClass->_rtiObjectClass->registerObjectInstance(this);
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Could not register object at the RTI!");
        return;
    }
    _name = _rtiObjectInstance->getName();
    objectClass->registerInstance(*this);
}

void
HLAObjectInstance::deleteInstance(const RTIData& tag)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_ALERT, "Trying to delete inactive object!");
        return;
    }
    SGSharedPtr<HLAObjectClass> objectClass = _objectClass.lock();
    if (!objectClass.valid())
        return;
    objectClass->deleteInstance(*this);
    _rtiObjectInstance->deleteObjectInstance(tag);
}

void
HLAObjectInstance::updateAttributeValues(const RTIData& tag)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_INFO, "Not updating inactive object!");
        return;
    }
    if (_attributeCallback.valid())
        _attributeCallback->updateAttributeValues(*this, tag);
    _rtiObjectInstance->updateAttributeValues(tag);
}

void
HLAObjectInstance::updateAttributeValues(const SGTimeStamp& timeStamp, const RTIData& tag)
{
    if (!_rtiObjectInstance.valid()) {
        SG_LOG(SG_IO, SG_INFO, "Not updating inactive object!");
        return;
    }
    if (_attributeCallback.valid())
        _attributeCallback->updateAttributeValues(*this, tag);
    _rtiObjectInstance->updateAttributeValues(timeStamp, tag);
}

void
HLAObjectInstance::removeInstance(const RTIData& tag)
{
    SGSharedPtr<HLAObjectClass> objectClass = _objectClass.lock();
    if (!objectClass.valid())
        return;
    objectClass->removeInstanceCallback(*this, tag);
}

void
HLAObjectInstance::reflectAttributeValues(const RTIIndexDataPairList& dataPairList, const RTIData& tag)
{
    if (!_attributeCallback.valid())
        return;
    _attributeCallback->reflectAttributeValues(*this, dataPairList, tag);
}

void
HLAObjectInstance::reflectAttributeValues(const RTIIndexDataPairList& dataPairList,
                                          const SGTimeStamp& timeStamp, const RTIData& tag)
{
    if (!_attributeCallback.valid())
        return;
    _attributeCallback->reflectAttributeValues(*this, dataPairList, timeStamp, tag);
}

} // namespace simgear
