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

#include "HLABasicDataType.hxx"

#include "HLADataType.hxx"
#include "HLADataTypeVisitor.hxx"

namespace simgear {

HLABasicDataType::HLABasicDataType(const std::string& name) :
    HLADataType(name)
{
}

HLABasicDataType::~HLABasicDataType()
{
}

void
HLABasicDataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLABasicDataType*
HLABasicDataType::toBasicDataType() const
{
    return this;
}

//////////////////////////////////////////////////////////////////////////////

HLAInt8DataType::HLAInt8DataType(const std::string& name) :
    HLABasicDataType(name)
{
    setAlignment(sizeof(int8_t));
}

HLAInt8DataType::~HLAInt8DataType()
{
}

void
HLAInt8DataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

//////////////////////////////////////////////////////////////////////////////

HLAUInt8DataType::HLAUInt8DataType(const std::string& name) :
    HLABasicDataType(name)
{
    setAlignment(sizeof(uint8_t));
}

HLAUInt8DataType::~HLAUInt8DataType()
{
}

void
HLAUInt8DataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

//////////////////////////////////////////////////////////////////////////////

#define BASIC_DATATYPE_IMPLEMENTATION(type, name)                       \
HLA##name##DataType::HLA##name##DataType(const std::string& name) :     \
    HLABasicDataType(name)                                              \
{                                                                       \
    setAlignment(sizeof(type));                                         \
}                                                                       \
                                                                        \
HLA##name##DataType::~HLA##name##DataType()                             \
{                                                                       \
}                                                                       \
                                                                        \
void                                                                    \
HLA##name##DataType::accept(HLADataTypeVisitor& visitor) const          \
{                                                                       \
    visitor.apply(*this);                                               \
}                                                                       \
                                                                        \
                                                                        \
                                                                        \
HLA##name##LEDataType::HLA##name##LEDataType(const std::string& name) : \
    HLA##name##DataType(name)                                           \
{                                                                       \
}                                                                       \
                                                                        \
HLA##name##LEDataType::~HLA##name##LEDataType()                         \
{                                                                       \
}                                                                       \
                                                                        \
bool                                                                    \
HLA##name##LEDataType::decode(HLADecodeStream& stream,                  \
                              type& value) const                        \
{                                                                       \
    if (!stream.alignOffsetForSize(getAlignment()))                     \
        return false;                                                   \
    return stream.decode##name##LE(value);                              \
}                                                                       \
                                                                        \
bool                                                                    \
HLA##name##LEDataType::encode(HLAEncodeStream& stream,                  \
                              const type& value) const                  \
{                                                                       \
    if (!stream.alignOffsetForSize(getAlignment()))                     \
        return false;                                                   \
    return stream.encode##name##LE(value);                              \
}                                                                       \
                                                                        \
                                                                        \
                                                                        \
HLA##name##BEDataType::HLA##name##BEDataType(const std::string& name) : \
    HLA##name##DataType(name)                                           \
{                                                                       \
}                                                                       \
                                                                        \
HLA##name##BEDataType::~HLA##name##BEDataType()                         \
{                                                                       \
}                                                                       \
                                                                        \
bool                                                                    \
HLA##name##BEDataType::decode(HLADecodeStream& stream,                  \
                              type& value) const                        \
{                                                                       \
    if (!stream.alignOffsetForSize(getAlignment()))                     \
        return false;                                                   \
    return stream.decode##name##BE(value);                              \
}                                                                       \
                                                                        \
bool                                                                    \
HLA##name##BEDataType::encode(HLAEncodeStream& stream,                  \
                              const type& value) const                  \
{                                                                       \
    if (!stream.alignOffsetForSize(getAlignment()))                     \
        return false;                                                   \
    return stream.encode##name##BE(value);                              \
}

BASIC_DATATYPE_IMPLEMENTATION(int16_t, Int16)
BASIC_DATATYPE_IMPLEMENTATION(uint16_t, UInt16)
BASIC_DATATYPE_IMPLEMENTATION(int32_t, Int32)
BASIC_DATATYPE_IMPLEMENTATION(uint32_t, UInt32)
BASIC_DATATYPE_IMPLEMENTATION(int64_t, Int64)
BASIC_DATATYPE_IMPLEMENTATION(uint64_t, UInt64)
BASIC_DATATYPE_IMPLEMENTATION(float, Float32)
BASIC_DATATYPE_IMPLEMENTATION(double, Float64)

#undef BASIC_DATATYPE_IMPLEMENTATION

} // namespace simgear
