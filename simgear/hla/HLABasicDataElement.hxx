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

#ifndef HLABasicDataElement_hxx
#define HLABasicDataElement_hxx

#include "HLABasicDataType.hxx"
#include "HLADataElement.hxx"

namespace simgear {

class HLABasicDataElement : public HLADataElement {
public:
    HLABasicDataElement(const HLABasicDataType* dataType);
    virtual ~HLABasicDataElement();

    virtual void accept(HLADataElementVisitor& visitor);
    virtual void accept(HLAConstDataElementVisitor& visitor) const;

    virtual bool encode(HLAEncodeStream& stream) const = 0;
    virtual bool decode(HLADecodeStream& stream) = 0;

    virtual const HLABasicDataType* getDataType() const;
    virtual bool setDataType(const HLADataType* dataType);
    void setDataType(const HLABasicDataType* dataType);

protected:
    SGSharedPtr<const HLABasicDataType> _dataType;
};

#define TYPED_HLA_BASIC_DATA_ELEMENT(type, ctype)                             \
class HLAAbstract##type##DataElement : public HLABasicDataElement {           \
public:                                                                       \
    HLAAbstract##type##DataElement(const HLABasicDataType* dataType = 0);     \
    virtual ~HLAAbstract##type##DataElement();                                \
                                                                              \
    virtual bool encode(HLAEncodeStream& stream) const;                       \
    virtual bool decode(HLADecodeStream& stream);                             \
                                                                              \
    virtual ctype getValue() const = 0;                                       \
    virtual void setValue(ctype) = 0;                                         \
};                                                                            \
class HLA##type##DataElement : public HLAAbstract##type##DataElement {        \
public:                                                                       \
    HLA##type##DataElement(const HLABasicDataType* dataType = 0);             \
    HLA##type##DataElement(const HLABasicDataType* dataType,                  \
                           const ctype& value);                               \
    virtual ~HLA##type##DataElement();                                        \
    virtual ctype getValue() const;                                           \
    virtual void setValue(ctype value);                                       \
private:                                                                      \
    ctype _value;                                                             \
};                                                                            \
class HLA##type##Data {                                                       \
public:                                                                       \
    HLA##type##Data() :                                                       \
        _value(new HLA##type##DataElement(0))                                 \
    { }                                                                       \
    HLA##type##Data(const ctype& value) :                                     \
        _value(new HLA##type##DataElement(0, value))                          \
    { }                                                                       \
    operator ctype() const                                                    \
    { return _value->getValue(); }                                            \
    HLA##type##Data& operator=(const ctype& value)                            \
    { _value->setValue(value); return *this; }                                \
    ctype getValue() const                                                    \
    { return _value->getValue(); }                                            \
    void setValue(const ctype& value)                                         \
    { _value->setValue(value); }                                              \
    const HLA##type##DataElement* getDataElement() const                      \
    { return _value.get(); }                                                  \
    HLA##type##DataElement* getDataElement()                                  \
    { return _value.get(); }                                                  \
    const HLABasicDataType* getDataType() const                               \
    { return _value->getDataType(); }                                         \
    void setDataType(const HLABasicDataType* dataType)                        \
    { _value->setDataType(dataType); }                                        \
                                                                              \
private:                                                                      \
    SGSharedPtr<HLA##type##DataElement> _value;                               \
};


TYPED_HLA_BASIC_DATA_ELEMENT(Bool, bool);
TYPED_HLA_BASIC_DATA_ELEMENT(Char, char);
TYPED_HLA_BASIC_DATA_ELEMENT(WChar, wchar_t);
TYPED_HLA_BASIC_DATA_ELEMENT(SChar, signed char);
TYPED_HLA_BASIC_DATA_ELEMENT(UChar, unsigned char);
TYPED_HLA_BASIC_DATA_ELEMENT(Short, short);
TYPED_HLA_BASIC_DATA_ELEMENT(UShort, unsigned short);
TYPED_HLA_BASIC_DATA_ELEMENT(Int, int);
TYPED_HLA_BASIC_DATA_ELEMENT(UInt, unsigned int);
TYPED_HLA_BASIC_DATA_ELEMENT(Long, long);
TYPED_HLA_BASIC_DATA_ELEMENT(ULong, unsigned long);
TYPED_HLA_BASIC_DATA_ELEMENT(Float, float);
TYPED_HLA_BASIC_DATA_ELEMENT(Double, double);

#undef TYPED_HLA_BASIC_DATA_ELEMENT

}

#endif
