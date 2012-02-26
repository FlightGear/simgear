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

#ifndef HLADataTypeVisitor_hxx
#define HLADataTypeVisitor_hxx

#include <cassert>
#include <string>
#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGMath.hxx>
#include "HLAArrayDataType.hxx"
#include "HLABasicDataType.hxx"
#include "HLADataElement.hxx"
#include "HLAEnumeratedDataType.hxx"
#include "HLAFixedRecordDataType.hxx"
#include "HLAVariantRecordDataType.hxx"

namespace simgear {

class HLADataTypeVisitor {
public:
    virtual ~HLADataTypeVisitor() {}

    virtual void apply(const HLADataType& dataType)
    { }

    virtual void apply(const HLABasicDataType& dataType)
    { apply(static_cast<const HLADataType&>(dataType)); }
    virtual void apply(const HLAInt8DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }
    virtual void apply(const HLAUInt8DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }
    virtual void apply(const HLAInt16DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }
    virtual void apply(const HLAUInt16DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }
    virtual void apply(const HLAInt32DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }
    virtual void apply(const HLAUInt32DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }
    virtual void apply(const HLAInt64DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }
    virtual void apply(const HLAUInt64DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }
    virtual void apply(const HLAFloat32DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }
    virtual void apply(const HLAFloat64DataType& dataType)
    { apply(static_cast<const HLABasicDataType&>(dataType)); }

    virtual void apply(const HLAArrayDataType& dataType)
    { apply(static_cast<const HLADataType&>(dataType)); }
    virtual void apply(const HLAFixedArrayDataType& dataType)
    { apply(static_cast<const HLAArrayDataType&>(dataType)); }
    virtual void apply(const HLAVariableArrayDataType& dataType)
    { apply(static_cast<const HLAArrayDataType&>(dataType)); }

    virtual void apply(const HLAEnumeratedDataType& dataType)
    { apply(static_cast<const HLADataType&>(dataType)); }

    virtual void apply(const HLAFixedRecordDataType& dataType)
    { apply(static_cast<const HLADataType&>(dataType)); }

    virtual void apply(const HLAVariantRecordDataType& dataType)
    { apply(static_cast<const HLADataType&>(dataType)); }
};

/// Walks the datatype tree and checks if it is completely defined
class HLADataTypeCheckVisitor : public HLADataTypeVisitor {
public:
    HLADataTypeCheckVisitor() : _valid(true) {}

    bool valid() const { return _valid; }

    virtual void apply(const HLAInt8DataType& dataType) { }
    virtual void apply(const HLAUInt8DataType& dataType) { }
    virtual void apply(const HLAInt16DataType& dataType) { }
    virtual void apply(const HLAUInt16DataType& dataType) { }
    virtual void apply(const HLAInt32DataType& dataType) { }
    virtual void apply(const HLAUInt32DataType& dataType) { }
    virtual void apply(const HLAInt64DataType& dataType) { }
    virtual void apply(const HLAUInt64DataType& dataType) { }
    virtual void apply(const HLAFloat32DataType& dataType) { }
    virtual void apply(const HLAFloat64DataType& dataType) { }

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        if (!dataType.getElementDataType())
            _valid = false;
        else
            dataType.getElementDataType()->accept(*this);
    }
    virtual void apply(const HLAVariableArrayDataType& dataType)
    {
        if (!dataType.getElementDataType())
            _valid = false;
        else
            dataType.getElementDataType()->accept(*this);

        if (!dataType.getSizeDataType())
            _valid = false;
        else
            dataType.getSizeDataType()->accept(*this);
    }

    virtual void apply(const HLAEnumeratedDataType& dataType)
    {
        if (!dataType.getRepresentation())
            _valid = false;
        else
            dataType.getRepresentation()->accept(*this);
    }

    virtual void apply(const HLAFixedRecordDataType& dataType)
    {
        unsigned numFields = dataType.getNumFields();
        for (unsigned i = 0; i < numFields; ++i) {
            if (!dataType.getFieldDataType(i))
                _valid = false;
            else
                dataType.getFieldDataType(i)->accept(*this);
        }
    }

    virtual void apply(const HLAVariantRecordDataType& dataType)
    { assert(0); }

protected:
    bool _valid;
};

class HLADataTypeDecodeVisitor : public HLADataTypeVisitor {
public:
    HLADataTypeDecodeVisitor(HLADecodeStream& stream) : _stream(stream) {}
    virtual ~HLADataTypeDecodeVisitor() {}

    virtual void apply(const HLAInt8DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAUInt8DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAInt16DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAUInt16DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAInt32DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAUInt32DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAInt64DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAUInt64DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAFloat32DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAFloat64DataType& dataType) { dataType.skip(_stream); }

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        unsigned numElements = dataType.getNumElements();
        for (unsigned i = 0; i < numElements; ++i)
            dataType.getElementDataType()->accept(*this);
    }
    virtual void apply(const HLAVariableArrayDataType& dataType);

    virtual void apply(const HLAEnumeratedDataType& dataType)
    {
        dataType.getRepresentation()->accept(*this);
    }

    virtual void apply(const HLAFixedRecordDataType& dataType)
    {
        unsigned numFields = dataType.getNumFields();
        for (unsigned i = 0; i < numFields; ++i)
            dataType.getFieldDataType(i)->accept(*this);
    }

    virtual void apply(const HLAVariantRecordDataType& dataType) { assert(0); }

protected:
    HLADecodeStream& _stream;
};

class HLADataTypeEncodeVisitor : public HLADataTypeVisitor {
public:
    HLADataTypeEncodeVisitor(HLAEncodeStream& stream) : _stream(stream) {}
    virtual ~HLADataTypeEncodeVisitor() {}

    virtual void apply(const HLAInt8DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAUInt8DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAInt16DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAUInt16DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAInt32DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAUInt32DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAInt64DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAUInt64DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAFloat32DataType& dataType) { dataType.skip(_stream); }
    virtual void apply(const HLAFloat64DataType& dataType) { dataType.skip(_stream); }

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        /// FIXME, might vanish in this sense ...
        // dataType.encode(_stream, *this);
        unsigned numElements = dataType.getNumElements();
        for (unsigned i = 0; i < numElements; ++i)
            dataType.getElementDataType()->accept(*this);
    }
    virtual void apply(const HLAVariableArrayDataType& dataType);

    virtual void apply(const HLAEnumeratedDataType& dataType)
    {
        dataType.getRepresentation()->accept(*this);
    }

    virtual void apply(const HLAFixedRecordDataType& dataType)
    {
        unsigned numFields = dataType.getNumFields();
        for (unsigned i = 0; i < numFields; ++i)
            dataType.getFieldDataType(i)->accept(*this);
    }

    virtual void apply(const HLAVariantRecordDataType& dataType) { assert(0); }

protected:
    HLAEncodeStream& _stream;
};

/// Begin implementation of some get/set visitors

class HLAScalarDecodeVisitor : public HLADataTypeDecodeVisitor {
public:
    HLAScalarDecodeVisitor(HLADecodeStream& stream) :
        HLADataTypeDecodeVisitor(stream)
    {}

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing scalar value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAVariableArrayDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing scalar value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAEnumeratedDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing scalar value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAFixedRecordDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing scalar value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAVariantRecordDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing scalar value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
};

class HLAScalarEncodeVisitor : public HLADataTypeEncodeVisitor {
public:
    HLAScalarEncodeVisitor(HLAEncodeStream& stream) :
        HLADataTypeEncodeVisitor(stream)
    {}

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing scalar value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAVariableArrayDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing scalar value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAEnumeratedDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing scalar value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAFixedRecordDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing scalar value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAVariantRecordDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing scalar value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
};

class HLAFixedRecordDecodeVisitor : public HLADataTypeDecodeVisitor {
public:
    HLAFixedRecordDecodeVisitor(HLADecodeStream& stream) :
        HLADataTypeDecodeVisitor(stream)
    { }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAVariableArrayDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAEnumeratedDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAVariantRecordDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while decodeing a fixed record value");
        HLADataTypeDecodeVisitor::apply(dataType);
    }
};

class HLAFixedRecordEncodeVisitor : public HLADataTypeEncodeVisitor {
public:
    HLAFixedRecordEncodeVisitor(HLAEncodeStream& stream) :
        HLADataTypeEncodeVisitor(stream)
    { }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
    virtual void apply(const HLAVariableArrayDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAEnumeratedDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }

    virtual void apply(const HLAVariantRecordDataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_WARN, "Unexpected HLADataType while writing a fixed record value");
        HLADataTypeEncodeVisitor::apply(dataType);
    }
};

template<typename T>
class HLATemplateDecodeVisitor : public HLAScalarDecodeVisitor {
public:
    typedef T value_type;

    HLATemplateDecodeVisitor(HLADecodeStream& stream, const value_type& value = value_type()) :
        HLAScalarDecodeVisitor(stream),
        _value(value)
    {}

    void setValue(const value_type& value)
    { _value = value; }
    const value_type& getValue() const
    { return _value; }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        int8_t value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        uint8_t value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        int16_t value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        uint16_t value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        int32_t value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        uint32_t value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        int64_t value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        uint64_t value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        float value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        double value = 0;
        dataType.decode(_stream, value);
        _value = value_type(value);
    }

private:
    value_type _value;
};

template<typename T>
class HLATemplateEncodeVisitor : public HLAScalarEncodeVisitor {
public:
    typedef T value_type;

    HLATemplateEncodeVisitor(HLAEncodeStream& stream, const value_type& value = value_type()) :
        HLAScalarEncodeVisitor(stream),
        _value(value)
    {}

    void setValue(const value_type& value)
    { _value = value; }
    const value_type& getValue() const
    { return _value; }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        dataType.encode(_stream, _value);
    }

private:
    value_type _value;
};

inline void HLADataTypeDecodeVisitor::apply(const HLAVariableArrayDataType& dataType)
{
    HLATemplateDecodeVisitor<unsigned> numElementsVisitor(_stream);
    dataType.getSizeDataType()->accept(numElementsVisitor);
    unsigned numElements = numElementsVisitor.getValue();
    for (unsigned i = 0; i < numElements; ++i)
        dataType.getElementDataType()->accept(*this);
}

inline void HLADataTypeEncodeVisitor::apply(const HLAVariableArrayDataType& dataType)
{
    HLATemplateEncodeVisitor<unsigned> numElementsVisitor(_stream, 0);
    dataType.getSizeDataType()->accept(numElementsVisitor);
}

/// Generate standard data elements according to the traversed type
class HLADataElementFactoryVisitor : public HLADataTypeVisitor {
public:
    virtual ~HLADataElementFactoryVisitor();

    virtual void apply(const HLADataType& dataType);

    virtual void apply(const HLAInt8DataType& dataType);
    virtual void apply(const HLAUInt8DataType& dataType);
    virtual void apply(const HLAInt16DataType& dataType);
    virtual void apply(const HLAUInt16DataType& dataType);
    virtual void apply(const HLAInt32DataType& dataType);
    virtual void apply(const HLAUInt32DataType& dataType);
    virtual void apply(const HLAInt64DataType& dataType);
    virtual void apply(const HLAUInt64DataType& dataType);
    virtual void apply(const HLAFloat32DataType& dataType);
    virtual void apply(const HLAFloat64DataType& dataType);

    virtual void apply(const HLAFixedArrayDataType& dataType);
    virtual void apply(const HLAVariableArrayDataType& dataType);

    virtual void apply(const HLAEnumeratedDataType& dataType);

    virtual void apply(const HLAFixedRecordDataType& dataType);

    virtual void apply(const HLAVariantRecordDataType& dataType);

    HLADataElement* getDataElement()
    { return _dataElement.release(); }

protected:
    class ArrayDataElementFactory;
    class VariantRecordDataElementFactory;

    SGSharedPtr<HLADataElement> _dataElement;
};

} // namespace simgear

#endif
