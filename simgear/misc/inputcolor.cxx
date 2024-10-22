// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Fernando García Liñán

#include "inputcolor.hxx"

using namespace simgear;

const std::vector<std::string> red_names   = {"r", "red"};
const std::vector<std::string> green_names = {"g", "green"};
const std::vector<std::string> blue_names  = {"b", "blue"};
const std::vector<std::string> alpha_names = {"a", "alpha"};

namespace {
Value_ptr
parseColorComponent(SGPropertyNode &prop_root,
                    SGPropertyNode &cfg,
                    const std::vector<std::string> &component_names,
                    float component_value)
{
    for (const auto &name : component_names) {
        SGPropertyNode_ptr component_node = cfg.getChild(name);
        if (component_node) {
            return new Value(prop_root, *component_node, component_value);
        }
    }
    return new Value(component_value);
}
} // anonymous namespace

//------------------------------------------------------------------------------

RGBColorValue::RGBColorValue(SGPropertyNode &prop_root,
                             SGPropertyNode &cfg,
                             const SGVec3f &value)
{
    _r = parseColorComponent(prop_root, cfg, red_names,   value.x());
    _g = parseColorComponent(prop_root, cfg, green_names, value.y());
    _b = parseColorComponent(prop_root, cfg, blue_names,  value.z());
}

RGBColorValue::RGBColorValue(const SGVec3f& value)
{
    _r = new Value(value.x());
    _g = new Value(value.y());
    _b = new Value(value.z());
}

SGVec3f
RGBColorValue::get_value() const
{
    return {
        static_cast<float>(_r->get_value()),
        static_cast<float>(_g->get_value()),
        static_cast<float>(_b->get_value())
    };
}

//------------------------------------------------------------------------------

RGBAColorValue::RGBAColorValue(SGPropertyNode &prop_root,
                               SGPropertyNode &cfg,
                               const SGVec4f &value)
{
    _r = parseColorComponent(prop_root, cfg, red_names,   value.x());
    _g = parseColorComponent(prop_root, cfg, green_names, value.y());
    _b = parseColorComponent(prop_root, cfg, blue_names,  value.z());
    _a = parseColorComponent(prop_root, cfg, alpha_names, value.w());
}

RGBAColorValue::RGBAColorValue(const SGVec4f& value)
{
    _r = new Value(value.x());
    _g = new Value(value.y());
    _b = new Value(value.z());
    _a = new Value(value.w());
}

SGVec4f
RGBAColorValue::get_value() const
{
    return {
        static_cast<float>(_r->get_value()),
        static_cast<float>(_g->get_value()),
        static_cast<float>(_b->get_value()),
        static_cast<float>(_a->get_value())
    };
}
