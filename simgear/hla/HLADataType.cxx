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

#include "HLADataType.hxx"

#include "HLADataElement.hxx"
#include "HLADataTypeVisitor.hxx"

namespace simgear {

HLADataType::HLADataType(const std::string& name, unsigned alignment) :
    _name(name),
    _alignment(1)
{
    setAlignment(alignment);
}

HLADataType::~HLADataType()
{
}

void
HLADataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLADataTypeReference*
HLADataType::toDataTypeReference() const
{
    return 0;
}

const HLABasicDataType*
HLADataType::toBasicDataType() const
{
    return 0;
}

const HLAArrayDataType*
HLADataType::toArrayDataType() const
{
    return 0;
}

const HLAEnumeratedDataType*
HLADataType::toEnumeratedDataType() const
{
    return 0;
}

const HLAFixedRecordDataType*
HLADataType::toFixedRecordDataType() const
{
    return 0;
}

const HLAVariantDataType*
HLADataType::toVariantDataType() const
{
    return 0;
}

void
HLADataType::setAlignment(unsigned alignment)
{
    /// FIXME: more checks
    if (alignment == 0)
        _alignment = 1;
    else
        _alignment = alignment;
}

HLADataTypeReference::~HLADataTypeReference()
{
}

void
HLADataTypeReference::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLADataTypeReference*
HLADataTypeReference::toDataTypeReference() const
{
    return this;
}

}
