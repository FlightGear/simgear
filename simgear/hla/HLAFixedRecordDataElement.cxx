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

#include "HLAFixedRecordDataElement.hxx"

#include <string>
#include <vector>
#include <simgear/debug/logstream.hxx>

#include "HLADataElementVisitor.hxx"
#include "HLADataTypeVisitor.hxx"

namespace simgear {

HLAAbstractFixedRecordDataElement::HLAAbstractFixedRecordDataElement(const HLAFixedRecordDataType* dataType) :
    _dataType(dataType)
{
}

HLAAbstractFixedRecordDataElement::~HLAAbstractFixedRecordDataElement()
{
}

void
HLAAbstractFixedRecordDataElement::accept(HLADataElementVisitor& visitor)
{
    visitor.apply(*this);
}

void
HLAAbstractFixedRecordDataElement::accept(HLAConstDataElementVisitor& visitor) const
{
    visitor.apply(*this);
}

bool
HLAAbstractFixedRecordDataElement::decode(HLADecodeStream& stream)
{
    if (!_dataType.valid())
        return false;
    return _dataType->decode(stream, *this);
}

bool
HLAAbstractFixedRecordDataElement::encode(HLAEncodeStream& stream) const
{
    if (!_dataType.valid())
        return false;
    return _dataType->encode(stream, *this);
}

const HLAFixedRecordDataType*
HLAAbstractFixedRecordDataElement::getDataType() const
{
    return _dataType.get();
}

bool
HLAAbstractFixedRecordDataElement::setDataType(const HLADataType* dataType)
{
    const HLAFixedRecordDataType* fixedRecordDataType = dataType->toFixedRecordDataType();
    if (!fixedRecordDataType) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAFixedRecordDataType: unable to set data type!");
        return false;
    }
    setDataType(fixedRecordDataType);
    return true;
}

void
HLAAbstractFixedRecordDataElement::setDataType(const HLAFixedRecordDataType* dataType)
{
    _dataType = dataType;
}

unsigned
HLAAbstractFixedRecordDataElement::getNumFields() const
{
    return _dataType->getNumFields();
}

unsigned
HLAAbstractFixedRecordDataElement::getFieldNumber(const std::string& name) const
{
    return _dataType->getFieldNumber(name);
}

const HLADataType*
HLAAbstractFixedRecordDataElement::getFieldDataType(unsigned i) const
{
    return _dataType->getFieldDataType(i);
}

const HLADataType*
HLAAbstractFixedRecordDataElement::getFieldDataType(const std::string& name) const
{
    return getFieldDataType(getFieldNumber(name));
}


HLAFixedRecordDataElement::HLAFixedRecordDataElement(const HLAFixedRecordDataType* dataType) :
    HLAAbstractFixedRecordDataElement(dataType)
{
    _fieldVector.resize(getNumFields());
}

HLAFixedRecordDataElement::~HLAFixedRecordDataElement()
{
    clearStamp();
}

bool
HLAFixedRecordDataElement::decodeField(HLADecodeStream& stream, unsigned i)
{
    HLADataElement* dataElement = getField(i);
    if (dataElement) {
        return dataElement->decode(stream);
    } else {
        HLADataTypeDecodeVisitor visitor(stream);
        getDataType()->getFieldDataType(i)->accept(visitor);
        return true;
    }
}

bool
HLAFixedRecordDataElement::encodeField(HLAEncodeStream& stream, unsigned i) const
{
    const HLADataElement* dataElement = getField(i);
    if (dataElement) {
        return dataElement->encode(stream);
    } else {
        HLADataTypeEncodeVisitor visitor(stream);
        getDataType()->getFieldDataType(i)->accept(visitor);
        return true;
    }
}

HLADataElement*
HLAFixedRecordDataElement::getField(const std::string& name)
{
    return getField(getFieldNumber(name));
}

const HLADataElement*
HLAFixedRecordDataElement::getField(const std::string& name) const
{
    return getField(getFieldNumber(name));
}

HLADataElement*
HLAFixedRecordDataElement::getField(unsigned i)
{
    if (_fieldVector.size() <= i)
        return 0;
    return _fieldVector[i].get();
}

const HLADataElement*
HLAFixedRecordDataElement::getField(unsigned i) const
{
    if (_fieldVector.size() <= i)
        return 0;
    return _fieldVector[i].get();
}

void
HLAFixedRecordDataElement::setField(unsigned index, HLADataElement* value)
{
    if (getNumFields() <= index)
        return;
    if (_fieldVector[index].valid())
        _fieldVector[index]->clearStamp();
    _fieldVector[index] = value;
    if (value)
        value->attachStamp(*this);
    setDirty(true);
}

void
HLAFixedRecordDataElement::setField(const std::string& name, HLADataElement* value)
{
    setField(getFieldNumber(name), value);
}

void
HLAFixedRecordDataElement::_setStamp(Stamp* stamp)
{
    HLAAbstractFixedRecordDataElement::_setStamp(stamp);
    for (FieldVector::iterator i = _fieldVector.begin(); i != _fieldVector.end(); ++i) {
        if (!i->valid())
            continue;
        (*i)->attachStamp(*this);
    }
}

}
