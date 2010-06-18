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

#include "HLAFixedRecordDataType.hxx"

#include "HLADataTypeVisitor.hxx"
#include "HLAFixedRecordDataElement.hxx"

namespace simgear {

HLAFixedRecordDataType::HLAFixedRecordDataType(const std::string& name) :
    HLADataType(name)
{
}

HLAFixedRecordDataType::~HLAFixedRecordDataType()
{
}

void
HLAFixedRecordDataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLAFixedRecordDataType*
HLAFixedRecordDataType::toFixedRecordDataType() const
{
    return this;
}

bool
HLAFixedRecordDataType::decode(HLADecodeStream& stream, HLAAbstractFixedRecordDataElement& value) const
{
    stream.alignOffsetForSize(getAlignment());
    unsigned numFields = getNumFields();
    for (unsigned i = 0; i < numFields; ++i)
        if (!value.decodeField(stream, i))
            return false;
    return true;
}

bool
HLAFixedRecordDataType::encode(HLAEncodeStream& stream, const HLAAbstractFixedRecordDataElement& value) const
{
    stream.alignOffsetForSize(getAlignment());
    unsigned numFields = getNumFields();
    for (unsigned i = 0; i < numFields; ++i)
        if (!value.encodeField(stream, i))
            return false;
    return true;
}

void
HLAFixedRecordDataType::addField(const std::string& name, const HLADataType* dataType)
{
    // FIXME this only works if we do not reset the alignment to something smaller
    if (getAlignment() < dataType->getAlignment())
        setAlignment(dataType->getAlignment());
    _fieldList.push_back(Field(name, dataType));
}

} // namespace simgear
