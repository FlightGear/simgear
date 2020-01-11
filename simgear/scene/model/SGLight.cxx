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

class SGLightDebugListener : public SGPropertyChangeListener {
public:
    SGLightDebugListener(osg::Switch *sw) : _sw(sw) {}
    virtual void valueChanged(SGPropertyNode *node) {
        _sw->setValue(0, node->getBoolValue());
    }
private:
    osg::ref_ptr<osg::Switch> _sw;
};

osg::Node *
SGLight::appendLight(const SGPropertyNode *configNode,
                     SGPropertyNode *modelRoot,
                     const osgDB::Options *options)
{
    SGConstPropertyNode_ptr p;

    SGLight *light = new SGLight;

    if((p = configNode->getNode("type")) != NULL) {
        std::string type = p->getStringValue();
        if (type == "point")
            light->setType(SGLight::Type::POINT);
        else if (type == "spot")
            light->setType(SGLight::Type::SPOT);
        else
            SG_LOG(SG_GENERAL, SG_ALERT, "ignoring unknown light type '" << type << "'");
    }

    light->setRange(configNode->getFloatValue("range-m"));

#define SGLIGHT_GET_COLOR_VALUE(n)                \
    osg::Vec4(configNode->getFloatValue(n "/r"),  \
              configNode->getFloatValue(n "/g"),  \
              configNode->getFloatValue(n "/b"),  \
              configNode->getFloatValue(n "/a"))
    light->setAmbient(SGLIGHT_GET_COLOR_VALUE("ambient"));
    light->setDiffuse(SGLIGHT_GET_COLOR_VALUE("diffuse"));
    light->setSpecular(SGLIGHT_GET_COLOR_VALUE("specular"));
#undef SGLIGHT_GET_COLOR_VALUE

    light->setConstantAttenuation(configNode->getFloatValue("attenuation/c"));
    light->setLinearAttenuation(configNode->getFloatValue("attenuation/l"));
    light->setQuadraticAttenuation(configNode->getFloatValue("attenuation/q"));

    light->setSpotExponent(configNode->getFloatValue("spot-exponent"));
    light->setSpotCutoff(configNode->getFloatValue("spot-cutoff"));

    osg::MatrixTransform *align = new osg::MatrixTransform;
    align->addChild(light);

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
    align->setMatrix(r * t);

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

    if ((p = configNode->getNode("name")) != NULL)
        align->setName(p->getStringValue());
    else
        align->setName("light");

    return align;
}

SGLight::SGLight() :
    _type(Type::POINT),
    _range(0.0f)
{
    // Default values taken from osg::Light
    // They don't matter anyway as they are overwritten by the XML config values
    _ambient.set(0.05f, 0.05f, 0.05f, 1.0f);
    _diffuse.set(0.8f, 0.8f, 0.8f, 1.0f);
    _specular.set(0.05f, 0.05f, 0.05f, 1.0f);
    _constant_attenuation = 1.0f;
    _linear_attenuation = 0.0f;
    _quadratic_attenuation = 0.0f;
    _spot_exponent = 0.0f;
    _spot_cutoff = 180.0f;
}

SGLight::~SGLight()
{

}
