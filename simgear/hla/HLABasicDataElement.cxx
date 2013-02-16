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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include "HLABasicDataElement.hxx"

#include "HLADataElementVisitor.hxx"
#include "HLADataTypeVisitor.hxx"

namespace simgear {

HLABasicDataElement::HLABasicDataElement(const HLABasicDataType* dataType) :
    _dataType(dataType)
{
}

HLABasicDataElement::~HLABasicDataElement()
{
}

void
HLABasicDataElement::accept(HLADataElementVisitor& visitor)
{
    visitor.apply(*this);
}

void
HLABasicDataElement::accept(HLAConstDataElementVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLABasicDataType*
HLABasicDataElement::getDataType() const
{
    return _dataType.get();
}

bool
HLABasicDataElement::setDataType(const HLADataType* dataType)
{
    const HLABasicDataType* scalarDataType = dynamic_cast<const HLABasicDataType*>(dataType);
    if (!scalarDataType) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLABasicDataType: unable to set data type!");
        return false;
    }
    setDataType(scalarDataType);
    return true;
}

void
HLABasicDataElement::setDataType(const HLABasicDataType* dataType)
{
    _dataType = dataType;
}

#define IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(type, ctype)                                       \
HLAAbstract##type##DataElement::HLAAbstract##type##DataElement(const HLABasicDataType* dataType) :\
    HLABasicDataElement(dataType)                                                                 \
{                                                                                                 \
}                                                                                                 \
                                                                                                  \
HLAAbstract##type##DataElement::~HLAAbstract##type##DataElement()                                 \
{                                                                                                 \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
HLAAbstract##type##DataElement::encode(HLAEncodeStream& stream) const                             \
{                                                                                                 \
    if (!_dataType.valid())                                                                       \
        return false;                                                                             \
    HLATemplateEncodeVisitor<ctype> visitor(stream, getValue());                                  \
    _dataType->accept(visitor);                                                                   \
    return true;                                                                                  \
}                                                                                                 \
                                                                                                  \
bool                                                                                              \
HLAAbstract##type##DataElement::decode(HLADecodeStream& stream)                                   \
{                                                                                                 \
    if (!_dataType.valid())                                                                       \
        return false;                                                                             \
    HLATemplateDecodeVisitor<ctype> visitor(stream);                                              \
    _dataType->accept(visitor);                                                                   \
    setValue(visitor.getValue());                                                                 \
    return true;                                                                                  \
}                                                                                                 \
                                                                                                  \
HLA##type##DataElement::HLA##type##DataElement(const HLABasicDataType* dataType) :                \
    HLAAbstract##type##DataElement(dataType),                                                     \
    _value(0)                                                                                     \
{                                                                                                 \
}                                                                                                 \
                                                                                                  \
HLA##type##DataElement::HLA##type##DataElement(const HLABasicDataType* dataType,                  \
                                               const ctype& value) :                              \
    HLAAbstract##type##DataElement(dataType),                                                     \
    _value(value)                                                                                 \
{                                                                                                 \
}                                                                                                 \
                                                                                                  \
HLA##type##DataElement::~HLA##type##DataElement()                                                 \
{                                                                                                 \
}                                                                                                 \
                                                                                                  \
ctype                                                                                             \
HLA##type##DataElement::getValue() const                                                          \
{                                                                                                 \
    return _value;                                                                                \
}                                                                                                 \
                                                                                                  \
void                                                                                              \
HLA##type##DataElement::setValue(ctype value)                                                     \
{                                                                                                 \
    _value = value;                                                                               \
    setDirty(true);                                                                               \
}

IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(Bool, bool);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(Char, char);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(WChar, wchar_t);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(SChar, signed char);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(UChar, unsigned char);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(Short, short);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(UShort, unsigned short);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(Int, int);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(UInt, unsigned int);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(Long, long);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(ULong, unsigned long);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(Float, float);
IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT(Double, double);

#undef IMPLEMENT_TYPED_HLA_BASIC_DATA_ELEMENT

}
