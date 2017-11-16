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

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#include "HLAEnumeratedDataType.hxx"
#include "HLADataTypeVisitor.hxx"

namespace simgear {

template<typename DataType, typename T>
class HLAEnumeratedDataType::Map : public HLAEnumeratedDataType::AbstractMap {
public:
    typedef typename std::pair<std::string,T> RepresentationPair;
    typedef typename std::map<T, unsigned> RepresentationIndexMap;
    typedef typename std::vector<RepresentationPair> IndexRepresentationMap;
    typedef typename std::map<std::string, unsigned> StringIndexMap;

    Map(const DataType* dataType) :
        _dataType(dataType)
    { }
    virtual bool encode(HLAEncodeStream& stream, unsigned index) const
    {
        T value = _otherRepresentation;
        if (index < _indexRepresentationMap.size())
            value = _indexRepresentationMap[index].second;
        if (!_dataType->encode(stream, value))
            return false;
        return true;
    }
    virtual bool decode(HLADecodeStream& stream, unsigned& index) const
    {
        T value;
        if (!_dataType->decode(stream, value))
            return false;
        typename RepresentationIndexMap::const_iterator i;
        i = _representationIndexMap.find(value);
        if (i == _representationIndexMap.end())
            index = _indexRepresentationMap.size();
        else
            index = i->second;
        return true;
    }
    virtual const HLABasicDataType* getDataType() const
    { return _dataType.get(); }

    virtual bool add(const std::string& name, const std::string& number)
    {
        std::stringstream ss(number);
        T value;
        ss >> value;
        if (ss.fail())
            return false;
        if (_representationIndexMap.find(value) != _representationIndexMap.end())
            return false;
        if (_stringIndexMap.find(name) != _stringIndexMap.end())
            return false;
        _stringIndexMap[name] = _indexRepresentationMap.size();
        _representationIndexMap[value] = _indexRepresentationMap.size();
        _indexRepresentationMap.push_back(RepresentationPair(name, value));
        return true;
    }
    virtual std::string getString(unsigned index) const
    {
        if (_indexRepresentationMap.size() <= index)
            return std::string();
        return _indexRepresentationMap[index].first;
    }
    virtual unsigned getIndex(const std::string& name) const
    {
        typename StringIndexMap::const_iterator i = _stringIndexMap.find(name);
        if (i == _stringIndexMap.end())
            return ~unsigned(0);
        return i->second;
    }
    virtual unsigned getNumEnumerators() const
    { return _indexRepresentationMap.size(); }

private:
    IndexRepresentationMap _indexRepresentationMap;
    StringIndexMap _stringIndexMap;
    RepresentationIndexMap _representationIndexMap;

    T _otherRepresentation;
    SGSharedPtr<const DataType> _dataType;
};

class HLAEnumeratedDataType::RepresentationVisitor : public HLADataTypeVisitor {
public:
    virtual void apply(const HLAInt8DataType& dataType)
    { _map = new Map<HLAInt8DataType, int8_t>(&dataType); }
    virtual void apply(const HLAUInt8DataType& dataType)
    { _map = new Map<HLAUInt8DataType, uint8_t>(&dataType); }
    virtual void apply(const HLAInt16DataType& dataType)
    { _map = new Map<HLAInt16DataType, int16_t>(&dataType); }
    virtual void apply(const HLAUInt16DataType& dataType)
    { _map = new Map<HLAUInt16DataType, uint16_t>(&dataType); }
    virtual void apply(const HLAInt32DataType& dataType)
    { _map = new Map<HLAInt32DataType, int32_t>(&dataType); }
    virtual void apply(const HLAUInt32DataType& dataType)
    { _map = new Map<HLAUInt32DataType, uint32_t>(&dataType); }
    virtual void apply(const HLAInt64DataType& dataType)
    { _map = new Map<HLAInt64DataType, int64_t>(&dataType); }
    virtual void apply(const HLAUInt64DataType& dataType)
    { _map = new Map<HLAUInt64DataType, uint64_t>(&dataType); }

    SGSharedPtr<AbstractMap> _map;
};

HLAEnumeratedDataType::HLAEnumeratedDataType(const std::string& name) :
    HLADataType(name)
{
}

HLAEnumeratedDataType::~HLAEnumeratedDataType()
{
}

void
HLAEnumeratedDataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLAEnumeratedDataType*
HLAEnumeratedDataType::toEnumeratedDataType() const
{
    return this;
}

bool
HLAEnumeratedDataType::decode(HLADecodeStream& stream, unsigned& index) const
{
    if (!_map.valid())
        return false;

    if (!_map->decode(stream, index))
        return false;
    return true;
}

bool
HLAEnumeratedDataType::encode(HLAEncodeStream& stream, unsigned index) const
{
    if (!_map.valid())
        return false;

    return _map->encode(stream, index);
}

void
HLAEnumeratedDataType::setRepresentation(HLABasicDataType* representation)
{
    if (!representation)
        return;

    RepresentationVisitor representationVisitor;
    representation->accept(representationVisitor);
    _map.swap(representationVisitor._map);
}

void
HLAEnumeratedDataType::_recomputeAlignmentImplementation()
{
    unsigned alignment = 1;
    if (const HLADataType* dataType = getRepresentation())
        alignment = std::max(alignment, dataType->getAlignment());
    setAlignment(alignment);
}

} // namespace simgear

