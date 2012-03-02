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

#include "HLAVariantRecordDataElement.hxx"

#include <simgear/debug/logstream.hxx>

#include "HLADataElementVisitor.hxx"

namespace simgear {

HLAAbstractVariantRecordDataElement::HLAAbstractVariantRecordDataElement(const HLAVariantRecordDataType* dataType) :
    _dataType(dataType)
{
}

HLAAbstractVariantRecordDataElement::~HLAAbstractVariantRecordDataElement()
{
}

void
HLAAbstractVariantRecordDataElement::accept(HLADataElementVisitor& visitor)
{
    visitor.apply(*this);
}

void
HLAAbstractVariantRecordDataElement::accept(HLAConstDataElementVisitor& visitor) const
{
    visitor.apply(*this);
}

bool
HLAAbstractVariantRecordDataElement::decode(HLADecodeStream& stream)
{
    if (!_dataType.valid())
        return false;
    return _dataType->decode(stream, *this);
}

bool
HLAAbstractVariantRecordDataElement::encode(HLAEncodeStream& stream) const
{
    if (!_dataType.valid())
        return false;
    return _dataType->encode(stream, *this);
}

const HLAVariantRecordDataType*
HLAAbstractVariantRecordDataElement::getDataType() const
{
    return _dataType.get();
}

bool
HLAAbstractVariantRecordDataElement::setDataType(const HLADataType* dataType)
{
    const HLAVariantRecordDataType* variantRecordDataType = dataType->toVariantRecordDataType();
    if (!variantRecordDataType) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAVariantRecordDataType: unable to set data type!");
        return false;
    }
    setDataType(variantRecordDataType);
    return true;
}

void
HLAAbstractVariantRecordDataElement::setDataType(const HLAVariantRecordDataType* dataType)
{
    _dataType = dataType;
}

std::string
HLAAbstractVariantRecordDataElement::getAlternativeName() const
{
    if (!_dataType.valid())
        return std::string();
    return _dataType->getAlternativeName(getAlternativeIndex());
}

const HLADataType*
HLAAbstractVariantRecordDataElement::getAlternativeDataType() const
{
    if (!_dataType.valid())
        return 0;
    return _dataType->getAlternativeDataType(getAlternativeIndex());
}


HLAVariantRecordDataElement::DataElementFactory::~DataElementFactory()
{
}

HLAVariantRecordDataElement::HLAVariantRecordDataElement(const HLAVariantRecordDataType* dataType) :
    HLAAbstractVariantRecordDataElement(dataType),
    _alternativeIndex(~0u)
{
}

HLAVariantRecordDataElement::~HLAVariantRecordDataElement()
{
    clearStamp();
}

bool
HLAVariantRecordDataElement::setAlternativeIndex(unsigned index)
{
    if (_alternativeIndex == index)
        return true;
    SGSharedPtr<HLADataElement> dataElement = newElement(index);
    if (!dataElement.valid())
        return false;
    _dataElement.swap(dataElement);
    _alternativeIndex = index;
    setDirty(true);
    return true;
}

bool
HLAVariantRecordDataElement::decodeAlternative(HLADecodeStream& stream)
{
    return _dataElement->decode(stream);
}

unsigned
HLAVariantRecordDataElement::getAlternativeIndex() const
{
    return _alternativeIndex;
}

bool
HLAVariantRecordDataElement::encodeAlternative(HLAEncodeStream& stream) const
{
    return _dataElement->encode(stream);
}

void
HLAVariantRecordDataElement::setDataElementFactory(HLAVariantRecordDataElement::DataElementFactory* dataElementFactory)
{
    _dataElementFactory = dataElementFactory;
}

HLAVariantRecordDataElement::DataElementFactory*
HLAVariantRecordDataElement::getDataElementFactory()
{
    return _dataElementFactory;
}

void
HLAVariantRecordDataElement::_setStamp(Stamp* stamp)
{
    HLAAbstractVariantRecordDataElement::_setStamp(stamp);
    if (!_dataElement.valid())
        return;
    _dataElement->attachStamp(*this);
}

HLADataElement*
HLAVariantRecordDataElement::newElement(unsigned index)
{
    if (!_dataElementFactory.valid())
        return 0;
    HLADataElement* dataElement = _dataElementFactory->createElement(*this, index);
    if (!dataElement)
        return 0;
    dataElement->attachStamp(*this);
    return dataElement;
}

}
