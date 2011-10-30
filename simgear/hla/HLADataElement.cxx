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

#include "HLADataElement.hxx"

#include <simgear/debug/logstream.hxx>

#include "HLADataElementVisitor.hxx"

namespace simgear {

HLADataElement::PathElement::Data::~Data()
{
}

const HLADataElement::PathElement::FieldData*
HLADataElement::PathElement::Data::toFieldData() const
{
    return 0;
}

const HLADataElement::PathElement::IndexData*
HLADataElement::PathElement::Data::toIndexData() const
{
    return 0;
}

HLADataElement::PathElement::FieldData::FieldData(const std::string& name) :
    _name(name)
{
}

HLADataElement::PathElement::FieldData::~FieldData()
{
}

const HLADataElement::PathElement::FieldData*
HLADataElement::PathElement::FieldData::toFieldData() const
{
    return this;
}

bool
HLADataElement::PathElement::FieldData::less(const Data* data) const
{
    const FieldData* fieldData = data->toFieldData();
    // IndexData is allways smaller than FieldData
    if (!fieldData)
        return false;
    return _name < fieldData->_name;
}

bool
HLADataElement::PathElement::FieldData::equal(const Data* data) const
{
    const FieldData* fieldData = data->toFieldData();
    // IndexData is allways smaller than FieldData
    if (!fieldData)
        return false;
    return _name == fieldData->_name;
}

void
HLADataElement::PathElement::FieldData::append(std::string& s) const
{
    s.append(1, std::string::value_type('.'));
    s.append(_name);
}

HLADataElement::PathElement::IndexData::IndexData(unsigned index) :
    _index(index)
{
}

HLADataElement::PathElement::IndexData::~IndexData()
{
}

const HLADataElement::PathElement::IndexData*
HLADataElement::PathElement::IndexData::toIndexData() const
{
    return this;
}

bool
HLADataElement::PathElement::IndexData::less(const Data* data) const
{
    const IndexData* indexData = data->toIndexData();
    // IndexData is allways smaller than FieldData
    if (!indexData)
        return true;
    return _index < indexData->_index;
}

bool
HLADataElement::PathElement::IndexData::equal(const Data* data) const
{
    const IndexData* indexData = data->toIndexData();
    // IndexData is allways smaller than FieldData
    if (!indexData)
        return false;
    return _index == indexData->_index;
}

void
HLADataElement::PathElement::IndexData::append(std::string& s) const
{
    s.append(1, std::string::value_type('['));
    unsigned value = _index;
    do {
        s.append(1, std::string::value_type('0' + value % 10));
    } while (value /= 10);
    s.append(1, std::string::value_type(']'));
}

HLADataElement::~HLADataElement()
{
}

void
HLADataElement::accept(HLADataElementVisitor& visitor)
{
    visitor.apply(*this);
}

void
HLADataElement::accept(HLAConstDataElementVisitor& visitor) const
{
    visitor.apply(*this);
}

std::string
HLADataElement::toString(const Path& path)
{
    std::string s;
    for (Path::const_iterator i = path.begin(); i != path.end(); ++i)
        i->append(s);
    return s;
}

HLADataElement::AttributePathPair
HLADataElement::toAttributePathPair(const std::string& s)
{
    Path path;
    // Skip the initial attribute name if given
    std::string::size_type i = s.find_first_of("[.");
    std::string attribute = s.substr(0, i);
    while (i < s.size()) {
        if (s[i] == '[') {
            ++i;
            unsigned index = 0;
            while (i < s.size()) {
                if (s[i] == ']')
                    break;
                unsigned v = s[i] - '0';
                // Error, no number
                if (10 <= v) {
                    SG_LOG(SG_NETWORK, SG_WARN, "HLADataElement: invalid character in array subscript for \""
                           << s << "\" at \"" << attribute << toString(path) << "\"!");
                    return AttributePathPair();
                }
                index *= 10;
                index += v;
                ++i;
            }
            path.push_back(index);
            ++i;
            continue;
        }
        if (s[i] == '.') {
            // Error, . cannot be last
            if (s.size() <= ++i) {
                SG_LOG(SG_NETWORK, SG_WARN, "HLADataElement: invalid terminating '.' for \""
                       << s << "\"!");
                return AttributePathPair();
            }
            std::string::size_type e = s.find_first_of("[.", i);
            path.push_back(s.substr(i, e - i));
            i = e;
            continue;
        }
    }

    return AttributePathPair(attribute, path);
}

}
