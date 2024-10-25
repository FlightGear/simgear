// Copyright (C) 2018 - 2024 Fernando García Liñán
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <stdexcept>

#include "SGLight.hxx"

#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/PolygonMode>
#include <osg/ShapeDrawable>
#include <osg/Switch>

#include <simgear/math/SGMath.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/scene/util/color_space.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/props/props_io.hxx>

#include "animation.hxx"

class SGLightDebugListener : public SGPropertyChangeListener {
public:
    SGLightDebugListener(osg::Switch *sw) : _sw(sw) {}
    void valueChanged(SGPropertyNode* node) override
    {
        _sw->setValue(0, node->getBoolValue());
    }
private:
    osg::ref_ptr<osg::Switch> _sw;
};

void
SGLight::UpdateCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
    auto light = dynamic_cast<SGLight*>(node);
    assert(light);
    // since we're in an update callback, it's safe to evaluate conditions
    // and expressions here.
    light->_dim_factor = light->_dim_factor_value->get_value();
    light->_ambient = toOsg(light->_ambient_value->get_value() * light->_dim_factor);
    light->_diffuse = toOsg(light->_diffuse_value->get_value() * light->_dim_factor);
    light->_specular = toOsg(light->_specular_value->get_value() * light->_dim_factor);
    light->_range = light->_range_value->get_value();
    light->_constant_attenuation = light->_constant_attenuation_value->get_value();
    light->_linear_attenuation = light->_linear_attenuation_value->get_value();
    light->_quadratic_attenuation = light->_quadratic_attenuation_value->get_value();
    light->_spot_exponent = light->_spot_exponent_value->get_value();
    light->_spot_cutoff = light->_spot_cutoff_value->get_value();
    light->_intensity = light->_intensity_value->get_value();
    // The color value is stored in sRGB space, but the renderer expects it
    // to be in linear RGB.
    light->_color = toOsg(simgear::eotf_inverse_sRGB(light->_color_value->get_value()));
}

class SGLightConfigListener : public SGPropertyChangeListener {
public:
    SGLightConfigListener(SGLight* light) : _light(light) {}

    void valueChanged(SGPropertyNode* node) override
    {
        // walk up to find the light node
        while (node->getNameString() != "light" && node->getParent()) {
            node = node->getParent();
        }
        _light->configure(node);
    }
private:
    SGLight* _light;
};

SGLight::SGLight(const bool legacy) :
    _legacyPropertyNames(legacy)
{
    // Do not let OSG cull lights by default, we usually leave that job to
    // other algorithms, like clustered shading.
    setCullingActive(false);
}

SGLight::SGLight(const SGLight& l, const osg::CopyOp& copyop) :
    osg::Node(l, copyop),
    _legacyPropertyNames(l._legacyPropertyNames),
    _type(l._type),
    _priority(l._priority),
    _range(l._range),
    _ambient(l._ambient),
    _diffuse(l._diffuse),
    _specular(l._specular),
    _constant_attenuation(l._constant_attenuation),
    _linear_attenuation(l._linear_attenuation),
    _quadratic_attenuation(l._quadratic_attenuation),
    _spot_exponent(l._spot_exponent),
    _spot_cutoff(l._spot_cutoff)
{
    _range_value = l._range_value;
    _dim_factor_value = l._dim_factor_value;
    _ambient_value = l._ambient_value;
    _diffuse_value = l._diffuse_value;
    _specular_value = l._specular_value;
    _constant_attenuation_value = l._constant_attenuation_value;
    _linear_attenuation_value = l._linear_attenuation_value;
    _quadratic_attenuation_value = l._quadratic_attenuation_value;
}

osg::ref_ptr<osg::Node>
SGLight::appendLight(const SGPropertyNode* configNode,
                     SGPropertyNode* modelRoot,
                     bool legacy)
{
    //-- create return variable
    osg::ref_ptr<osg::MatrixTransform> align = new osg::MatrixTransform;

    SGLight* light = new SGLight{legacy};
    light->_transform = align;
    light->_modelRoot = modelRoot;

    align->addChild(light);
    //-- copy config to prop tree --
    const std::string propPath {"/scenery/lights"};
    const std::string propName {"light"};
    SGPropertyNode_ptr _pConfig = simgear::getPropertyRoot()->getNode(propPath, true);
    _pConfig->setAttribute(SGPropertyNode::VALUE_CHANGED_DOWN, true);
    _pConfig = _pConfig->addChild(propName);

    copyProperties(configNode, _pConfig);

    //-- configure light and add listener to conf in prop tree
    _pConfig->addChangeListener(new SGLightConfigListener(light), true);
    light->setUpdateCallback(new SGLight::UpdateCallback());
    light->configure(configNode);

    //-- debug visualisation --
    osg::Shape *debug_shape = nullptr;
    if (light->getType() == SGLight::Type::POINT) {
        debug_shape = new osg::Sphere(osg::Vec3f(0.0f, 0.0f, 0.0f), light->getRange());
    } else if (light->getType() == SGLight::Type::SPOT) {
        debug_shape = new osg::Cone(
            // Origin of the cone is at its center of mass
            osg::Vec3f(0.0f, 0.0f, -0.75f * light->getRange()),
            tan(light->getSpotCutoff() * SG_DEGREES_TO_RADIANS) * light->getRange(),
            light->getRange());
    } else {
        throw std::domain_error("Unsupported SGLight::Type");
    }

    osg::ShapeDrawable *debug_drawable = new osg::ShapeDrawable(debug_shape);
    debug_drawable->setColor(
        osg::Vec4(configNode->getFloatValue("debug-color/r", 1.0f),
                  configNode->getFloatValue("debug-color/g", 0.0f),
                  configNode->getFloatValue("debug-color/b", 0.0f),
                  configNode->getFloatValue("debug-color/a", 1.0f)));
    osg::Geode *debug_geode = new osg::Geode;
    debug_geode->addDrawable(debug_drawable);

    osg::StateSet *debug_ss = debug_drawable->getOrCreateStateSet();
    debug_ss->setAttributeAndModes(
        new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE),
        osg::StateAttribute::ON);

    osg::Switch *debug_switch = new osg::Switch;
    debug_switch->addChild(debug_geode);
    simgear::getPropertyRoot()->getNode("/sim/debug/show-light-volumes", true)->
        addChangeListener(new SGLightDebugListener(debug_switch), true);
    align->addChild(debug_switch);
    //-- end debug visualisation --

    SGConstPropertyNode_ptr p;
    if ((p = configNode->getNode("name")) != NULL)
        align->setName(p->getStringValue());
    else
        align->setName("light");

    return align;
}

SGLight::~SGLight() = default;

simgear::Value_ptr
SGLight::buildValue(const SGPropertyNode* node, double defaultVal)
{
    if (!node) {
        return new simgear::Value(defaultVal);
    }
    simgear::Value_ptr expr = new simgear::Value(*_modelRoot, *(const_cast<SGPropertyNode*>(node)), defaultVal);
    return expr;
}

simgear::RGBColorValue_ptr
SGLight::buildRGBColorValue(const SGPropertyNode *node, const osg::Vec3f &defaultVal)
{
    if (!node) {
        // Node does not exist: use a fixed color
        return new simgear::RGBColorValue(toSG(defaultVal));
    }
    // Node exists: parse every color component and set missing components to 0
    return new simgear::RGBColorValue(*_modelRoot, *(const_cast<SGPropertyNode*>(node)));
}

simgear::RGBAColorValue_ptr
SGLight::buildRGBAColorValue(const SGPropertyNode *node, const osg::Vec4f &defaultVal)
{
    if (!node) {
        // Node does not exist: use a fixed color
        return new simgear::RGBAColorValue(toSG(defaultVal));
    }
    // Node exists: parse every color component and set missing components to 0
    return new simgear::RGBAColorValue(*_modelRoot, *(const_cast<SGPropertyNode*>(node)));
}

void
SGLight::configure(const SGPropertyNode* configNode)
{
    SGConstPropertyNode_ptr p;
    if ((p = configNode->getNode(_legacyPropertyNames ? "light-type" : "type")) != NULL) {
        std::string type = p->getStringValue();
        if (type == "point") {
            _type = Type::POINT;
        } else if (type == "spot") {
            _type = Type::SPOT;
        } else {
            SG_LOG(SG_GENERAL, SG_ALERT, "SGLight: Ignoring unknown light type '" << type << "'");
        }
    }

    std::string priority = configNode->getStringValue("priority", "low");
    if (priority == "low") {
        _priority = Priority::LOW;
    } else if (priority == "medium") {
        _priority = Priority::MEDIUM;
    } else if (priority == "high") {
        _priority = Priority::HIGH;
    } else {
        SG_LOG(SG_GENERAL, SG_ALERT, "SGLight: Unknown priority '" << priority << "'. Using LOW priority");
        _priority = Priority::LOW;
    }

    _dim_factor_value = buildValue(configNode->getChild("dim-factor"), 1.0f);
    _range_value = buildValue(configNode->getChild(_legacyPropertyNames ? "far-m" : "range-m"), 1.0f);
    _ambient_value = buildRGBAColorValue(configNode->getChild("ambient"), osg::Vec4f(0.05f, 0.05f, 0.05f, 1.0f));
    _diffuse_value = buildRGBAColorValue(configNode->getChild("diffuse"), osg::Vec4f(0.8f, 0.8f, 0.8f, 1.0f));
    _specular_value = buildRGBAColorValue(configNode->getChild("specular"), osg::Vec4f(0.05f, 0.05f, 0.05f, 1.0f));
    _constant_attenuation_value = buildValue(configNode->getNode("attenuation/c"), 1.0f);
    _linear_attenuation_value = buildValue(configNode->getNode("attenuation/l"), 0.0f);
    _quadratic_attenuation_value = buildValue(configNode->getNode("attenuation/q"), 0.0f);
    _spot_exponent_value = buildValue(configNode->getNode(_legacyPropertyNames ? "exponent" : "spot-exponent"), 0.0f);
    _spot_cutoff_value = buildValue(configNode->getNode(_legacyPropertyNames ? "cutoff" : "spot-cutoff"), 180.0f);

    _color_value = buildRGBColorValue(configNode->getChild("color"), osg::Vec3f(1.0f, 1.0f, 1.0f));
    _intensity_value = buildValue(configNode->getNode("intensity"), 1.0f);

    osg::Matrixf t;
    osg::Vec3f pos;
    if (const SGPropertyNode* posNode = configNode->getNode("position")) {
        // use the legacy node names for x,y,z when in legacy mode and at least one is specified as this is the most compatible
        // because sometimes modellers omit any node that has a zero value as a shortcut.
        if (_legacyPropertyNames && (posNode->hasValue("x") || posNode->hasValue("y") || posNode->hasValue("z")) ) {
            pos = osg::Vec3(posNode->getFloatValue("x"),
                          posNode->getFloatValue("y"),
                          posNode->getFloatValue("z"));
            t.makeTranslate(pos);
        } else {
            pos = osg::Vec3(posNode->getFloatValue("x-m"),
                          posNode->getFloatValue("y-m"),
                          posNode->getFloatValue("z-m"));
            t.makeTranslate(pos);
        }
    }
    osg::Matrixf r;
    if (const SGPropertyNode *dirNode = configNode->getNode("direction")) {
        if (dirNode->hasValue("pitch-deg")) {
            r.makeRotate(
                dirNode->getFloatValue("pitch-deg")*SG_DEGREES_TO_RADIANS,
                osg::Vec3f(0.0f, 1.0f, 0.0f),
                dirNode->getFloatValue("roll-deg")*SG_DEGREES_TO_RADIANS,
                osg::Vec3f(1.0f, 0.0f, 0.0f),
                dirNode->getFloatValue("heading-deg")*SG_DEGREES_TO_RADIANS,
                osg::Vec3f(0.0f, 0.0f, 1.0f));
        } else if (dirNode->hasValue("lookat-x-m")) {
            osg::Vec3f lookAt(dirNode->getFloatValue("lookat-x-m"),
                             dirNode->getFloatValue("lookat-y-m"),
                             dirNode->getFloatValue("lookat-z-m"));
            osg::Vec3f dir = lookAt - pos;
            r.makeRotate(osg::Vec3(0.0f, 0.0f, -1.0f), dir);
        } else if (dirNode->hasValue("pointing_x")) { // ALS compatible
            r.makeRotate(osg::Vec3(0.0f, 0.0f, -1.0f),
                         osg::Vec3(-dirNode->getFloatValue("pointing_x"),
                                   -dirNode->getFloatValue("pointing_y"),
                                   -dirNode->getFloatValue("pointing_z")));
        } else {
            r.makeRotate(osg::Vec3(0.0f, 0.0f, -1.0f),
                         osg::Vec3(dirNode->getFloatValue("x"),
                                   dirNode->getFloatValue("y"),
                                   dirNode->getFloatValue("z")));
        }
    }
    if (_transform) {
        _transform->setMatrix(r * t);
    }
}
