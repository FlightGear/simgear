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

#ifndef LIGHT_BIN_HXX
#define LIGHT_BIN_HXX

#include <osg/Drawable>

#include <simgear/math/SGMath.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/tgdb/LightBin.hxx>

namespace simgear
{
class LightBin {
public:
    struct Light {
        // Omni-directional non-animated lights constructor
        Light(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c) :
            position(p), color(c), size(s), intensity(i), on_period(o),
            direction(SGVec3f(0.0f, 0.0f, 0.0f)), horizontal_angle(360.0f), vertical_angle(360.0f),
            animation_params(SGVec4f(-1.0f, 0.0f, 0.0f, 0.0f))
        { }

        // Omni-directional animated lights constructor
        Light(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c, const SGVec4f& a) :
            position(p), color(c), size(s), intensity(i), on_period(o),
            direction(SGVec3f(0.0f, 0.0f, 0.0f)), horizontal_angle(360.0f), vertical_angle(360.0f),
            animation_params(a)
        { }

        // Directional non-animated lights constructor
        Light(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c, const SGVec3f& d, const double& ha, const double& va) :
            position(p), color(c), size(s), intensity(i), on_period(o),
            direction(d), horizontal_angle(ha), vertical_angle(va),
            animation_params(SGVec4f(-1.0f, 0.0f, 0.0f, 0.0f))
        { }

        // Directional animated lights constructor
        Light(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c, const SGVec3f& d, const double& ha, const double& va, const SGVec4f& a) :
            position(p), color(c), size(s), intensity(i), on_period(o),
            direction(d), horizontal_angle(ha), vertical_angle(va),
            animation_params(a)
        { }

        // Basic Light parameters
        SGVec3f position;
        SGVec4f color;
        double size, intensity;
        int on_period; // 0 - all day, 1 - turn on exactly at sunset for night, 2 - turn on randomly around sunset, 3 - night + low visibility

        // Directionality parameters
        SGVec3f direction;
        double horizontal_angle, vertical_angle; // If both angles are 360, the light is treated as omni-directional

        /*
        * animation_params defines the animation (if any) that the light has to be
        * rendered with. It consists of 4 parameters:
        * 1. animation_params[0] - interval: Length of the animation (in seconds) - i.e. the animation will loop every `interval` seconds. Animation is disabled if `interval` is less than equal to 0.
        * 2. animation_params[1] - on_portion: Portion of the interval the light should functional, i.e. a value of 0.5 implies that the light will only function in the first half of the interval.
        * 3. animation_params[2] - strobe_rate: Number of light strobes in the entire `interval`. A value of 1 implies the light will turn on and off once within an `interval`, 2 implies the light will turn on, turn off, turn on, turn off in one `interval`. A value less than or equal to 0 implies an always on light.
        * 4. animation_params[3] - offset: Offset the interval start to allow for variety within multiple lights in the same scene, or specific multi-light animations like the RABBIT near the runway. A value less than 0 implies randomized offset.
        */
        SGVec4f animation_params;
    };

    typedef std::vector<Light> LightList;

    LightBin() = default;
    LightBin(const SGPath& absoluteFileName);

    ~LightBin() = default;

    void insert(const Light& light);
    void insert(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c);
    void insert(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c, const SGVec4f& a);
    void insert(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c, const SGVec3f& d, const double& ha, const double& va);
    void insert(const SGVec3f& p, const double& s, const double& i, const int& o, const SGVec4f& c, const SGVec3f& d, const double& ha, const double& va, const SGVec4f& a);

    unsigned getNumLights() const;
    const Light& getLight(unsigned i) const;
    double getAverageSize() const;
    double getAverageIntensity() const;
private:
    LightList _lights;

    double _aggregated_size, _aggregated_intensity;
};

osg::ref_ptr<simgear::EffectGeode> createLights(LightBin& lightList, const osg::Matrix& transform, const SGReaderWriterOptions* options);
};
#endif
