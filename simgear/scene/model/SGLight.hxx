// Copyright (C) 2018  Fernando García Liñán <fernandogarcialinan@gmail.com>
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#ifndef SG_LIGHT_HXX
#define SG_LIGHT_HXX

#include <osgDB/ReaderWriter>
#include <osg/Group>

#include <simgear/props/props.hxx>

class SGLight : public osg::Node {
public:
    enum Type {
        POINT,
        SPOT
    };

    SGLight();

    SGLight(const SGLight& l,
            const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY) :
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

    META_Node(simgear, SGLight);

    static osg::Node *appendLight(const SGPropertyNode *configNode,
                                  SGPropertyNode *modelRoot,
                                  const osgDB::Options *options);

    void setType(Type type) { _type = type; }
    Type getType() const { return _type; }

    void setRange(float range) { _range = range; }
    float getRange() const { return _range; }

    void setAmbient(const osg::Vec4 &ambient) { _ambient = ambient; }
    const osg::Vec4 &getAmbient() const { return _ambient; }

    void setDiffuse(const osg::Vec4 &diffuse) { _diffuse = diffuse; }
    const osg::Vec4 &getDiffuse() const { return _diffuse; }

    void setSpecular(const osg::Vec4 &specular) { _specular = specular; }
    const osg::Vec4 &getSpecular() const { return _specular; }

    void setConstantAttenuation(float constant_attenuation) { _constant_attenuation = constant_attenuation; }
    float getConstantAttenuation() const { return _constant_attenuation; }

    void setLinearAttenuation(float linear_attenuation) { _linear_attenuation = linear_attenuation; }
    float getLinearAttenuation() const { return _linear_attenuation; }

    void setQuadraticAttenuation(float quadratic_attenuation) { _quadratic_attenuation = quadratic_attenuation; }
    float getQuadraticAttenuation() const { return _quadratic_attenuation; }

    void setSpotExponent(float spot_exponent) { _spot_exponent = spot_exponent; }
    float getSpotExponent() const { return _spot_exponent; }

    void setSpotCutoff(float spot_cutoff) { _spot_cutoff = spot_cutoff; }
    float getSpotCutoff() const { return _spot_cutoff; }

protected:
    virtual ~SGLight();

    Type _type;

    float _range;

    osg::Vec4 _ambient;
    osg::Vec4 _diffuse;
    osg::Vec4 _specular;

    float _constant_attenuation;
    float _linear_attenuation;
    float _quadratic_attenuation;
    float _spot_exponent;
    float _spot_cutoff;
};

typedef std::vector<osg::ref_ptr<SGLight>> SGLightList;

#endif /* SG_LIGHT_HXX */
