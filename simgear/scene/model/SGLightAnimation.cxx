#include "animation.hxx"

#include <osg/MatrixTransform>

#include "SGLight.hxx"

SGLightAnimation::SGLightAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData)
{
    SGLight *light = new SGLight;

    std::string light_type = getConfig()->getStringValue("light-type");
    if (light_type == "point")
        light->setType(SGLight::POINT);
    else if (light_type == "spot")
        light->setType(SGLight::SPOT);

    light->setRange(getConfig()->getFloatValue("far-m"));

#define SGLIGHT_GET_COLOR_VALUE(n)                  \
    osg::Vec4(getConfig()->getFloatValue(n "/r"),    \
              getConfig()->getFloatValue(n "/g"),    \
              getConfig()->getFloatValue(n "/b"),    \
              getConfig()->getFloatValue(n "/a"))
    light->setAmbient(SGLIGHT_GET_COLOR_VALUE("ambient"));
    light->setDiffuse(SGLIGHT_GET_COLOR_VALUE("diffuse"));
    light->setSpecular(SGLIGHT_GET_COLOR_VALUE("specular"));
#undef SGLIGHT_GET_COLOR_VALUE

    light->setConstantAttenuation(getConfig()->getFloatValue("attenuation/c"));
    light->setLinearAttenuation(getConfig()->getFloatValue("attenuation/l"));
    light->setQuadraticAttenuation(getConfig()->getFloatValue("attenuation/q"));

    light->setSpotExponent(getConfig()->getFloatValue("exponent"));
    light->setSpotCutoff(getConfig()->getFloatValue("cutoff"));

    osg::MatrixTransform *align = new osg::MatrixTransform;
    align->addChild(light);

    osg::Matrix t;
    osg::Vec3 pos(getConfig()->getFloatValue("position/x"),
                  getConfig()->getFloatValue("position/y"),
                  getConfig()->getFloatValue("position/z"));
    t.makeTranslate(pos);

    osg::Matrix r;
    if (const SGPropertyNode *dirNode = getConfig()->getNode("direction")) {
        r.makeRotate(osg::Vec3(0, 0, -1),
                     osg::Vec3(dirNode->getFloatValue("x"),
                               dirNode->getFloatValue("y"),
                               dirNode->getFloatValue("z")));
    }
    align->setMatrix(r * t);

    const SGPropertyNode *dim_factor = getConfig()->getChild("dim-factor");
    if (dim_factor) {
        const SGExpressiond *expression =
            read_value(dim_factor, modelData.getModelRoot(), "", 0, 1);
        light->setUpdateCallback(
            new SGLight::UpdateCallback(expression,
                                        light->getAmbient(),
                                        light->getDiffuse(),
                                        light->getSpecular()));
    }

    align->setName(getConfig()->getStringValue("name"));
    _light = align;
}

osg::Group*
SGLightAnimation::createAnimationGroup(osg::Group& parent)
{
    osg::Group* group = new osg::Group;
    group->setName("light animation node");
    parent.addChild(group);
    group->addChild(_light);
    return group;
}

void
SGLightAnimation::install(osg::Node& node)
{
    SGAnimation::install(node);
    // Hide the legacy light geometry
    node.setNodeMask(0);
}
