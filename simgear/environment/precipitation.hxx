/**
 * @file precipitation.hxx
 * @author Nicolas VIVIEN
 * @date 2008-02-10
 *
 * @note Copyright (C) 2008 Nicolas VIVIEN
 *
 * @brief Precipitation effects to draw rain and snow.
 *
 * @par Licences
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of the
 *   License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _PRECIPITATION_HXX
#define _PRECIPITATION_HXX

#include <osg/Group>
#include <osg/Referenced>
#include <osgParticle/PrecipitationEffect>


class SGPrecipitation : public osg::Referenced
{
private:
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

    void setEnabled( bool );
    bool getEnabled() const;
};

#endif
