// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2008 Nicolas Vivien

/**
 * @file
 * @brief Precipitation effects to draw rain and snow.
 */

#ifndef _PRECIPITATION_HXX
#define _PRECIPITATION_HXX

#include <osg/Group>
#include <osg/Referenced>
#include <osgParticle/PrecipitationEffect>


class SGPrecipitation : public osg::Referenced
{
protected:
    bool _freeze;
    bool _enabled;
    bool _droplet_external;

    float _snow_intensity;
    float _rain_intensity;
    float _clip_distance;
    float _rain_droplet_size;
    float _snow_flake_size;
    float _illumination;
	
    osg::Vec3 _wind_vec;
	
    osg::ref_ptr<osgParticle::PrecipitationEffect> _precipitationEffect;

public:
    SGPrecipitation();
    virtual ~SGPrecipitation() {}
    osg::Group* build(void);
    bool update(void);
	
    void setWindProperty(double, double);
    void setFreezing(bool);
    void setDropletExternal(bool);
    void setRainIntensity(float);
    void setSnowIntensity(float);
    void setRainDropletSize(float);
    void setSnowFlakeSize(float);
    void setIllumination(float);
    void setClipDistance(float);

    void setEnabled(bool);
    bool getEnabled() const;
};

#endif
