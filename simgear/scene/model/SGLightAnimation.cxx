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

#include "animation.hxx"
#include "ConditionNode.hxx"

#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <simgear/props/vectorPropTemplates.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/material/Technique.hxx>
#include <simgear/scene/material/Pass.hxx>
#include <simgear/scene/util/CopyOp.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

typedef std::map<std::string, osg::observer_ptr<simgear::Effect> > EffectMap;
static EffectMap lightEffectMap;

#define GET_COLOR_VALUE(n) \
    SGVec4d( getConfig()->getDoubleValue(n "/r"), \
                getConfig()->getDoubleValue(n "/g"), \
                getConfig()->getDoubleValue(n "/b"), \
                getConfig()->getDoubleValue(n "/a") )

class SGLightAnimation::UpdateCallback : public osg::NodeCallback {
public:
    UpdateCallback(const SGExpressiond* v, SGVec4d a, SGVec4d d, SGVec4d s) :
        _ambient(a),
        _diffuse(d),
        _specular(s),
        _animationValue(v)
    {
        _prev_value = -1;
    }
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        double dim = _animationValue->getValue();
        if (dim != _prev_value) {
            _prev_value = dim;
            simgear::EffectGeode* geode = dynamic_cast<simgear::EffectGeode*>( node );
            if (geode != 0) {
                osg::ref_ptr<simgear::Effect> effect = geode->getEffect();
                if (effect != nullptr) {
                    SGPropertyNode* params = effect->parametersProp;
                    params->getNode("ambient")->setValue(_ambient * dim);
                    params->getNode("diffuse")->setValue(_diffuse * dim);
                    params->getNode("specular")->setValue(_specular * dim);
                    for (auto& technique : effect->techniques) {
                        for (auto& pass : technique->passes) {
                            osg::Uniform* amb = pass->getUniform("Ambient");
                            if (amb)
                                amb->set(osg::Vec4f(_ambient.x() * dim, _ambient.y() * dim, _ambient.z() * dim, _ambient.w() * dim));
                            osg::Uniform* dif = pass->getUniform("Diffuse");
                            if (dif)
                                dif->set(osg::Vec4f(_diffuse.x() * dim, _diffuse.y() * dim, _diffuse.z() * dim, _diffuse.w() * dim));
                            osg::Uniform* spe = pass->getUniform("Specular");
                            if (spe)
                                spe->set(osg::Vec4f(_specular.x() * dim, _specular.y() * dim, _specular.z() * dim, _specular.w() * dim));
                        }
                    }
                }
            }
        }
        traverse(node, nv);
    }
public:
    SGVec4d _ambient;
    SGVec4d _diffuse;
    SGVec4d _specular;
    SGSharedPtr<SGExpressiond const> _animationValue;
    double _prev_value;
};

SGLightAnimation::SGLightAnimation(simgear::SGTransientModelData &modelData) :
    SGAnimation(modelData), _options(modelData.getOptions())
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
    _key = modelData.getPath() + ";" + std::to_string(modelData.getIndex());

    SGConstPropertyNode_ptr dim_factor = modelData.getConfigNode()->getChild("dim-factor");
    if (dim_factor.valid()) {
        _animationValue = read_value(dim_factor, modelData.getModelRoot(), "", 0, 1);
    }
}

osg::Group*
SGLightAnimation::createAnimationGroup(osg::Group& parent)
{
    osg::Group* grp = new osg::Group;
    grp->setName("light animation node");
    parent.addChild(grp);
    grp->setNodeMask( simgear::MODELLIGHT_BIT );
    return grp;
}

void
SGLightAnimation::install(osg::Node& node)
{
    SGAnimation::install(node);

    bool cacheEffect = false;
    osg::ref_ptr<simgear::Effect> effect;
    EffectMap::iterator iter = lightEffectMap.end();
    if (!_animationValue.valid()) { // Effects with animated properties should be singletons
        iter = lightEffectMap.find(_key);
        cacheEffect = true;
    }

    if (iter == lightEffectMap.end() || !iter->second.lock(effect)) {
        SGPropertyNode_ptr effectProp = new SGPropertyNode;
        if (_light_type == "spot") {
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
        }
        else if (_light_type == "point") {
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
        } else {
            return;
        }

        effect = simgear::makeEffect(effectProp, true, dynamic_cast<const simgear::SGReaderWriterOptions*>(_options.get()));
        if (iter == lightEffectMap.end() && cacheEffect)
            lightEffectMap.insert(EffectMap::value_type(_key, effect));
        else if (cacheEffect)
            iter->second = effect;
    } else {
        effect = iter->second.get();
    }
    if (effect == nullptr) {
        printf("invalid effect - hiding geometry as light not valid.\n");
        node.setNodeMask(0);
        return;
    }
    node.setNodeMask( simgear::MODELLIGHT_BIT );
    simgear::EffectGeode* geode = dynamic_cast<simgear::EffectGeode*>(&node);
    if (geode == 0) {
        osg::Group* grp = node.asGroup();
        if (grp != 0) {
            for (size_t i=0; i<grp->getNumChildren(); ++i) {
                geode = dynamic_cast<simgear::EffectGeode*>(grp->getChild(i));
                if (geode) {
                    geode->setNodeMask( simgear::MODELLIGHT_BIT );
                    geode->setEffect(effect);
                }
            }
        }
    }

    if (geode != 0 && _animationValue.valid())
        geode->setUpdateCallback(new UpdateCallback(_animationValue, _ambient, _diffuse, _specular));
}
