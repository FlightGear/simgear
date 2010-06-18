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

#include "HLAPropertyDataElement.hxx"

#include "HLADataTypeVisitor.hxx"

namespace simgear {

class HLAPropertyDataElement::DecodeVisitor : public HLADataTypeDecodeVisitor {
public:
    DecodeVisitor(HLADecodeStream& stream, HLAPropertyReference& propertyReference) :
        HLADataTypeDecodeVisitor(stream),
        _propertyReference(propertyReference)
    { }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        int8_t value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setIntValue(value);
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        uint8_t value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setIntValue(value);
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        int16_t value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setIntValue(value);
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        uint16_t value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setIntValue(value);
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        int32_t value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setIntValue(value);
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        uint32_t value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setIntValue(value);
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        int64_t value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setLongValue(value);
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        uint64_t value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setLongValue(value);
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        float value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setFloatValue(value);
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        double value = 0;
        dataType.decode(_stream, value);
        _propertyReference.setDoubleValue(value);
    }

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        unsigned numElements = dataType.getNumElements();
        std::string value;
        value.reserve(numElements);
        for (unsigned i = 0; i < numElements; ++i) {
            HLATemplateDecodeVisitor<char> visitor(_stream);
            dataType.getElementDataType()->accept(visitor);
            value.push_back(visitor.getValue());
        }
        _propertyReference.setStringValue(value);
    }
    virtual void apply(const HLAVariableArrayDataType& dataType)
    {
        HLATemplateDecodeVisitor<unsigned> numElementsVisitor(_stream);
        dataType.getSizeDataType()->accept(numElementsVisitor);
        unsigned numElements = numElementsVisitor.getValue();
        std::string value;
        value.reserve(numElements);
        for (unsigned i = 0; i < numElements; ++i) {
            HLATemplateDecodeVisitor<char> visitor(_stream);
            dataType.getElementDataType()->accept(visitor);
            value.push_back(visitor.getValue());
        }
        _propertyReference.setStringValue(value);
    }

protected:
    HLAPropertyReference& _propertyReference;
};

class HLAPropertyDataElement::EncodeVisitor : public HLADataTypeEncodeVisitor {
public:
    EncodeVisitor(HLAEncodeStream& stream, const HLAPropertyReference& propertyReference) :
        HLADataTypeEncodeVisitor(stream),
        _propertyReference(propertyReference)
    { }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getIntValue());
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getIntValue());
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getIntValue());
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getIntValue());
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getIntValue());
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getIntValue());
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getLongValue());
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getLongValue());
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getFloatValue());
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        dataType.encode(_stream, _propertyReference.getDoubleValue());
    }

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        unsigned numElements = dataType.getNumElements();
        std::string value = _propertyReference.getStringValue();
        for (unsigned i = 0; i < numElements; ++i) {
            if (i < value.size()) {
                HLATemplateEncodeVisitor<char> visitor(_stream, value[i]);
                dataType.getElementDataType()->accept(visitor);
            } else {
                HLADataTypeEncodeVisitor visitor(_stream);
                dataType.getElementDataType()->accept(visitor);
            }
        }
    }

    virtual void apply(const HLAVariableArrayDataType& dataType)
    {
        std::string value = _propertyReference.getStringValue();
        HLATemplateEncodeVisitor<std::string::size_type> numElementsVisitor(_stream, value.size());
        dataType.getSizeDataType()->accept(numElementsVisitor);
        for (unsigned i = 0; i < value.size(); ++i) {
            HLATemplateEncodeVisitor<char> visitor(_stream, value[i]);
            dataType.getElementDataType()->accept(visitor);
        }
    }

protected:
    const HLAPropertyReference& _propertyReference;
};

HLAPropertyDataElement::HLAPropertyDataElement(HLAPropertyReference* propertyReference) :
    _propertyReference(propertyReference)
{
}

HLAPropertyDataElement::HLAPropertyDataElement(const simgear::HLADataType* dataType, HLAPropertyReference* propertyReference) :
    _dataType(dataType),
    _propertyReference(propertyReference)
{
}

HLAPropertyDataElement::~HLAPropertyDataElement()
{
}

bool
HLAPropertyDataElement::encode(HLAEncodeStream& stream) const
{
    if (!_dataType.valid())
        return false;
    if (_propertyReference.valid()) {
        EncodeVisitor visitor(stream, *_propertyReference);
        _dataType->accept(visitor);
    } else {
        HLADataTypeEncodeVisitor visitor(stream);
        _dataType->accept(visitor);
    }
    return true;
}

bool
HLAPropertyDataElement::decode(HLADecodeStream& stream)
{
    if (!_dataType.valid())
        return false;
    if (_propertyReference.valid()) {
        DecodeVisitor visitor(stream, *_propertyReference);
        _dataType->accept(visitor);
    } else {
        HLADataTypeDecodeVisitor visitor(stream);
        _dataType->accept(visitor);
    }
    return true;
}

const HLADataType*
HLAPropertyDataElement::getDataType() const
{
    return _dataType.get();
}

bool
HLAPropertyDataElement::setDataType(const HLADataType* dataType)
{
    if (dataType->toBasicDataType()) {
        _dataType = dataType;
        return true;
    } else {
        const HLAArrayDataType* arrayDataType = dataType->toArrayDataType();
        if (arrayDataType && arrayDataType->getElementDataType() &&
            arrayDataType->getElementDataType()->toBasicDataType()) {
            _dataType = dataType;
            return true;
        }
    }
    return false;
}

} // namespace simgear
