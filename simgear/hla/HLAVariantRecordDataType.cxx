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

#include <algorithm>

#include <simgear/compiler.h>

#include "HLAVariantRecordDataType.hxx"
#include "HLADataTypeVisitor.hxx"
#include "HLAVariantRecordDataElement.hxx"

namespace simgear {

HLAVariantRecordDataType::HLAVariantRecordDataType(const std::string& name) :
    HLADataType(name)
{
}

HLAVariantRecordDataType::~HLAVariantRecordDataType()
{
}

void
HLAVariantRecordDataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLAVariantRecordDataType*
HLAVariantRecordDataType::toVariantRecordDataType() const
{
    return this;
}

void
HLAVariantRecordDataType::releaseDataTypeReferences()
{
    for (AlternativeList::iterator i = _alternativeList.begin(); i != _alternativeList.end(); ++i)
        i->_dataType = 0;
}

bool
HLAVariantRecordDataType::decode(HLADecodeStream& stream, HLAAbstractVariantRecordDataElement& value) const
{
    if (!stream.alignOffsetForSize(getAlignment()))
        return false;
    if (!_enumeratedDataType.valid())
        return false;
    unsigned index = ~0u;
    if (!_enumeratedDataType->decode(stream, index))
        return false;
    if (!value.setAlternativeIndex(index))
        return false;
    if (!value.decodeAlternative(stream))
        return false;
    return true;
}

bool
HLAVariantRecordDataType::encode(HLAEncodeStream& stream, const HLAAbstractVariantRecordDataElement& value) const
{
    if (!stream.alignOffsetForSize(getAlignment()))
        return false;
    if (!_enumeratedDataType.valid())
        return false;
    unsigned index = value.getAlternativeIndex();
    if (!_enumeratedDataType->encode(stream, index))
        return false;
    if (!value.encodeAlternative(stream))
        return false;
    return true;
}

void
HLAVariantRecordDataType::setEnumeratedDataType(HLAEnumeratedDataType* dataType)
{
    _enumeratedDataType = dataType;
}

bool
HLAVariantRecordDataType::addAlternative(const std::string& name, const std::string& enumerator,
                                         const HLADataType* dataType, const std::string& semantics)
{
    if (!_enumeratedDataType.valid())
        return false;
    unsigned index = _enumeratedDataType->getIndex(enumerator);
    if (_enumeratedDataType->getNumEnumerators() <= index)
        return false;
    _alternativeList.resize(_enumeratedDataType->getNumEnumerators());
    _alternativeList[index]._name = name;
    _alternativeList[index]._dataType = dataType;
    _alternativeList[index]._semantics = semantics;
    return true;
}

void
HLAVariantRecordDataType::_recomputeAlignmentImplementation()
{
    unsigned alignment = 1;
    if (const HLADataType* dataType = getEnumeratedDataType())
        alignment = std::max(alignment, dataType->getAlignment());
    for (unsigned i = 0; i < getNumAlternatives(); ++i) {
        if (const HLADataType* dataType = getAlternativeDataType(i))
            alignment = std::max(alignment, dataType->getAlignment());
    }
    setAlignment(alignment);
}

} // namespace simgear
