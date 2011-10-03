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
    DecodeVisitor(HLADecodeStream& stream, SGPropertyNode& propertyNode) :
        HLADataTypeDecodeVisitor(stream),
        _propertyNode(propertyNode)
    { }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        int8_t value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setIntValue(value);
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        uint8_t value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setIntValue(value);
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        int16_t value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setIntValue(value);
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        uint16_t value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setIntValue(value);
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        int32_t value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setIntValue(value);
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        uint32_t value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setIntValue(value);
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        int64_t value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setLongValue(value);
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        uint64_t value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setLongValue(value);
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        float value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setFloatValue(value);
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        double value = 0;
        dataType.decode(_stream, value);
        _propertyNode.setDoubleValue(value);
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
        _propertyNode.setStringValue(value);
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
        _propertyNode.setStringValue(value);
    }

protected:
    SGPropertyNode& _propertyNode;
};

class HLAPropertyDataElement::EncodeVisitor : public HLADataTypeEncodeVisitor {
public:
    EncodeVisitor(HLAEncodeStream& stream, const SGPropertyNode& propertyNode) :
        HLADataTypeEncodeVisitor(stream),
        _propertyNode(propertyNode)
    { }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getIntValue());
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getIntValue());
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getIntValue());
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getIntValue());
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getIntValue());
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getIntValue());
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getLongValue());
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getLongValue());
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getFloatValue());
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        dataType.encode(_stream, _propertyNode.getDoubleValue());
    }

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        unsigned numElements = dataType.getNumElements();
        std::string value = _propertyNode.getStringValue();
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
        std::string value = _propertyNode.getStringValue();
        HLATemplateEncodeVisitor<std::string::size_type> numElementsVisitor(_stream, value.size());
        dataType.getSizeDataType()->accept(numElementsVisitor);
        for (unsigned i = 0; i < value.size(); ++i) {
            HLATemplateEncodeVisitor<char> visitor(_stream, value[i]);
            dataType.getElementDataType()->accept(visitor);
        }
    }

protected:
    const SGPropertyNode& _propertyNode;
};

HLAPropertyDataElement::HLAPropertyDataElement()
{
}

HLAPropertyDataElement::HLAPropertyDataElement(SGPropertyNode* propertyNode)
{
    setPropertyNode(propertyNode);
}

HLAPropertyDataElement::HLAPropertyDataElement(const HLADataType* dataType, SGPropertyNode* propertyNode) :
    _dataType(dataType)
{
    setPropertyNode(propertyNode);
}

HLAPropertyDataElement::HLAPropertyDataElement(const HLADataType* dataType) :
    _dataType(dataType)
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
    if (const SGPropertyNode* propertyNode = getPropertyNode()) {
        EncodeVisitor visitor(stream, *propertyNode);
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
    if (SGPropertyNode* propertyNode = getPropertyNode()) {
        DecodeVisitor visitor(stream, *propertyNode);
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

void
HLAPropertyDataElement::setPropertyNode(SGPropertyNode* propertyNode)
{
    _propertyNode = propertyNode;
}

SGPropertyNode*
HLAPropertyDataElement::getPropertyNode()
{
    return _propertyNode.get();
}

const SGPropertyNode*
HLAPropertyDataElement::getPropertyNode() const
{
    return _propertyNode.get();
}

} // namespace simgear
