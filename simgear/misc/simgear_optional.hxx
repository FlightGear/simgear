// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2020 James Turner <james@flightgear.org>

/**
 * @file
 * @brief  Mimic std::optional until we can use C++14
 */

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
