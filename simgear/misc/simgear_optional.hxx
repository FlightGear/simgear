// -*- coding: utf-8 -*-
//
// simgear_optional.hxx --- Mimic std::optional until we can use C++14
// Copyright (C) 2020  James Turner <james@flightgear.org>
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301  USA.


#pragma once

#include <simgear/structure/exception.hxx>

namespace simgear
{

/**
 * Inefficient version of std::optional. It requires a default-constructable,
 * copyable T type, unlike the real version.
 */

template <class T>
class optional
{
public:
    using value_type  = T;

    optional() = default;

    optional(const T& v) :
        _value(v),
        _haveValue(true)
    {}

    optional(const optional<T>& other) :
        _value(other._value),
        _haveValue(other._haveValue)
    {}

    optional<T>& operator=(const optional<T>& other)
    {
        _haveValue = other._haveValue;
        _value = other._value;
        return *this;
    }

    explicit operator bool() const
    {
        return _haveValue;
    }

    bool has_value() const
    {
        return _haveValue;
    }

    const T& value() const
    {
        if (!_haveValue) {
            throw sg_exception("No value in optional");
        }
        return _value;
    }

    T& value()
    {
        if (!_haveValue) {
            throw sg_exception("No value in optional");
        }
        return _value;
    }

    void reset()
    {
        _haveValue = false;
        _value = {};
    }
private:
    T _value = {};
    bool _haveValue = false;
};

} // of namespace simgear
