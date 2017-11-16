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

#include <algorithm>

#include <simgear/compiler.h>

#include "HLAArrayDataType.hxx"

#include "HLAArrayDataElement.hxx"

namespace simgear {

HLAArrayDataType::HLAArrayDataType(const std::string& name) :
    HLADataType(name),
    _isOpaque(false),
    _isString(false)
{
}

HLAArrayDataType::~HLAArrayDataType()
{
}

void
HLAArrayDataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLAArrayDataType*
HLAArrayDataType::toArrayDataType() const
{
    return this;
}

void
HLAArrayDataType::releaseDataTypeReferences()
{
    _elementDataType = 0;
    HLADataType::releaseDataTypeReferences();
}

void
HLAArrayDataType::setElementDataType(const HLADataType* elementDataType)
{
    _elementDataType = elementDataType;
}

void
HLAArrayDataType::setIsOpaque(bool isOpaque)
{
    _isOpaque = isOpaque;
}

void
HLAArrayDataType::setIsString(bool isString)
{
    _isString = isString;
}

void
HLAArrayDataType::_recomputeAlignmentImplementation()
{
    unsigned alignment = 1;
    if (const HLADataType* dataType = getElementDataType())
        alignment = std::max(alignment, dataType->getAlignment());
    setAlignment(alignment);
}

///////////////////////////////////////////////////////////////////////////////////

HLAFixedArrayDataType::HLAFixedArrayDataType(const std::string& name) :
    HLAArrayDataType(name)
{
}

HLAFixedArrayDataType::~HLAFixedArrayDataType()
{
}

void
HLAFixedArrayDataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

bool
HLAFixedArrayDataType::decode(HLADecodeStream& stream, HLAAbstractArrayDataElement& value) const
{
    stream.alignOffsetForSize(getAlignment());
    unsigned numElements = getNumElements();
    if (!value.setNumElements(numElements))
        return false;
    for (unsigned i = 0; i < numElements; ++i)
        if (!value.decodeElement(stream, i))
            return false;
    return true;
}

bool
HLAFixedArrayDataType::encode(HLAEncodeStream& stream, const HLAAbstractArrayDataElement& value) const
{
    stream.alignOffsetForSize(getAlignment());
    unsigned numElementsType = getNumElements();
    unsigned numElementsValue = value.getNumElements();
    unsigned numElements = SGMisc<unsigned>::min(numElementsType, numElementsValue);
    unsigned i = 0;
    for (; i < numElements; ++i)
        if (!value.encodeElement(stream, i))
            return false;
    for (; i < numElementsType; ++i) {
        HLADataTypeEncodeVisitor visitor(stream);
        getElementDataType()->accept(visitor);
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////

HLAVariableArrayDataType::HLAVariableArrayDataType(const std::string& name) :
    HLAArrayDataType(name)
{
    setSizeDataType(new HLAUInt32BEDataType);
}

HLAVariableArrayDataType::~HLAVariableArrayDataType()
{
}

void
HLAVariableArrayDataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

bool
HLAVariableArrayDataType::decode(HLADecodeStream& stream, HLAAbstractArrayDataElement& value) const
{
    stream.alignOffsetForSize(getAlignment());
    HLATemplateDecodeVisitor<unsigned> numElementsVisitor(stream);
    getSizeDataType()->accept(numElementsVisitor);
    unsigned numElements = numElementsVisitor.getValue();
    if (!value.setNumElements(numElements))
        return false;
    for (unsigned i = 0; i < numElements; ++i)
        if (!value.decodeElement(stream, i))
            return false;
    return true;
}

bool
HLAVariableArrayDataType::encode(HLAEncodeStream& stream, const HLAAbstractArrayDataElement& value) const
{
    stream.alignOffsetForSize(getAlignment());
    unsigned numElements = value.getNumElements();
    HLATemplateEncodeVisitor<unsigned> numElementsVisitor(stream, numElements);
    getSizeDataType()->accept(numElementsVisitor);
    for (unsigned i = 0; i < numElements; ++i)
        if (!value.encodeElement(stream, i))
            return false;
    return true;
}

void
HLAVariableArrayDataType::setSizeDataType(const HLABasicDataType* sizeDataType)
{
    _sizeDataType = sizeDataType;
}

void
HLAVariableArrayDataType::_recomputeAlignmentImplementation()
{
    unsigned alignment = 1;
    if (const HLADataType* dataType = getElementDataType())
        alignment = std::max(alignment, dataType->getAlignment());
    if (const HLADataType* dataType = getSizeDataType())
        alignment = std::max(alignment, dataType->getAlignment());
    setAlignment(alignment);
}

} // namespace simgear
