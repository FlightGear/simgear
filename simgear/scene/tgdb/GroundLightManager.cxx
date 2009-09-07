#include <osg/Fog>

#include <simgear/scene/util/RenderConstants.hxx>
#include "GroundLightManager.hxx"



using namespace osg;

namespace
{
StateSet* makeLightSS()
{
    StateSet* ss = new StateSet;
    Fog* fog = new Fog;
    fog->setMode(Fog::EXP2);
    ss->setAttribute(fog);
    ss->setDataVariance(Object::DYNAMIC);
    return ss;
}
}

namespace simgear
{
GroundLightManager::GroundLightManager()
{
    runwayLightSS = makeLightSS();
    taxiLightSS = makeLightSS();
    groundLightSS = makeLightSS();
}

void GroundLightManager::update(const SGUpdateVisitor* updateVisitor)
{
    osg::Fog* fog;
    SGVec4f fogColor = updateVisitor->getFogColor();
    fog = static_cast<osg::Fog*>(runwayLightSS
                                 ->getAttribute(StateAttribute::FOG));
    fog->setColor(toOsg(fogColor));
    fog->setDensity(updateVisitor->getRunwayFogExp2Density());
    fog = static_cast<osg::Fog*>(taxiLightSS
                                 ->getAttribute(StateAttribute::FOG));
    fog->setColor(toOsg(fogColor));
    fog->setDensity(updateVisitor->getTaxiFogExp2Density());
    fog = static_cast<osg::Fog*>(groundLightSS
                                 ->getAttribute(StateAttribute::FOG));
    fog->setColor(toOsg(fogColor));
    fog->setDensity(updateVisitor->getGroundLightsFogExp2Density());
}

unsigned GroundLightManager::getLightNodeMask(const SGUpdateVisitor* updateVisitor)
{
    unsigned mask = 0;
    // The current sun angle in degree
    float sun_angle = updateVisitor->getSunAngleDeg();
    if (sun_angle > 85 || updateVisitor->getVisibility() < 5000)
        mask |= RUNWAYLIGHTS_BIT;
    // ground lights
    if ( sun_angle > 95 )
        mask |= GROUNDLIGHTS2_BIT;
    if ( sun_angle > 92 )
        mask |= GROUNDLIGHTS1_BIT;
    if ( sun_angle > 89 )
        mask |= GROUNDLIGHTS0_BIT;
    return mask;
}
}
