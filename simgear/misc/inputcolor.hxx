// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Fernando García Liñán

#pragma once

#include "inputvalue.hxx"

namespace simgear {

using RGBColorValue_ptr  = SGSharedPtr<class RGBColorValue>;
using RGBAColorValue_ptr = SGSharedPtr<class RGBAColorValue>;

/**
 * @brief An aggregation of three values that make up an RGB color.
 */
class RGBColorValue : public SGReferenced {
private:
    Value_ptr _r;
    Value_ptr _g;
    Value_ptr _b;
public:
    RGBColorValue(SGPropertyNode& prop_root,
                  SGPropertyNode& cfg,
                  const SGVec3f& value = {0.0f, 0.0f, 0.0f});
    RGBColorValue(const SGVec3f& value = {0.0f, 0.0f, 0.0f});

    SGVec3f get_value() const;
};

/**
 * @brief An aggregation of four values that make up an RGBA color.
 */
class RGBAColorValue : public SGReferenced {
private:
    Value_ptr _r;
    Value_ptr _g;
    Value_ptr _b;
    Value_ptr _a;
public:
    RGBAColorValue(SGPropertyNode& prop_root,
                   SGPropertyNode& cfg,
                   const SGVec4f& value = {0.0f, 0.0f, 0.0f, 1.0f});
    RGBAColorValue(const SGVec4f& value = {0.0f, 0.0f, 0.0f, 1.0f});

    SGVec4f get_value() const;
};

} // namespace simgear
