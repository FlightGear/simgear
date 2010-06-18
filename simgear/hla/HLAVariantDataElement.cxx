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

#include "HLAVariantDataElement.hxx"

#include <simgear/debug/logstream.hxx>

namespace simgear {

HLAAbstractVariantDataElement::HLAAbstractVariantDataElement(const HLAVariantDataType* dataType) :
    _dataType(dataType)
{
}

HLAAbstractVariantDataElement::~HLAAbstractVariantDataElement()
{
}

bool
HLAAbstractVariantDataElement::decode(HLADecodeStream& stream)
{
    if (!_dataType.valid())
        return false;
    return _dataType->decode(stream, *this);
}

bool
HLAAbstractVariantDataElement::encode(HLAEncodeStream& stream) const
{
    if (!_dataType.valid())
        return false;
    return _dataType->encode(stream, *this);
}

const HLAVariantDataType*
HLAAbstractVariantDataElement::getDataType() const
{
    return _dataType.get();
}

bool
HLAAbstractVariantDataElement::setDataType(const HLADataType* dataType)
{
    const HLAVariantDataType* variantDataType = dataType->toVariantDataType();
    if (!variantDataType) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAVariantDataType: unable to set data type!");
        return false;
    }
    setDataType(variantDataType);
    return true;
}

void
HLAAbstractVariantDataElement::setDataType(const HLAVariantDataType* dataType)
{
    _dataType = dataType;
}

std::string
HLAAbstractVariantDataElement::getAlternativeName() const
{
    if (!_dataType.valid())
        return std::string();
    return _dataType->getAlternativeName(getAlternativeIndex());
}

const HLADataType*
HLAAbstractVariantDataElement::getAlternativeDataType() const
{
    if (!_dataType.valid())
        return 0;
    return _dataType->getAlternativeDataType(getAlternativeIndex());
}


HLAVariantDataElement::DataElementFactory::~DataElementFactory()
{
}

HLAVariantDataElement::HLAVariantDataElement(const HLAVariantDataType* dataType) :
    HLAAbstractVariantDataElement(dataType),
    _alternativeIndex(~0u)
{
}

HLAVariantDataElement::~HLAVariantDataElement()
{
}

bool
HLAVariantDataElement::setAlternativeIndex(unsigned index)
{
    if (_alternativeIndex == index)
        return true;
    SGSharedPtr<HLADataElement> dataElement = newElement(index);
    if (!dataElement.valid())
        return false;
    _dataElement.swap(dataElement);
    _alternativeIndex = index;
    return true;
}

bool
HLAVariantDataElement::decodeAlternative(HLADecodeStream& stream)
{
    return _dataElement->decode(stream);
}

unsigned
HLAVariantDataElement::getAlternativeIndex() const
{
    return _alternativeIndex;
}

bool
HLAVariantDataElement::encodeAlternative(HLAEncodeStream& stream) const
{
    return _dataElement->encode(stream);
}

void
HLAVariantDataElement::setDataElementFactory(HLAVariantDataElement::DataElementFactory* dataElementFactory)
{
    _dataElementFactory = dataElementFactory;
}

HLAVariantDataElement::DataElementFactory*
HLAVariantDataElement::getDataElementFactory()
{
    return _dataElementFactory;
}

HLADataElement*
HLAVariantDataElement::newElement(unsigned index)
{
    if (!_dataElementFactory.valid())
        return 0;
    return _dataElementFactory->createElement(*this, index);
}

}
