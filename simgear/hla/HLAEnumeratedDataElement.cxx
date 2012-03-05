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

#include "HLAEnumeratedDataElement.hxx"

#include <simgear/debug/logstream.hxx>

#include "HLADataElementVisitor.hxx"

namespace simgear {

HLAAbstractEnumeratedDataElement::HLAAbstractEnumeratedDataElement(const HLAEnumeratedDataType* dataType) :
    _dataType(dataType)
{
}

HLAAbstractEnumeratedDataElement::~HLAAbstractEnumeratedDataElement()
{
}

void
HLAAbstractEnumeratedDataElement::accept(HLADataElementVisitor& visitor)
{
    visitor.apply(*this);
}

void
HLAAbstractEnumeratedDataElement::accept(HLAConstDataElementVisitor& visitor) const
{
    visitor.apply(*this);
}

bool
HLAAbstractEnumeratedDataElement::decode(HLADecodeStream& stream)
{
    if (!_dataType.valid())
        return false;
    unsigned index;
    if (!_dataType->decode(stream, index))
        return false;
    setEnumeratorIndex(index);
    return true;
}

bool
HLAAbstractEnumeratedDataElement::encode(HLAEncodeStream& stream) const
{
    if (!_dataType.valid())
        return false;
    return _dataType->encode(stream, getEnumeratorIndex());
}

const HLAEnumeratedDataType*
HLAAbstractEnumeratedDataElement::getDataType() const
{
    return _dataType.get();
}

bool
HLAAbstractEnumeratedDataElement::setDataType(const HLADataType* dataType)
{
    const HLAEnumeratedDataType* enumeratedDataType = dataType->toEnumeratedDataType();
    if (!enumeratedDataType) {
        SG_LOG(SG_NETWORK, SG_WARN, "HLAEnumeratedDataType: unable to set data type!");
        return false;
    }
    setDataType(enumeratedDataType);
    return true;
}

void
HLAAbstractEnumeratedDataElement::setDataType(const HLAEnumeratedDataType* dataType)
{
    _dataType = dataType;
}

const HLABasicDataType*
HLAAbstractEnumeratedDataElement::getRepresentationDataType() const
{
    if (!_dataType.valid())
        return 0;
    return _dataType->getRepresentation();
}

std::string
HLAAbstractEnumeratedDataElement::getStringRepresentation() const
{
    if (!_dataType.valid())
        return std::string();
    return _dataType->getString(getEnumeratorIndex());
}

bool
HLAAbstractEnumeratedDataElement::setStringRepresentation(const std::string& name)
{
    if (!_dataType.valid())
        return false;
    unsigned index = _dataType->getIndex(name);
    if (_dataType->getNumEnumerators() <= index)
        return false;
    setEnumeratorIndex(index);
    return true;
}


HLAEnumeratedDataElement::HLAEnumeratedDataElement(const HLAEnumeratedDataType* dataType) :
    HLAAbstractEnumeratedDataElement(dataType),
    _enumeratorIndex(~unsigned(0))
{
}

HLAEnumeratedDataElement::~HLAEnumeratedDataElement()
{
}

unsigned
HLAEnumeratedDataElement::getEnumeratorIndex() const
{
    return _enumeratorIndex;
}

void
HLAEnumeratedDataElement::setEnumeratorIndex(unsigned index)
{
    _enumeratorIndex = index;
}

}
