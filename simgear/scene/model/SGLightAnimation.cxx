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
#include <simgear/scene/material/Technique.hxx>
#include <simgear/scene/material/Pass.hxx>
#include <simgear/scene/util/CopyOp.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <boost/scoped_array.hpp>
#include <boost/foreach.hpp>

typedef std::map<string, osg::ref_ptr<simgear::Effect> > EffectMap;
static EffectMap lightEffectMap;

#define GET_COLOR_VALUE(n) \
    SGVec4d( getConfig()->getDoubleValue(n "/r"), \
                getConfig()->getDoubleValue(n "/g"), \
                getConfig()->getDoubleValue(n "/b"), \
                getConfig()->getDoubleValue(n "/a") )

class SGLightAnimation::UpdateCallback : public osg::NodeCallback {
public:
    UpdateCallback(string k, const SGExpressiond* v, SGVec4d a, SGVec4d d, SGVec4d s) :
        _key(k),
        _ambient(a),
        _diffuse(d),
        _specular(s),
        _animationValue(v)
    {
        _prev_values[k] = -1;
    }
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        double dim = _animationValue->getValue();
        if (dim != _prev_values[_key]) {
            _prev_values[_key] = dim;

            EffectMap::iterator iter = lightEffectMap.find(_key);
            if (iter != lightEffectMap.end()) {
                simgear::Effect* effect = iter->second;
                SGPropertyNode* params = effect->parametersProp;
                params->getNode("ambient")->setValue(_ambient * dim);
                params->getNode("diffuse")->setValue(_diffuse * dim);
                params->getNode("specular")->setValue(_specular * dim);
                BOOST_FOREACH(osg::ref_ptr<simgear::Technique>& technique, effect->techniques)
                {
                    BOOST_FOREACH(osg::ref_ptr<simgear::Pass>& pass, technique->passes)
                    {
                        osg::Uniform* amb = pass->getUniform("Ambient");
                        if (amb)
                            amb->set(toOsg(_ambient) * dim);
                        osg::Uniform* dif = pass->getUniform("Diffuse");
                        if (dif)
                            dif->set(toOsg(_diffuse) * dim);
                        osg::Uniform* spe = pass->getUniform("Specular");
                        if (spe)
                            spe->set(toOsg(_specular) * dim);
                    }
                }
            }
        }
        traverse(node, nv);
    }
public:
    string _key;
    SGVec4d _ambient;
    SGVec4d _diffuse;
    SGVec4d _specular;
    SGSharedPtr<SGExpressiond const> _animationValue;

    typedef std::map<string, double> PrevValueMap;
    static PrevValueMap _prev_values;
};
SGLightAnimation::UpdateCallback::PrevValueMap SGLightAnimation::UpdateCallback::_prev_values;

SGLightAnimation::SGLightAnimation(const SGPropertyNode* configNode,
                                   SGPropertyNode* modelRoot,
                                   const string &path, int i) :
    SGAnimation(configNode, modelRoot)
{
    _light_type = getConfig()->getStringValue("light-type");
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

    SGConstPropertyNode_ptr dim_factor = configNode->getChild("dim-factor");
    if (dim_factor.valid()) {
        _animationValue = read_value(dim_factor, modelRoot, "", 0, 1);
    }
}

osg::Group*
SGLightAnimation::createAnimationGroup(osg::Group& parent)
{
    osg::Group* grp = new osg::Group;
    grp->setName("light animation node");
    if (_animationValue.valid())
        grp->setUpdateCallback(new UpdateCallback(_key, _animationValue, _ambient, _diffuse, _specular));
    parent.addChild(grp);
    grp->setNodeMask( simgear::MODELLIGHT_BIT );
    return grp;
}

void
SGLightAnimation::install(osg::Node& node)
{
    SGAnimation::install(node);

    if (_light_type == "spot") {

        simgear::Effect* effect = 0;
        EffectMap::iterator iter = lightEffectMap.find(_key);
        if (iter == lightEffectMap.end()) {
            SGPropertyNode_ptr effectProp = new SGPropertyNode;
            makeChild(effectProp, "name")->setStringValue(_key);
            makeChild(effectProp, "inherits-from")->setStringValue("Effects/light-spot");
            double dim = 1.0;
            if (_animationValue.valid())
                dim = _animationValue->getValue();

            SGPropertyNode* params = makeChild(effectProp, "parameters");
            params->getNode("position",true)->setValue(SGVec4d(_position.x(),_position.y(),_position.z(),1.0));
            params->getNode("direction",true)->setValue(SGVec4d(_direction.x(),_direction.y(),_direction.z(),0.0));
            params->getNode("ambient",true)->setValue(_ambient * dim);
            params->getNode("diffuse",true)->setValue(_diffuse * dim);
            params->getNode("specular",true)->setValue(_specular * dim);
            params->getNode("attenuation",true)->setValue(_attenuation);
            params->getNode("exponent",true)->setValue(_exponent);
            params->getNode("cutoff",true)->setValue(_cutoff);
            params->getNode("cosCutoff",true)->setValue( cos(_cutoff*SG_DEGREES_TO_RADIANS) );
            params->getNode("near",true)->setValue(_near);
            params->getNode("far",true)->setValue(_far);

            effect = simgear::makeEffect(effectProp, true);
            lightEffectMap.insert(EffectMap::value_type(_key, effect));
        } else {
            effect = iter->second.get();
        }

        node.setNodeMask( simgear::MODELLIGHT_BIT );
        simgear::EffectGeode* geode = dynamic_cast<simgear::EffectGeode*>(&node);
        if (geode == 0) {
            osg::Group* grp = node.asGroup();
            if (grp != 0) {
                for (size_t i=0; i<grp->getNumChildren(); ++i) {
                    geode = dynamic_cast<simgear::EffectGeode*>(grp->getChild(i));
                    if (geode)
                        geode->setEffect(effect);
                }
            }
        }
    }
    else if (_light_type == "point") {

        simgear::Effect* effect = 0;
        EffectMap::iterator iter = lightEffectMap.find(_key);
        if (iter == lightEffectMap.end()) {
            SGPropertyNode_ptr effectProp = new SGPropertyNode;
            makeChild(effectProp, "name")->setStringValue(_key);
            makeChild(effectProp, "inherits-from")->setStringValue("Effects/light-point");
            double dim = 1.0;
            if (_animationValue.valid())
                dim = _animationValue->getValue();

            SGPropertyNode* params = makeChild(effectProp, "parameters");
            params->getNode("position",true)->setValue(SGVec4d(_position.x(),_position.y(),_position.z(),1.0));
            params->getNode("ambient",true)->setValue(_ambient * dim);
            params->getNode("diffuse",true)->setValue(_diffuse * dim);
            params->getNode("specular",true)->setValue(_specular * dim);
            params->getNode("attenuation",true)->setValue(_attenuation);
            params->getNode("near",true)->setValue(_near);
            params->getNode("far",true)->setValue(_far);

            effect = simgear::makeEffect(effectProp, true);
            lightEffectMap.insert(EffectMap::value_type(_key, effect));
        } else {
            effect = iter->second.get();
        }

        node.setNodeMask( simgear::MODELLIGHT_BIT );
        simgear::EffectGeode* geode = dynamic_cast<simgear::EffectGeode*>(&node);
        if (geode == 0) {
            osg::Group* grp = node.asGroup();
            if (grp != 0) {
                for (size_t i=0; i<grp->getNumChildren(); ++i) {
                    geode = dynamic_cast<simgear::EffectGeode*>(grp->getChild(i));
                    if (geode)
                        geode->setEffect(effect);
                }
            }
        }
    }
}
