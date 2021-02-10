// Copyright (C) 2018  Fernando García Liñán <fernandogarcialinan@gmail.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include <stdexcept>

#include "SGLight.hxx"

#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/PolygonMode>
#include <osg/ShapeDrawable>
#include <osg/Switch>

#include <simgear/math/SGMath.hxx>
#include <simgear/scene/tgdb/userdata.hxx>
#include <simgear/props/props_io.hxx>

#include "animation.hxx"

class SGLightDebugListener : public SGPropertyChangeListener {
public:
    SGLightDebugListener(osg::Switch *sw) : _sw(sw) {}
    virtual void valueChanged(SGPropertyNode *node) {
        _sw->setValue(0, node->getBoolValue());
    }
private:
    osg::ref_ptr<osg::Switch> _sw;
};

class SGLightConfigListener : public SGPropertyChangeListener {
public:
    SGLightConfigListener(SGLight *light) : _light(light) {}

    virtual void valueChanged(SGPropertyNode *node) {
        while (strcmp(node->getName(), "light") && node->getParent()) {
            node = node->getParent();
        }
        _light->configure(node);
        auto *cb = _light->getUpdateCallback();
        if (cb != nullptr) {
            dynamic_cast<SGLight::UpdateCallback*>(cb)->configure(_light->getAmbient(), _light->getDiffuse(), _light->getSpecular());
        }
    }
private:
    SGLight *_light;
};

SGLight::SGLight(const SGLight& l, const osg::CopyOp& copyop) :
    osg::Node(l, copyop),
    _type(l._type),
    _range(l._range),
    _ambient(l._ambient),
    _diffuse(l._diffuse),
    _specular(l._specular),
    _constant_attenuation(l._constant_attenuation),
    _linear_attenuation(l._linear_attenuation),
    _quadratic_attenuation(l._quadratic_attenuation),
    _spot_exponent(l._spot_exponent),
    _spot_cutoff(l._spot_cutoff)
{}

osg::Node *
SGLight::appendLight(const SGPropertyNode *configNode,
                     SGPropertyNode *modelRoot,
                     const osgDB::Options *options)
{
    //-- create return variable
    osg::ref_ptr<osg::MatrixTransform> align = new osg::MatrixTransform;

    SGLight *light = new SGLight(align);
    align->addChild(light);

    //-- copy config to prop tree --
    const std::string propPath {"/scenery/lights"};
    const std::string propName {"light"};
    SGPropertyNode_ptr _pConfig = simgear::getPropertyRoot()->getNode(propPath, true)
        ->addChild(propName);
    
    copyProperties(configNode, _pConfig);
    
    //-- setup osg::NodeCallback --
    const SGPropertyNode *dim_factor = configNode->getChild("dim-factor");
    if (dim_factor) {
        const SGExpressiond *expression =
            read_value(dim_factor, modelRoot, "", 0, 1);
        if (expression)
            light->setUpdateCallback(new SGLight::UpdateCallback(expression));
    }
    else {
        //need UpdateCallback to activate config changes at runtime
        light->setUpdateCallback(new SGLight::UpdateCallback());
    }
    
    //-- configure light and add listener to conf in prop tree
    light->configure(configNode);
    _pConfig->addChangeListener(new SGLightConfigListener(light), true);

    //-- debug visualisation --
    osg::Shape *debug_shape = nullptr;
    if (light->getType() == SGLight::Type::POINT) {
        debug_shape = new osg::Sphere(osg::Vec3(0, 0, 0), light->getRange());
    } else if (light->getType() == SGLight::Type::SPOT) {
        debug_shape = new osg::Cone(
            // Origin of the cone is at its center of mass
            osg::Vec3(0, 0, -0.75 * light->getRange()),
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
    debug_ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

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

SGLight::SGLight()
{
    // Default values taken from osg::Light
    // They don't matter anyway as they are overwritten by the XML config values

//TODO: can this be moved to initalizer in hxx?
    _ambient.set(0.05f, 0.05f, 0.05f, 1.0f);
    _diffuse.set(0.8f, 0.8f, 0.8f, 1.0f);
    _specular.set(0.05f, 0.05f, 0.05f, 1.0f);

    // Do not let OSG cull lights by default, we usually leave that job to
    // other algorithms, like clustered shading.
    setCullingActive(false);
}

SGLight::~SGLight()
{

}

void SGLight::configure(const SGPropertyNode *configNode)
{
    SGConstPropertyNode_ptr p;
    if((p = configNode->getNode("type")) != NULL) {
        std::string type = p->getStringValue();
        if (type == "point")
            setType(SGLight::Type::POINT);
        else if (type == "spot")
            setType(SGLight::Type::SPOT);
        else
            SG_LOG(SG_GENERAL, SG_ALERT, "ignoring unknown light type '" << type << "'");
    }

    std::string priority = configNode->getStringValue("priority", "low");
    if (priority == "high")
        setPriority(SGLight::HIGH);
    else if (priority == "medium")
        setPriority(SGLight::MEDIUM);

    setRange(configNode->getFloatValue("range-m"));

#define SGLIGHT_GET_COLOR_VALUE(n)                \
    osg::Vec4(configNode->getFloatValue(n "/r"),  \
              configNode->getFloatValue(n "/g"),  \
              configNode->getFloatValue(n "/b"),  \
              configNode->getFloatValue(n "/a"))
    setAmbient(SGLIGHT_GET_COLOR_VALUE("ambient"));
    setDiffuse(SGLIGHT_GET_COLOR_VALUE("diffuse"));
    setSpecular(SGLIGHT_GET_COLOR_VALUE("specular"));
#undef SGLIGHT_GET_COLOR_VALUE

    setConstantAttenuation(configNode->getFloatValue("attenuation/c"));
    setLinearAttenuation(configNode->getFloatValue("attenuation/l"));
    setQuadraticAttenuation(configNode->getFloatValue("attenuation/q"));

    setSpotExponent(configNode->getFloatValue("spot-exponent"));
    setSpotCutoff(configNode->getFloatValue("spot-cutoff"));

    osg::Matrix t;
    osg::Vec3 pos(configNode->getFloatValue("position/x-m"),
                  configNode->getFloatValue("position/y-m"),
                  configNode->getFloatValue("position/z-m"));
    t.makeTranslate(pos);

    osg::Matrix r;
    if (const SGPropertyNode *dirNode = configNode->getNode("direction")) {
        if (dirNode->hasValue("pitch-deg")) {
            r.makeRotate(
                dirNode->getFloatValue("pitch-deg")*SG_DEGREES_TO_RADIANS,
                osg::Vec3(0, 1, 0),
                dirNode->getFloatValue("roll-deg")*SG_DEGREES_TO_RADIANS,
                osg::Vec3(1, 0, 0),
                dirNode->getFloatValue("heading-deg")*SG_DEGREES_TO_RADIANS,
                osg::Vec3(0, 0, 1));
        } else if (dirNode->hasValue("lookat-x-m")) {
            osg::Vec3 lookAt(dirNode->getFloatValue("lookat-x-m"),
                             dirNode->getFloatValue("lookat-y-m"),
                             dirNode->getFloatValue("lookat-z-m"));
            osg::Vec3 dir = lookAt - pos;
            r.makeRotate(osg::Vec3(0, 0, -1), dir);
        } else {
            r.makeRotate(osg::Vec3(0, 0, -1),
                         osg::Vec3(dirNode->getFloatValue("x"),
                                   dirNode->getFloatValue("y"),
                                   dirNode->getFloatValue("z")));
        }
    }
    if (_transform) _transform->setMatrix(r * t);
}
