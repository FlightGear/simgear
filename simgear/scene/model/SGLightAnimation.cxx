// Copyright (C) 2011 Frederic Bouvier (fredfgfs01@free.fr)
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <boost/lexical_cast.hpp>

#include "animation.hxx"
#include "ConditionNode.hxx"

#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <simgear/scene/material/EffectGeode.hxx>
#include <boost/scoped_array.hpp>
#include <simgear/scene/util/CopyOp.hxx>

typedef std::map<string, osg::ref_ptr<simgear::Effect> > EffectMap;
static EffectMap lightEffectMap;

#define GET_COLOR_VALUE(n) \
    SGVec4d( getConfig()->getDoubleValue(n "/r"), \
                getConfig()->getDoubleValue(n "/g"), \
                getConfig()->getDoubleValue(n "/b"), \
                getConfig()->getDoubleValue(n "/a") )

SGLightAnimation::SGLightAnimation(const SGPropertyNode* configNode,
                                   SGPropertyNode* modelRoot,
                                   const string &path, int i) :
    SGAnimation(configNode, modelRoot)
{
    _position = SGVec3d( getConfig()->getDoubleValue("position/x"), getConfig()->getDoubleValue("position/y"), getConfig()->getDoubleValue("position/z") );
    _direction = SGVec3d( getConfig()->getDoubleValue("direction/x"), getConfig()->getDoubleValue("direction/y"), getConfig()->getDoubleValue("direction/z") );
    double l = length(_direction);
    if (l > 0.001) _direction /= l;
    _ambient = GET_COLOR_VALUE("ambient");
    _diffuse = GET_COLOR_VALUE("diffuse");
    _specular = GET_COLOR_VALUE("specular");
    _attenuation = SGVec3d( getConfig()->getDoubleValue("attenuation/c"), getConfig()->getDoubleValue("attenuation/l"), getConfig()->getDoubleValue("attenuation/q") );
    _exponent = getConfig()->getDoubleValue("exponent");
    _cutoff = getConfig()->getDoubleValue("cutoff");
    _near = getConfig()->getDoubleValue("near-m");
    _far = getConfig()->getDoubleValue("far-m");
    _key = path + ";" + boost::lexical_cast<string>( i );
}

osg::Group*
SGLightAnimation::createAnimationGroup(osg::Group& parent)
{
    osg::MatrixTransform* grp = new osg::MatrixTransform;
    grp->setMatrix(osg::Matrix::translate(toOsg(_position)));
    parent.addChild(grp);
    grp->setNodeMask( simgear::MODELLIGHT_BIT );
    return grp;
}

void
SGLightAnimation::install(osg::Node& node)
{
    SGAnimation::install(node);

    std::string light_type = getConfig()->getStringValue("light-type");
    if (light_type == "spot") {

        SGVec3d p1( _direction.z(), _direction.x(), _direction.y() ),
        p2 = cross( _direction, p1 );
        p1 = cross( p2, _direction );

        float r2 = _far * tan( _cutoff * SG_DEGREES_TO_RADIANS );

        osg::Geometry* cone = new osg::Geometry;
cone->setUseDisplayList(false);
        osg::Vec3Array* vertices = new osg::Vec3Array(34);
        (*vertices)[0] = osg::Vec3(0.0,0.0,0.0);
        for (int i=0; i<16; ++i) {
            (*vertices)[16-i]    = toOsg(_direction)*_far + toOsg(p1)*r2*cos( i * 2 * M_PI / 16 ) + toOsg(p2)*r2*sin( i * 2 * M_PI / 16 );
        }
        (*vertices)[17] = (*vertices)[1];

        // Bottom
        for (int i=0; i<16; ++i) {
            (*vertices)[18+i]    = toOsg(_direction)*_far + toOsg(p1)*r2*cos( i * 2 * M_PI / 16 ) + toOsg(p2)*r2*sin( i * 2 * M_PI / 16 );
        }

        osg::Vec4Array* colours = new osg::Vec4Array(1);
        (*colours)[0] = toOsg(_diffuse);
        cone->setColorArray(colours);
        cone->setColorBinding(osg::Geometry::BIND_OVERALL);

        osg::Vec3Array* normals = new osg::Vec3Array(1);
        (*normals)[0] = toOsg(_direction);
        cone->setNormalArray(normals);
        cone->setNormalBinding(osg::Geometry::BIND_OVERALL);

        cone->setVertexArray(vertices);
        cone->addPrimitiveSet( new osg::DrawArrays( GL_TRIANGLE_FAN, 0, 18 ) );
        cone->addPrimitiveSet( new osg::DrawArrays( GL_TRIANGLE_FAN, 18, 16 ) );

        simgear::EffectGeode* geode = new simgear::EffectGeode;
        geode->addDrawable( cone );

        node.asGroup()->addChild( geode );

        simgear::Effect* effect = 0;
        EffectMap::iterator iter = lightEffectMap.find(_key);
        if (iter == lightEffectMap.end()) {
            SGPropertyNode_ptr effectProp = new SGPropertyNode;
            makeChild(effectProp, "inherits-from")->setStringValue("Effects/light-spot");
            SGPropertyNode* params = makeChild(effectProp, "parameters");
            params->getNode("light-spot/position",true)->setValue(SGVec4d(_position.x(),_position.y(),_position.z(),1.0));
            params->getNode("light-spot/direction",true)->setValue(SGVec4d(_direction.x(),_direction.y(),_direction.z(),0.0));
            params->getNode("light-spot/ambient",true)->setValue(_ambient);
            params->getNode("light-spot/diffuse",true)->setValue(_diffuse);
            params->getNode("light-spot/specular",true)->setValue(_specular);
            params->getNode("light-spot/attenuation",true)->setValue(_attenuation);
            params->getNode("light-spot/exponent",true)->setValue(_exponent);
            params->getNode("light-spot/cutoff",true)->setValue(_cutoff);
            params->getNode("light-spot/cosCutoff",true)->setValue( cos(_cutoff*SG_DEGREES_TO_RADIANS) );
            params->getNode("light-spot/near",true)->setValue(_near);
            params->getNode("light-spot/far",true)->setValue(_far);

            effect = simgear::makeEffect(effectProp, true);
            lightEffectMap.insert(EffectMap::value_type(_key, effect));
        } else {
            effect = iter->second.get();
        }
        geode->setEffect(effect);
    }
}
