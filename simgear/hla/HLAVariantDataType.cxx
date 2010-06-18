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

#include "HLAVariantDataType.hxx"

#include "HLADataTypeVisitor.hxx"
#include "HLAVariantDataElement.hxx"

namespace simgear {

HLAVariantDataType::HLAVariantDataType(const std::string& name) :
    HLADataType(name)
{
}

HLAVariantDataType::~HLAVariantDataType()
{
}

void
HLAVariantDataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLAVariantDataType*
HLAVariantDataType::toVariantDataType() const
{
    return this;
}

bool
HLAVariantDataType::decode(HLADecodeStream& stream, HLAAbstractVariantDataElement& value) const
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
HLAVariantDataType::encode(HLAEncodeStream& stream, const HLAAbstractVariantDataElement& value) const
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
HLAVariantDataType::setEnumeratedDataType(HLAEnumeratedDataType* dataType)
{
    _enumeratedDataType = dataType;
}

bool
HLAVariantDataType::addAlternative(const std::string& name, const std::string& enumerator,
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
    setAlignment(SGMisc<unsigned>::max(getAlignment(), dataType->getAlignment()));
    return true;
}

} // namespace simgear
