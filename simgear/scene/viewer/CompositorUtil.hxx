// Copyright (C) 2018  Fernando García Liñán <fernandogarcialinan@gmail.com>
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef SG_COMPOSITOR_UTIL_HXX
#define SG_COMPOSITOR_UTIL_HXX

#include <unordered_map>

namespace simgear {
namespace compositor {

/**
 * Lookup table that ties a string property value to a type that cannot be
 * represented in the property tree. Useful for OSG or OpenGL enums.
 */
template<class T>
using PropStringMap = std::unordered_map<std::string, T>;

template <class T>
bool findPropString(const std::string &str,
                    T &value,
                    const PropStringMap<T> &map)
{
    auto itr = map.find(str);
    if (itr == map.end())
        return false;

    value = itr->second;
    return true;
}

template <class T>
bool findPropString(const SGPropertyNode *parent,
                    const std::string &child_name,
                    T &value,
                    const PropStringMap<T> &map)
{
    const SGPropertyNode *child = parent->getNode(child_name);
    if (child) {
        if (findPropString<T>(child->getStringValue(), value, map))
            return true;
    }
    return false;
}

/**
 * Check if node should be enabled based on a condition tag.
 * If no condition tag is found inside or it is malformed, it will be enabled.
 */
bool checkConditional(const SGPropertyNode *node);

const SGPropertyNode *getPropertyNode(const SGPropertyNode *prop);
const SGPropertyNode *getPropertyChild(const SGPropertyNode *prop,
                                       const char *name);

} // namespace compositor
} // namespace simgear

#endif /* SG_COMPOSITOR_UTIL_HXX */
