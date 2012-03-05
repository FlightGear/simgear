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

#include "HLADataTypeVisitor.hxx"

#include "HLAArrayDataElement.hxx"
#include "HLABasicDataElement.hxx"
#include "HLADataTypeVisitor.hxx"
#include "HLAEnumeratedDataElement.hxx"
#include "HLAFixedRecordDataElement.hxx"
#include "HLAVariantRecordDataElement.hxx"

namespace simgear {

HLADataElementFactoryVisitor::~HLADataElementFactoryVisitor()
{
}

void
HLADataElementFactoryVisitor::apply(const HLADataType& dataType)
{
    SG_LOG(SG_NETWORK, SG_ALERT, "HLA: Can not find a suitable data element for data type \""
           << dataType.getName() << "\"");
}

void
HLADataElementFactoryVisitor::apply(const HLAInt8DataType& dataType)
{
    _dataElement = new HLASCharDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAUInt8DataType& dataType)
{
    _dataElement = new HLAUCharDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAInt16DataType& dataType)
{
    _dataElement = new HLAShortDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAUInt16DataType& dataType)
{
    _dataElement = new HLAUShortDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAInt32DataType& dataType)
{
    _dataElement = new HLAIntDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAUInt32DataType& dataType)
{
    _dataElement = new HLAUIntDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAInt64DataType& dataType)
{
    _dataElement = new HLALongDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAUInt64DataType& dataType)
{
    _dataElement = new HLAULongDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAFloat32DataType& dataType)
{
    _dataElement = new HLAFloatDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAFloat64DataType& dataType)
{
    _dataElement = new HLADoubleDataElement(&dataType);
}

class HLADataElementFactoryVisitor::ArrayDataElementFactory : public HLAArrayDataElement::DataElementFactory {
public:
    virtual HLADataElement* createElement(const HLAArrayDataElement& element, unsigned index)
    {
        const HLADataType* dataType = element.getElementDataType();
        if (!dataType)
            return 0;
        
        HLADataElementFactoryVisitor visitor;
        dataType->accept(visitor);
        return visitor.getDataElement();
    }
};

void
HLADataElementFactoryVisitor::apply(const HLAFixedArrayDataType& dataType)
{
    if (dataType.getIsString()) {
        _dataElement = new HLAStringDataElement(&dataType);
    } else {
        SGSharedPtr<HLAArrayDataElement> arrayDataElement;
        arrayDataElement = new HLAArrayDataElement(&dataType);
        arrayDataElement->setDataElementFactory(new ArrayDataElementFactory);
        arrayDataElement->setNumElements(dataType.getNumElements());
        _dataElement = arrayDataElement;
    }
}

void
HLADataElementFactoryVisitor::apply(const HLAVariableArrayDataType& dataType)
{
    if (dataType.getIsString()) {
        _dataElement = new HLAStringDataElement(&dataType);
    } else {
        SGSharedPtr<HLAArrayDataElement> arrayDataElement;
        arrayDataElement = new HLAArrayDataElement(&dataType);
        arrayDataElement->setDataElementFactory(new ArrayDataElementFactory);
        _dataElement = arrayDataElement;
    }
}

void
HLADataElementFactoryVisitor::apply(const HLAEnumeratedDataType& dataType)
{
    _dataElement = new HLAEnumeratedDataElement(&dataType);
}

void
HLADataElementFactoryVisitor::apply(const HLAFixedRecordDataType& dataType)
{
    SGSharedPtr<HLAFixedRecordDataElement> recordDataElement;
    recordDataElement = new HLAFixedRecordDataElement(&dataType);

    unsigned numFields = dataType.getNumFields();
    for (unsigned i = 0; i < numFields; ++i) {
        HLADataElementFactoryVisitor visitor;
        dataType.getFieldDataType(i)->accept(visitor);
        recordDataElement->setField(i, visitor._dataElement.get());
    }

    _dataElement = recordDataElement;
}

class HLADataElementFactoryVisitor::VariantRecordDataElementFactory : public HLAVariantRecordDataElement::DataElementFactory {
public:
    virtual HLADataElement* createElement(const HLAVariantRecordDataElement& element, unsigned index)
    {
        const HLAVariantRecordDataType* dataType = element.getDataType();
        if (!dataType)
            return 0;
        const HLADataType* alternativeDataType = element.getAlternativeDataType();
        if (!alternativeDataType)
            return 0;
        HLADataElementFactoryVisitor visitor;
        alternativeDataType->accept(visitor);
        return visitor.getDataElement();
    }
};

void
HLADataElementFactoryVisitor::apply(const HLAVariantRecordDataType& dataType)
{
    SGSharedPtr<HLAVariantRecordDataElement> variantRecordDataElement;
    variantRecordDataElement = new HLAVariantRecordDataElement(&dataType);
    variantRecordDataElement->setDataElementFactory(new VariantRecordDataElementFactory);
    _dataElement = variantRecordDataElement;
}

} // namespace simgear
