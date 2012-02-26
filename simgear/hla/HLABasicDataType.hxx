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

#ifndef HLABasicDataType_hxx
#define HLABasicDataType_hxx

#include <string>
#include "HLADataType.hxx"

namespace simgear {

class HLABasicDataType : public HLADataType {
public:
    virtual ~HLABasicDataType();
    virtual void accept(HLADataTypeVisitor& visitor) const;

    virtual const HLABasicDataType* toBasicDataType() const;

protected:
    HLABasicDataType(const std::string& name);
};

class HLAInt8DataType : public HLABasicDataType {
public:
    HLAInt8DataType(const std::string& name = "HLAInt8DataType");
    virtual ~HLAInt8DataType();

    virtual void accept(HLADataTypeVisitor& visitor) const;

    bool skip(HLADecodeStream& stream) const
    {
        if (!stream.alignOffsetForSize(getAlignment()))
            return false;
        return stream.skip(1);
    }
    bool skip(HLAEncodeStream& stream) const
    {
        if (!stream.alignOffsetForSize(getAlignment()))
            return false;
        return stream.skip(1);
    }
    bool decode(HLADecodeStream& stream, int8_t& value) const
    {
        if (!stream.alignOffsetForSize(getAlignment()))
            return false;
        return stream.decodeInt8(value);
    }
    bool encode(HLAEncodeStream& stream, const int8_t& value) const
    {
        if (!stream.alignOffsetForSize(getAlignment()))
            return false;
        return stream.encodeInt8(value);
    }
};

class HLAUInt8DataType : public HLABasicDataType {
public:
    HLAUInt8DataType(const std::string& name = "HLAUInt8DataType");
    virtual ~HLAUInt8DataType();

    virtual void accept(HLADataTypeVisitor& visitor) const;

    bool skip(HLADecodeStream& stream) const
    {
        if (!stream.alignOffsetForSize(getAlignment()))
            return false;
        return stream.skip(1);
    }
    bool skip(HLAEncodeStream& stream) const
    {
        if (!stream.alignOffsetForSize(getAlignment()))
            return false;
        return stream.skip(1);
    }
    bool decode(HLADecodeStream& stream, uint8_t& value) const
    {
        if (!stream.alignOffsetForSize(getAlignment()))
            return false;
        return stream.decodeUInt8(value);
    }
    bool encode(HLAEncodeStream& stream, const uint8_t& value) const
    {
        if (!stream.alignOffsetForSize(getAlignment()))
            return false;
        return stream.encodeUInt8(value);
    }
};

#define BASIC_DATATYPE_IMPLEMENTATION(type, name)                          \
class HLA##name##DataType : public HLABasicDataType {                      \
public:                                                                    \
 virtual ~HLA##name##DataType();                                           \
                                                                           \
 virtual void accept(HLADataTypeVisitor& visitor) const;                   \
                                                                           \
 bool skip(HLADecodeStream& stream) const                                  \
 {                                                                         \
   if (!stream.alignOffsetForSize(getAlignment()))                         \
     return false;                                                         \
   return stream.skip(sizeof(type));                                       \
 }                                                                         \
 bool skip(HLAEncodeStream& stream) const                                  \
 {                                                                         \
   if (!stream.alignOffsetForSize(getAlignment()))                         \
     return false;                                                         \
   return stream.skip(sizeof(type));                                       \
 }                                                                         \
 virtual bool decode(HLADecodeStream& stream, type& value) const = 0;      \
 virtual bool encode(HLAEncodeStream& stream, const type& value) const = 0;\
protected:                                                                 \
 HLA##name##DataType(const std::string& name);                             \
};                                                                         \
class HLA##name##LEDataType : public HLA##name##DataType {                 \
public:                                                                    \
 HLA##name##LEDataType(const std::string& name = "HLA" #name "LEDataType");\
 virtual ~HLA##name##LEDataType();                                         \
 virtual bool decode(HLADecodeStream& stream, type& value) const;          \
 virtual bool encode(HLAEncodeStream& stream, const type& value) const;    \
};                                                                         \
class HLA##name##BEDataType : public HLA##name##DataType {                 \
public:                                                                    \
 HLA##name##BEDataType(const std::string& name = "HLA" #name "BEDataType");\
 virtual ~HLA##name##BEDataType();                                         \
 virtual bool decode(HLADecodeStream& stream, type& value) const;          \
 virtual bool encode(HLAEncodeStream& stream, const type& value) const;    \
};

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

#endif
