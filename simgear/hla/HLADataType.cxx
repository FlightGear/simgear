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

#include "HLADataType.hxx"

#include "HLADataElement.hxx"
#include "HLADataTypeVisitor.hxx"

namespace simgear {

HLADataType::HLADataType(const std::string& name, unsigned alignment) :
    _name(name),
    _alignment(1)
{
    setAlignment(alignment);
}

HLADataType::~HLADataType()
{
}

void
HLADataType::accept(HLADataTypeVisitor& visitor) const
{
    visitor.apply(*this);
}

const HLABasicDataType*
HLADataType::toBasicDataType() const
{
    return 0;
}

const HLAArrayDataType*
HLADataType::toArrayDataType() const
{
    return 0;
}

const HLAEnumeratedDataType*
HLADataType::toEnumeratedDataType() const
{
    return 0;
}

const HLAFixedRecordDataType*
HLADataType::toFixedRecordDataType() const
{
    return 0;
}

const HLAVariantRecordDataType*
HLADataType::toVariantRecordDataType() const
{
    return 0;
}

bool
HLADataType::recomputeAlignment()
{
    unsigned alignment = getAlignment();
    _recomputeAlignmentImplementation();
    return alignment != getAlignment();
}

void
HLADataType::releaseDataTypeReferences()
{
}

class HLADataType::_DataElementIndexVisitor : public HLADataTypeVisitor {
public:
    _DataElementIndexVisitor(HLADataElementIndex& index, const std::string& path, std::string::size_type offset) :
        _index(index),
        _path(path),
        _offset(offset),
        _success(false)
    { }
    virtual ~_DataElementIndexVisitor()
    { }

    virtual void apply(const HLADataType& dataType)
    {
        if (_path.size() == _offset)
            _success = true;
    }
    virtual void apply(const HLAArrayDataType& dataType)
    {
        if (_path.size() == _offset) {
            _success = true;
            return;
        }
        if (_path.size() < _offset) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Expected array subscript at the end of the path!");
            return;
        }
        if (_path[_offset] != '[') {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Expected array subscript at the end of the path!");
            return;
        }
        ++_offset;
        if (_path.size() <= _offset) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Expected closing array subscript at the end of the path!");
            return;
        }
        unsigned index = 0;
        bool closed = false;
        while (_offset <= _path.size()) {
            if (_path[_offset] == ']') {
                ++_offset;
                closed = true;
                break;
            }
            unsigned v = _path[_offset] - '0';
            // Error, no number
            if (10 <= v) {
                SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                       << "Array subscript is not a number!");
                return;
            }
            index *= 10;
            index += v;
            ++_offset;
        }
        if (!closed) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Expected closing array subscript at the end of the path!");
            return;
        }
        if (!dataType.getElementDataType()) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Undefined array element data type!");
            return;
        }
        _index.push_back(index);
        _success = dataType.getElementDataType()->getDataElementIndex(_index, _path, _offset);
    }

    virtual void apply(const HLAFixedRecordDataType& dataType)
    {
        if (_path.size() == _offset) {
            _success = true;
            return;
        }
        if (_path.size() < _offset) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Expected field name at the end of the path!");
            return;
        }
        if (_path[_offset] == '.')
            ++_offset;
        if (_path.size() <= _offset) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Expected field name at the end of the path!");
            return;
        }
        std::string::size_type len = std::min(_path.find_first_of("[.", _offset), _path.size()) - _offset;
        unsigned index = 0;
        while (index < dataType.getNumFields()) {
            if (_path.compare(_offset, len, dataType.getFieldName(index)) == 0)
                break;
            ++index;
        }
        if (dataType.getNumFields() <= index) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Field \"" << _path.substr(_offset, len) << "\" not found in fixed record data type \""
                   << dataType.getName() << "\"!");
            return;
        }
        if (!dataType.getFieldDataType(index)) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Undefined field data type in variant record data type \""
                   << dataType.getName() << "\" at field \"" << dataType.getFieldName(index) << "\"!");
            return;
        }
        _index.push_back(index);
        _success = dataType.getFieldDataType(index)->getDataElementIndex(_index, _path, _offset + len);
    }

    virtual void apply(const HLAVariantRecordDataType& dataType)
    {
        if (_path.size() == _offset) {
            _success = true;
            return;
        }
        if (_path.size() < _offset) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Expected alternative name at the end of the path!");
            return;
        }
        if (_path[_offset] == '.')
            ++_offset;
        if (_path.size() <= _offset) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Expected alternative name at the end of the path!");
            return;
        }
        std::string::size_type len = std::min(_path.find_first_of("[.", _offset), _path.size()) - _offset;
        unsigned index = 0;
        while (index < dataType.getNumAlternatives()) {
            if (_path.compare(_offset, len, dataType.getAlternativeName(index)) == 0)
                break;
            ++index;
        }
        if (dataType.getNumAlternatives() <= index) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Alternative \"" << _path.substr(_offset, len) << "\" not found in variant record data type \""
                   << dataType.getName() << "\"!");
            return;
        }
        if (!dataType.getAlternativeDataType(index)) {
            SG_LOG(SG_NETWORK, SG_ALERT, "HLADataType: faild to parse data element index \"" << _path << "\":\n"
                   << "Undefined alternative data type in variant record data type \""
                   << dataType.getName() << "\" at alternative \"" << dataType.getAlternativeName(index) << "\"!");
            return;
        }
        _index.push_back(index);
        _success = dataType.getAlternativeDataType(index)->getDataElementIndex(_index, _path, _offset + len);
    }

    HLADataElementIndex& _index;
    const std::string& _path;
    std::string::size_type _offset;
    bool _success;
};

bool
HLADataType::getDataElementIndex(HLADataElementIndex& index, const std::string& path, std::string::size_type offset) const
{
    _DataElementIndexVisitor visitor(index, path, offset);
    accept(visitor);
    return visitor._success;
}

void
HLADataType::setAlignment(unsigned alignment)
{
    /// FIXME: more checks
    if (alignment == 0)
        _alignment = 1;
    else
        _alignment = alignment;
}

void
HLADataType::_recomputeAlignmentImplementation()
{
}

}
