/* -*-c++-*-
 *
 * Copyright (C) 2020 Fahim Dalvi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "LightBin.hxx"

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Geometry>
#include <osg/StateSet>

#include <simgear/debug/logstream.hxx>
#include <simgear/io/iostreams/sgstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/SGEnlargeBoundingBox.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>

using namespace osg;
namespace simgear {

void LightBin::insert(const Light& light) {
    _aggregated_size += light.size;
    _aggregated_intensity += light.intensity;
    _lights.push_back(light);
}

void LightBin::insert(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c) {
    insert(Light(p, s, i, o, c));
}
void LightBin::insert(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c, const SGVec4f& a) {
    insert(Light(p, s, i, o, c, a));
}
void LightBin::insert(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c, const SGVec3f& d, const double& ha, const double& va) {
    insert(Light(p, s, i, o, c, d, ha, va));
}
void LightBin::insert(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c, const SGVec3f& d, const double& ha, const double& va, const SGVec4f& a) {
    insert(Light(p, s, i, o, c, d, ha, va, a));
}

unsigned LightBin::getNumLights() const
{
    return _lights.size();
}

const LightBin::Light& LightBin::getLight(unsigned i) const
{
    return _lights[i];
}

double LightBin::getAverageSize() const
{
    return _aggregated_size / getNumLights();
}

double LightBin::getAverageIntensity() const
{
    return _aggregated_intensity / getNumLights();
}

LightBin::LightBin() :
    _aggregated_size(0.0), _aggregated_intensity(0.0)  
{
}

LightBin::LightBin(const SGPath& absoluteFileName) :
    _aggregated_size(0.0), _aggregated_intensity(0.0)  
{ 
    sg_gzifstream stream(absoluteFileName);
    if (!stream.is_open()) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Unable to open " << absoluteFileName);
        return;
    }

    // Every light is defined by at most 19 properties denoting position,
    // size, intensity, color, directionality and animations
    float props[19];

    while (!stream.eof()) {
        // read a line.  Each line defines a single light position and its properties,
        // and may have a comment, starting with #
        std::string line;
        std::getline(stream, line);

        // strip comments
        std::string::size_type hash_pos = line.find('#');
        if (hash_pos != std::string::npos)
            line.resize(hash_pos);

        if (line.empty()) {
            continue; // skip blank lines
        }

        // and process further
        std::stringstream in(line);

        int number_of_props = 0;
        while(!in.eof()) {
            in >> props[number_of_props++];
        }

        if (number_of_props == 10) {
            // Omnidirectional
            insert(
                SGVec3f(props[0], props[1], props[2]), // position
                props[3], // size
                props[4], // intensity
                props[5], // on_period
                SGVec4f(props[6], props[7], props[8], props[9]) //color
            );
        } else if (number_of_props == 14) {
            // Omnidirectional Animated
            insert(
                SGVec3f(props[0], props[1], props[2]), // position
                props[3], // size
                props[4], // intensity
                props[5], // on_period
                SGVec4f(props[6], props[7], props[8], props[9]), // color
                SGVec4f(props[10], props[11], props[12], props[13]) // animation_params
            );
        } else if (number_of_props == 15) {
            // Directional
            insert(
                SGVec3f(props[0], props[1], props[2]), // position
                props[3], // size
                props[4], // intensity
                props[5], // on_period
                SGVec4f(props[6], props[7], props[8], props[9]), // color
                SGVec3f(props[10], props[11], props[12]), // direction
                props[13], // horizontal_angle
                props[14] // vertical_angle
            );
        } else if (number_of_props == 19) {
            // Directional Animated
            insert(
                SGVec3f(props[0], props[1], props[2]), // position
                props[3], // size
                props[4], // intensity
                props[5], // on_period
                SGVec4f(props[6], props[7], props[8], props[9]), // color
                SGVec3f(props[10], props[11], props[12]), // direction
                props[13], // horizontal_angle
                props[14], // vertical_angle
                SGVec4f(props[15], props[16], props[17], props[18]) // animation_params
            );
        } else {
            SG_LOG(SG_TERRAIN, SG_WARN, "Error parsing light entry in: " << absoluteFileName << " line: \"" << line << "\"");
                continue;
        }
    }

    stream.close();
}

Effect* getLightEffect(double average_size, double average_intensity, const SGReaderWriterOptions* options)
{
    SGPropertyNode_ptr effectProp = new SGPropertyNode;
    makeChild(effectProp, "inherits-from")->setStringValue("Effects/scenery-lights");

    /** Add average size + intensity as uniforms for non-shader or
     *  non-point-sprite based lights. We cannot have per-light control using
     *  OpenGL's default lights GL_ARB_point_sprite/GL_ARB_point_parameters
     *  extensions, so we make a best-effort by making all lights in this bin
     *  the average size/intensity.
     */

    // Clamp intensity between 0 and 50000 candelas
    average_intensity = std::clamp(average_intensity, 0.0, 50000.0);

    // Scale size from cm to what OpenGL extensions expects
    average_size = 1 + 50 * std::clamp(average_size, 0.0, 500.0) / 500.0;

    SGPropertyNode* params = makeChild(effectProp, "parameters");
    params->getNode("size",true)->setValue(average_size);
    params->getNode("attenuation",true)->getNode("x", true)->setValue(1.0);
    params->getNode("attenuation",true)->getNode("y", true)->setValue(0.00001);
    params->getNode("attenuation",true)->getNode("z", true)->setValue(0.00000001);
    params->getNode("min-size",true)->setValue(1.0f);
    params->getNode("max-size",true)->setValue(average_size*(1+2*average_intensity/50000));
    params->getNode("cull-face",true)->setValue("off");

    return makeEffect(effectProp, true, options);
}

osg::Drawable* createDrawable(LightBin& lightList, const osg::Matrix& transform) {
    if (lightList.getNumLights() <= 0)
        return 0;

    osg::Vec3Array* vertices = new osg::Vec3Array;
    osg::Vec4Array* colors = new osg::Vec4Array;
    osg::Vec3Array* light_params = new osg::Vec3Array;
    osg::Vec4Array* animation_params = new osg::Vec4Array;
    osg::Vec3Array* direction_params_1 = new osg::Vec3Array;
    osg::Vec2Array* direction_params_2 = new osg::Vec2Array;

    for (unsigned int lightIdx = 0; lightIdx < lightList.getNumLights(); lightIdx++) {
        const auto l = lightList.getLight(lightIdx);
        vertices->push_back(toOsg(l.position) * transform);
        light_params->push_back(toOsg(SGVec3f(l.size, l.intensity, l.on_period)));
        direction_params_1->push_back(toOsg(l.direction));
        direction_params_2->push_back(toOsg(SGVec2f(l.horizontal_angle, l.vertical_angle)));
        animation_params->push_back(toOsg(l.animation_params));
        colors->push_back(toOsg(l.color));
    }

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setDataVariance(osg::Object::STATIC);
    geometry->setVertexArray(vertices);
    geometry->setVertexAttribArray(LIGHT_ATTR1, light_params, osg::Array::BIND_PER_VERTEX);
    geometry->setVertexAttribArray(LIGHT_ATTR2, animation_params, osg::Array::BIND_PER_VERTEX);
    geometry->setVertexAttribArray(LIGHT_ATTR3, direction_params_1, osg::Array::BIND_PER_VERTEX);
    geometry->setVertexAttribArray(LIGHT_ATTR4, direction_params_2, osg::Array::BIND_PER_VERTEX);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->setColorArray(colors, osg::Array::BIND_PER_VERTEX);

    geometry->setComputeBoundingBoxCallback(new SGEnlargeBoundingBox(1));

    osg::DrawArrays* drawArrays;
    drawArrays = new osg::DrawArrays(osg::PrimitiveSet::POINTS,
                                    0, vertices->size());
    geometry->addPrimitiveSet(drawArrays);

    osg::StateSet* stateSet = geometry->getOrCreateStateSet();
    stateSet->setRenderBinDetails(POINT_LIGHTS_BIN, "DepthSortedBin");
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    stateSet->setAttribute(blendFunc);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);

    osg::AlphaFunc* alphaFunc;
    alphaFunc = new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0.01);
    stateSet->setAttribute(alphaFunc);
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
    
    return geometry;
}

osg::ref_ptr<simgear::EffectGeode> createLights(LightBin& lightList, const osg::Matrix& transform, const SGReaderWriterOptions* options) {
    osg::ref_ptr<EffectGeode> geode = new EffectGeode;
    osg::ref_ptr<Effect> lightEffect = getLightEffect(lightList.getAverageSize(), lightList.getAverageIntensity(), options);
    geode->setEffect(lightEffect);
    geode->addDrawable(createDrawable(lightList, transform));

    // Set all light bits so that the lightbin is always on. Per light on/off
    // control is provided by the shader.
    geode->setNodeMask(LIGHTS_BITS | MODEL_BIT | PERMANENTLIGHT_BIT);

    return geode;
}

};
