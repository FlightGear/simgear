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
    if (!_dataType.valid())
        return 0;
    return _dataType->getNumFields();
}

unsigned
HLAAbstractFixedRecordDataElement::getFieldNumber(const std::string& name) const
{
    if (!_dataType.valid())
        return ~0u;
    return _dataType->getFieldNumber(name);
}

const HLADataType*
HLAAbstractFixedRecordDataElement::getFieldDataType(unsigned i) const
{
    if (!_dataType.valid())
        return 0;
    return _dataType->getFieldDataType(i);
}

const HLADataType*
HLAAbstractFixedRecordDataElement::getFieldDataType(const std::string& name) const
{
    if (!_dataType.valid())
        return 0;
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
HLAFixedRecordDataElement::setDataType(const HLADataType* dataType)
{
    if (!HLAAbstractFixedRecordDataElement::setDataType(dataType))
        return false;
    _fieldVector.resize(getNumFields());
    return true;
}

bool
HLAFixedRecordDataElement::setDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end, HLADataElement* dataElement)
{
    // Must have happened in the parent
    if (begin == end)
        return false;
    unsigned index = *begin;
    if (++begin != end) {
        if (getNumFields() <= index)
            return false;
        if (!getField(index) && getFieldDataType(index)) {
            HLADataElementFactoryVisitor visitor;
            getFieldDataType(index)->accept(visitor);
            setField(index, visitor.getDataElement());
        }
        if (!getField(index))
            return false;
        return getField(index)->setDataElement(begin, end, dataElement);
    } else {
        if (!dataElement->setDataType(getFieldDataType(index)))
            return false;
        setField(index, dataElement);
        return true;
    }
}

HLADataElement*
HLAFixedRecordDataElement::getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end)
{
    if (begin == end)
        return this;
    HLADataElement* dataElement = getField(*begin);
    if (!dataElement)
        return 0;
    return dataElement->getDataElement(++begin, end);
}

const HLADataElement*
HLAFixedRecordDataElement::getDataElement(HLADataElementIndex::const_iterator begin, HLADataElementIndex::const_iterator end) const
{
    if (begin == end)
        return this;
    const HLADataElement* dataElement = getField(*begin);
    if (!dataElement)
        return 0;
    return dataElement->getDataElement(++begin, end);
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
