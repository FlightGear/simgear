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

#include "HLAPropertyDataElement.hxx"

#include "HLAArrayDataElement.hxx"
#include "HLABasicDataElement.hxx"
#include "HLADataElementVisitor.hxx"
#include "HLADataTypeVisitor.hxx"
#include "HLAFixedRecordDataElement.hxx"
#include "HLAVariantRecordDataElement.hxx"

namespace simgear {

class HLAPropertyDataElement::ScalarDecodeVisitor : public HLADataTypeDecodeVisitor {
public:
    ScalarDecodeVisitor(HLADecodeStream& stream, SGPropertyNode& propertyNode) :
        HLADataTypeDecodeVisitor(stream),
        _propertyNode(propertyNode)
    { }
    virtual ~ScalarDecodeVisitor()
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

protected:
    SGPropertyNode& _propertyNode;
};

class HLAPropertyDataElement::ScalarEncodeVisitor : public HLADataTypeEncodeVisitor {
public:
    ScalarEncodeVisitor(HLAEncodeStream& stream, const SGPropertyNode& propertyNode) :
        HLADataTypeEncodeVisitor(stream),
        _propertyNode(propertyNode)
    { }
    virtual ~ScalarEncodeVisitor()
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

protected:
    const SGPropertyNode& _propertyNode;
};

class HLAPropertyDataElement::ScalarDataElement : public HLABasicDataElement {
public:
    ScalarDataElement(const HLABasicDataType* dataType, SGPropertyNode* propertyNode);
    virtual ~ScalarDataElement();

    virtual bool encode(HLAEncodeStream& stream) const;
    virtual bool decode(HLADecodeStream& stream);

private:
    SGSharedPtr<SGPropertyNode> _propertyNode;
};

HLAPropertyDataElement::ScalarDataElement::ScalarDataElement(const HLABasicDataType* dataType, SGPropertyNode* propertyNode) :
    HLABasicDataElement(dataType),
    _propertyNode(propertyNode)
{
}

HLAPropertyDataElement::ScalarDataElement::~ScalarDataElement()
{
}

bool
HLAPropertyDataElement::ScalarDataElement::encode(HLAEncodeStream& stream) const
{
    ScalarEncodeVisitor visitor(stream, *_propertyNode);
    _dataType->accept(visitor);
    return true;
}

bool
HLAPropertyDataElement::ScalarDataElement::decode(HLADecodeStream& stream)
{
    ScalarDecodeVisitor visitor(stream, *_propertyNode);
    _dataType->accept(visitor);
    return true;
}

class HLAPropertyDataElement::StringDataElement : public HLAStringDataElement {
public:
    StringDataElement(const HLAArrayDataType* dataType, SGPropertyNode* propertyNode);
    virtual ~StringDataElement();

    virtual bool decodeElement(HLADecodeStream& stream, unsigned i);

    class Listener : public SGPropertyChangeListener {
    public:
        Listener(StringDataElement* stringDataElement);
        virtual ~Listener();
        virtual void valueChanged (SGPropertyNode * node);
    private:
        StringDataElement* _stringDataElement;
    };

private:
    SGSharedPtr<SGPropertyNode> _propertyNode;
    Listener* _listener;
};

HLAPropertyDataElement::StringDataElement::Listener::Listener(StringDataElement* stringDataElement) :
            _stringDataElement(stringDataElement)
{
}

HLAPropertyDataElement::StringDataElement::Listener::~Listener()
{
}

void
HLAPropertyDataElement::StringDataElement::Listener::valueChanged (SGPropertyNode * node)
{
    _stringDataElement->setValue(node->getStringValue());
}

HLAPropertyDataElement::StringDataElement::StringDataElement(const HLAArrayDataType* dataType, SGPropertyNode* propertyNode) :
    HLAStringDataElement(dataType),
    _propertyNode(propertyNode),
    _listener(new Listener(this))
{
    _propertyNode->addChangeListener(_listener, true);
}

HLAPropertyDataElement::StringDataElement::~StringDataElement()
{
    _propertyNode->removeChangeListener(_listener);
    delete _listener;
    _listener = 0;
}

bool
HLAPropertyDataElement::StringDataElement::decodeElement(HLADecodeStream& stream, unsigned i)
{
    if (!HLAStringDataElement::decodeElement(stream, i))
        return false;
    if (i + 1 == getValue().size())
        _propertyNode->setStringValue(getValue());
    return true;
}

class HLAPropertyDataElement::DataElementFactoryVisitor : public HLADataTypeVisitor {
public:
    DataElementFactoryVisitor(SGPropertyNode* propertyNode) :
        _propertyNode(propertyNode)
    { }
    virtual ~DataElementFactoryVisitor()
    { }

    virtual void apply(const HLADataType& dataType)
    {
        SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Can not find a suitable data element for data type \""
               << dataType.getName() << "\"");
    }

    virtual void apply(const HLAInt8DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }
    virtual void apply(const HLAUInt8DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }
    virtual void apply(const HLAInt16DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }
    virtual void apply(const HLAUInt16DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }
    virtual void apply(const HLAInt32DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }
    virtual void apply(const HLAUInt32DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }
    virtual void apply(const HLAInt64DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }
    virtual void apply(const HLAUInt64DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }
    virtual void apply(const HLAFloat32DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }
    virtual void apply(const HLAFloat64DataType& dataType)
    {
        _dataElement = new ScalarDataElement(&dataType, _propertyNode.get());
    }

    class ArrayDataElementFactory : public HLAArrayDataElement::DataElementFactory {
    public:
        ArrayDataElementFactory(SGPropertyNode* propertyNode) :
            _propertyNode(propertyNode)
        { }
        virtual HLADataElement* createElement(const HLAArrayDataElement& element, unsigned index)
        {
            const HLADataType* dataType = element.getElementDataType();
            if (!dataType)
                return 0;

            SGPropertyNode* parent = _propertyNode->getParent();
            DataElementFactoryVisitor visitor(parent->getChild(_propertyNode->getNameString(), index, true));
            dataType->accept(visitor);
            return visitor.getDataElement();
        }
    private:
        SGSharedPtr<SGPropertyNode> _propertyNode;
    };

    virtual void apply(const HLAFixedArrayDataType& dataType)
    {
        if (dataType.getIsString()) {
            _dataElement = new StringDataElement(&dataType, _propertyNode.get());
        } else {
            SGSharedPtr<HLAArrayDataElement> arrayDataElement;
            arrayDataElement = new HLAArrayDataElement(&dataType);
            arrayDataElement->setDataElementFactory(new ArrayDataElementFactory(_propertyNode.get()));
            arrayDataElement->setNumElements(dataType.getNumElements());
            _dataElement = arrayDataElement;
        }
    }

    virtual void apply(const HLAVariableArrayDataType& dataType)
    {
        if (dataType.getIsString()) {
            _dataElement = new StringDataElement(&dataType, _propertyNode.get());
        } else {
            SGSharedPtr<HLAArrayDataElement> arrayDataElement;
            arrayDataElement = new HLAArrayDataElement(&dataType);
            arrayDataElement->setDataElementFactory(new ArrayDataElementFactory(_propertyNode.get()));
            _dataElement = arrayDataElement;
        }
    }

    virtual void apply(const HLAEnumeratedDataType& dataType)
    {
        _dataElement = new ScalarDataElement(dataType.getRepresentation(), _propertyNode.get());
    }

    virtual void apply(const HLAFixedRecordDataType& dataType)
    {
        SGSharedPtr<HLAFixedRecordDataElement> recordDataElement;
        recordDataElement = new HLAFixedRecordDataElement(&dataType);

        unsigned numFields = dataType.getNumFields();
        for (unsigned i = 0; i < numFields; ++i) {
            DataElementFactoryVisitor visitor(_propertyNode->getChild(dataType.getFieldName(i), 0, true));
            dataType.getFieldDataType(i)->accept(visitor);
            recordDataElement->setField(i, visitor._dataElement.get());
        }

        _dataElement = recordDataElement;
    }

    class VariantRecordDataElementFactory : public HLAVariantRecordDataElement::DataElementFactory {
    public:
        VariantRecordDataElementFactory(SGPropertyNode* propertyNode) :
            _propertyNode(propertyNode)
        { }
        virtual HLADataElement* createElement(const HLAVariantRecordDataElement& element, unsigned index)
        {
            const HLAVariantRecordDataType* dataType = element.getDataType();
            if (!dataType)
                return 0;
            const HLADataType* alternativeDataType = element.getAlternativeDataType();
            if (!alternativeDataType)
                return 0;
            DataElementFactoryVisitor visitor(_propertyNode->getChild(dataType->getAlternativeName(index), 0, true));
            alternativeDataType->accept(visitor);
            return visitor.getDataElement();
        }
    private:
        SGSharedPtr<SGPropertyNode> _propertyNode;
    };

    virtual void apply(const HLAVariantRecordDataType& dataType)
    {
        SGSharedPtr<HLAVariantRecordDataElement> variantRecordDataElement;
        variantRecordDataElement = new HLAVariantRecordDataElement(&dataType);
        variantRecordDataElement->setDataElementFactory(new VariantRecordDataElementFactory(_propertyNode.get()));
        _dataElement = variantRecordDataElement;
    }

    HLADataElement* getDataElement()
    { return _dataElement.release(); }

private:
    SGSharedPtr<SGPropertyNode> _propertyNode;
    SGSharedPtr<HLADataElement> _dataElement;
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

void
HLAPropertyDataElement::accept(HLADataElementVisitor& visitor)
{
    if (_dataElement.valid()) {
        visitor.apply(*_dataElement);
    } else {
        // We cant do anything if the data type is not valid
        if (_dataType.valid()) {
            HLADataElementFactoryVisitor factoryVisitor;
            _dataType->accept(factoryVisitor);
            _dataElement = factoryVisitor.getDataElement();
            if (_dataElement.valid()) {
                visitor.apply(*_dataElement);
            } else {
                HLADataElement::accept(visitor);
            }
        } else {
            HLADataElement::accept(visitor);
        }
    }
}

void
HLAPropertyDataElement::accept(HLAConstDataElementVisitor& visitor) const
{
    if (_dataElement.valid()) {
        visitor.apply(*_dataElement);
    } else {
        HLADataElement::accept(visitor);
    }
}

bool
HLAPropertyDataElement::encode(HLAEncodeStream& stream) const
{
    if (_dataElement.valid()) {
        return _dataElement->encode(stream);
    } else {
        if (!_dataType.valid())
            return false;
        HLADataTypeEncodeVisitor visitor(stream);
        _dataType->accept(visitor);
        return true;
    }
}

bool
HLAPropertyDataElement::decode(HLADecodeStream& stream)
{
    if (_dataElement.valid()) {
        return _dataElement->decode(stream);
    } else if (!_dataType.valid()) {
        // We cant do anything if the data type is not valid
        return false;
    } else {
        HLADataElementFactoryVisitor visitor;
        _dataType->accept(visitor);
        _dataElement = visitor.getDataElement();
        if (_dataElement.valid()) {
            return _dataElement->decode(stream);
        } else {
            HLADataTypeDecodeVisitor visitor(stream);
            _dataType->accept(visitor);
            return true;
        }
    }
}

const HLADataType*
HLAPropertyDataElement::getDataType() const
{
    return _dataType.get();
}

bool
HLAPropertyDataElement::setDataType(const HLADataType* dataType)
{
    _dataType = dataType;
    if (_dataType.valid() && _propertyNode.valid())
        _dataElement = createDataElement(_dataType, _propertyNode);
    return true;
}

void
HLAPropertyDataElement::setPropertyNode(SGPropertyNode* propertyNode)
{
    _propertyNode = propertyNode;
    if (_dataType.valid() && _propertyNode.valid())
        _dataElement = createDataElement(_dataType, _propertyNode);
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

HLADataElement*
HLAPropertyDataElement::createDataElement(const SGSharedPtr<const HLADataType>& dataType,
                                          const SGSharedPtr<SGPropertyNode>& propertyNode)
{
    DataElementFactoryVisitor visitor(propertyNode);
    dataType->accept(visitor);
    SGSharedPtr<HLADataElement> dataElement = visitor.getDataElement();

    // Copy over the content of the previous data element if there is any.
    if (_dataElement.valid()) {
        // FIXME is encode/decode the right tool here??
        RTIData data;
        HLAEncodeStream encodeStream(data);
        if (_dataElement->encode(encodeStream)) {
            HLADecodeStream decodeStream(data);
            dataElement->decode(decodeStream);
        }
    }

    return dataElement.release();
}

} // namespace simgear
