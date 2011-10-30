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

#include "HLAArrayDataElement.hxx"

#include <simgear/debug/logstream.hxx>

#include "HLADataElementVisitor.hxx"

namespace simgear {

HLAAbstractArrayDataElement::HLAAbstractArrayDataElement(const HLAArrayDataType* dataType) :
    _dataType(dataType)
{
}

HLAAbstractArrayDataElement::~HLAAbstractArrayDataElement()
{
}

void
HLAAbstractArrayDataElement::accept(HLADataElementVisitor& visitor)
{
    visitor.apply(*this);
}

void
HLAAbstractArrayDataElement::accept(HLAConstDataElementVisitor& visitor) const
{
    visitor.apply(*this);
}

bool
HLAAbstractArrayDataElement::decode(HLADecodeStream& stream)
{
    if (!_dataType.valid())
        return false;
    return _dataType->decode(stream, *this);
}

bool
HLAAbstractArrayDataElement::encode(HLAEncodeStream& stream) const
{
    if (!_dataType.valid())
        return false;
    return _dataType->encode(stream, *this);
}

const HLAArrayDataType*
HLAAbstractArrayDataElement::getDataType() const
{
    return _dataType.get();
}

bool
HLAAbstractArrayDataElement::setDataType(const HLADataType* dataType)
{
    const HLAArrayDataType* arrayDataType = dynamic_cast<const HLAArrayDataType*>(dataType);
    if (!arrayDataType) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAArrayDataType: unable to set data type!");
        return false;
    }
    _dataType = arrayDataType;
    return true;
}

const HLADataType*
HLAAbstractArrayDataElement::getElementDataType() const
{
    if (!_dataType.valid())
        return 0;
    return _dataType->getElementDataType();
}

////////////////////////////////////////////////////////////////////////

HLAArrayDataElement::DataElementFactory::~DataElementFactory()
{
}

HLAArrayDataElement::HLAArrayDataElement(const HLAArrayDataType* dataType) :
    HLAAbstractArrayDataElement(dataType)
{
}

HLAArrayDataElement::~HLAArrayDataElement()
{
}

bool
HLAArrayDataElement::setNumElements(unsigned size)
{
    unsigned oldSize = _elementVector.size();
    if (size == oldSize)
        return true;
    _elementVector.resize(size);
    for (unsigned i = oldSize; i < size; ++i)
        _elementVector[i] = newElement(i);
    return true;
}

bool
HLAArrayDataElement::decodeElement(HLADecodeStream& stream, unsigned i)
{
    HLADataElement* dataElement = getElement(i);
    if (!dataElement)
        return false;
    return dataElement->decode(stream);
}

unsigned
HLAArrayDataElement::getNumElements() const
{
    return _elementVector.size();
}

bool
HLAArrayDataElement::encodeElement(HLAEncodeStream& stream, unsigned i) const
{
    const HLADataElement* dataElement = getElement(i);
    if (!dataElement)
        return false;
    return dataElement->encode(stream);
}

const HLADataElement*
HLAArrayDataElement::getElement(unsigned index) const
{
    if (_elementVector.size() <= index)
        return 0;
    return _elementVector[index].get();
}

HLADataElement*
HLAArrayDataElement::getElement(unsigned index)
{
    if (_elementVector.size() <= index)
        return 0;
    return _elementVector[index].get();
}

HLADataElement*
HLAArrayDataElement::getOrCreateElement(unsigned index)
{
    if (_elementVector.size() <= index)
        if (!setNumElements(index + 1))
            return 0;
    return _elementVector[index].get();
}

void
HLAArrayDataElement::setElement(unsigned index, HLADataElement* value)
{
    unsigned oldSize = _elementVector.size();
    if (oldSize <= index) {
        _elementVector.resize(index + 1);
        for (unsigned j = oldSize; j < index; ++j)
            _elementVector[j] = newElement(j);
    }
    _elementVector[index] = value;
}

void
HLAArrayDataElement::setDataElementFactory(HLAArrayDataElement::DataElementFactory* dataElementFactory)
{
    _dataElementFactory = dataElementFactory;
}

HLAArrayDataElement::DataElementFactory*
HLAArrayDataElement::getDataElementFactory()
{
    return _dataElementFactory.get();
}

HLADataElement*
HLAArrayDataElement::newElement(unsigned index)
{
    if (!_dataElementFactory.valid())
        return 0;
    return _dataElementFactory->createElement(*this, index);
}

////////////////////////////////////////////////////////////////////////

HLAVariantArrayDataElement::HLAVariantArrayDataElement() :
    HLAAbstractArrayDataElement(0)
{
}

HLAVariantArrayDataElement::~HLAVariantArrayDataElement()
{
}

bool
HLAVariantArrayDataElement::setDataType(const HLADataType* dataType)
{
    const HLAArrayDataType* arrayDataType = dataType->toArrayDataType();
    if (!arrayDataType) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAVariantArrayDataType: unable to set data type, dataType is not an array data type!");
        return false;
    }
    const HLAVariantDataType* variantDataType = arrayDataType->getElementDataType()->toVariantDataType();
    if (!variantDataType) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAVariantArrayDataType: unable to set data type: arrayDataTypes element data type is no a variant data type!");
        return false;
    }
    _dataType = arrayDataType;
    return true;
}

bool
HLAVariantArrayDataElement::setNumElements(unsigned size)
{
    unsigned oldSize = _elementVector.size();
    if (size == oldSize)
        return true;
    _elementVector.resize(size);
    for (unsigned i = oldSize; i < size; ++i)
        _elementVector[i] = newElement();
    return true;
}

bool
HLAVariantArrayDataElement::decodeElement(HLADecodeStream& stream, unsigned i)
{
    HLAVariantDataElement* dataElement = getElement(i);
    if (!dataElement)
        return false;
    return dataElement->decode(stream);
}

unsigned
HLAVariantArrayDataElement::getNumElements() const
{
    return _elementVector.size();
}

bool
HLAVariantArrayDataElement::encodeElement(HLAEncodeStream& stream, unsigned i) const
{
    const HLADataElement* dataElement = getElement(i);
    if (!dataElement)
        return false;
    return dataElement->encode(stream);
}

const HLAVariantDataElement*
HLAVariantArrayDataElement::getElement(unsigned index) const
{
    if (_elementVector.size() <= index)
        return 0;
    return _elementVector[index].get();
}

HLAVariantDataElement*
HLAVariantArrayDataElement::getElement(unsigned index)
{
    if (_elementVector.size() <= index)
        return 0;
    return _elementVector[index].get();
}

HLAVariantDataElement*
HLAVariantArrayDataElement::getOrCreateElement(unsigned index)
{
    if (_elementVector.size() <= index)
        if (!setNumElements(index + 1))
            return 0;
    return _elementVector[index].get();
}

void
HLAVariantArrayDataElement::setElement(unsigned index, HLAVariantDataElement* value)
{
    unsigned oldSize = _elementVector.size();
    if (oldSize <= index) {
        _elementVector.resize(index + 1);
        for (unsigned j = oldSize; j < index; ++j)
            _elementVector[j] = newElement();
    }
    _elementVector[index] = value;
}

void
HLAVariantArrayDataElement::setAlternativeDataElementFactory(HLAVariantArrayDataElement::AlternativeDataElementFactory* alternativeDataElementFactory)
{
    _alternativeDataElementFactory = alternativeDataElementFactory;
}

HLAVariantArrayDataElement::AlternativeDataElementFactory*
HLAVariantArrayDataElement::getAlternativeDataElementFactory()
{
    return _alternativeDataElementFactory.get();
}

HLAVariantDataElement*
HLAVariantArrayDataElement::newElement()
{
    const HLAArrayDataType* arrayDataType = getDataType();
    if (!arrayDataType)
        return 0;
    const HLADataType* elementDataType = arrayDataType->getElementDataType();
    if (!elementDataType)
        return 0;
    const HLAVariantDataType* variantDataType = elementDataType->toVariantDataType();
    if (!variantDataType)
        return 0;
    HLAVariantDataElement* variantDataElement = new HLAVariantDataElement(variantDataType);
    variantDataElement->setDataElementFactory(_alternativeDataElementFactory.get());
    return variantDataElement;
}

}
