/**
 * @file precipitation.cxx
 * @author Nicolas VIVIEN
 * @date 2008-02-10
 *
 * @note Copyright (C) 2008 Nicolas VIVIEN
 *
 * @brief Precipitation effects to draw rain and snow.
 *
 * @par License
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

#include "precipitation.hxx"

#include <simgear/constants.h>
#include <osg/ClipNode>
#include <simgear/scene/util/RenderConstants.hxx>

/**
 * @brief SGPrecipitation constructor
 *
 * Build a new OSG object from osgParticle.
 */
SGPrecipitation::SGPrecipitation() :
    _freeze(false), _enabled(true), _droplet_external(false), _snow_intensity(0.0), _rain_intensity(0.0), _clip_distance(5.0), _rain_droplet_size(0.0), _snow_flake_size(0.0), _illumination(1.0)
{
    _precipitationEffect = new osgParticle::PrecipitationEffect;
}

void SGPrecipitation::setEnabled( bool value )
{
    _enabled = value;
    if (!_enabled) {
        _precipitationEffect->snow(0);
        _precipitationEffect->rain(0);
    }
}

void SGPrecipitation::setDropletExternal( bool value )
{
    _droplet_external = value;
}

bool SGPrecipitation::getEnabled() const
{
    return _enabled;
}

/**
 * @brief Build and add the object "precipitationEffect"
 *
 * This function permits you to create an object precipitationEffect and initialize it.
 * I define by default the color of water (for raining)
 */
osg::Group* SGPrecipitation::build(void)
{
    if (!_enabled) 
        return nullptr;

    osg::ref_ptr<osg::Group> group = new osg::Group;

    _precipitationEffect->snow(0);	
    _precipitationEffect->rain(0);	

    if (_clip_distance!=0.0)
    {
        osg::ref_ptr<osg::ClipNode> clipNode = new osg::ClipNode;
        osg::ref_ptr<osg::ClipPlane> clipPlane = new osg::ClipPlane(0);
        clipNode->addClipPlane(clipPlane.get());
        clipNode->getClipPlane(0)->setClipPlane(0.0, 0.0, -1.0, -_clip_distance);
        clipNode->setReferenceFrame(osg::ClipNode::ABSOLUTE_RF);
        clipNode->addChild(_precipitationEffect.get());

        group->addChild(clipNode.get());
    }
    else
    {
        group->addChild(_precipitationEffect.get());
    }

    group->setNodeMask( ~(simgear::CASTSHADOW_BIT | simgear::MODELLIGHT_BIT) );

    return group.release();
}


/**
 * @brief Define the snow intensity
 *
 * This function permits you to define and change the snow intensity
 * The param 'intensity' is normed (0 to 1).
 */
void SGPrecipitation::setSnowIntensity(float intensity)
{
    if (this->_snow_intensity < intensity-0.001)
        this->_snow_intensity += 0.001;
    else if (this->_snow_intensity > intensity+0.001)
        this->_snow_intensity -= 0.001;
    else
        this->_snow_intensity = intensity;
}


/**
 * @brief Define the rain intensity
 *
 * This function permits you to define and change the rain intensity
 * The param 'intensity' is normed (0 to 1).
 */
void SGPrecipitation::setRainIntensity(float intensity)
{
    if (this->_rain_intensity < intensity-0.001)
        this->_rain_intensity += 0.001;
    else if (this->_rain_intensity > intensity+0.001)
        this->_rain_intensity -= 0.001;
    else
        this->_rain_intensity = intensity;
}

/**
 * @brief Define the rain droplet size
 *
 * This function permits you to define and change the rain droplet size
 * which is used if external droplet size control is enabled
 */

void SGPrecipitation::setRainDropletSize(float size)
{
    _rain_droplet_size = size;
}


/**
 * @brief Define the illumination multiplier
 *
 * This function permits you to define and change the brightness
 * of the precipitation.
 */

void SGPrecipitation::setIllumination(float illumination)
{
    _illumination = illumination;
}

/**
 * @brief Define the snow flake size
 *
 * This function permits you to define and change the snow flake size
 * which is used if external droplet size control is enabled
 */

void SGPrecipitation::setSnowFlakeSize(float size)
{
    _snow_flake_size = size;
}


/**
 * @brief Define the clip plane distance
 *
 * This function permits you to define and change the clip plane distance.
 */

void SGPrecipitation::setClipDistance(float distance)
{
    _clip_distance = distance;
}

/**
 * @brief Freeze the rain to snow
 *
 * @param freeze Boolean
 *
 * This function permits you to turn off the rain to snow.
 */
void SGPrecipitation::setFreezing(bool freeze)
{
    if ((this->_freeze)&&(!freeze)) // rain freezes suddenly, so we need to unfreeze
	{
	this->_rain_intensity = this->_snow_intensity;
	this->_snow_intensity = 0.0;
	}
    this->_freeze = freeze;

}


/**
 * @brief Define the wind direction and speed
 *
 * This function permits you to define and change the wind direction
 *
 * After apply the MatrixTransform to the osg::Precipitation object,
 * x points full south... From wind heading and speed, we can calculate
 * the wind vector.
 */
void SGPrecipitation::setWindProperty(double heading, double speed)
{
    double x, y, z;

    heading = (heading + 180) * SGD_DEGREES_TO_RADIANS;
    speed = speed * SG_FEET_TO_METER;

    x = -cos(heading) * speed;
    y = sin(heading) * speed;
    z = 0;

    this->_wind_vec = osg::Vec3(x, y, z);
}


/**
 * @brief Update the precipitation effects
 *
 * This function permits you to update the precipitation effects.
 * Be careful, if snow and rain intensity are greater than '0', snow effect
 * will be first.
 *
 * The settings come from the osgParticle/PrecipitationEffect.cpp example.
 */
bool SGPrecipitation::update(void)
{
    if (!_enabled)
        return false;

    if (this->_freeze) {
        if (this->_rain_intensity > 0) {
		this->_snow_intensity = this->_rain_intensity;
	}
    }

    if (_enabled && this->_snow_intensity > 0) {
        _precipitationEffect->setWind(_wind_vec);

		
	if(_droplet_external)
		{

		if ((_freeze) && (_rain_droplet_size > 0.03)) // this is hail or sleet
			{
			_precipitationEffect->setParticleSize(_rain_droplet_size*1.5f);
			_precipitationEffect->setParticleSpeed( -1.0f - 22.36f*sqrtf(_rain_droplet_size));
        		_precipitationEffect->setMaximumParticleDensity(_snow_intensity * 4.8f);
			}
		else if (_freeze) // this is snow from frozen small rain droplets
			{
			_precipitationEffect->setParticleSize(_rain_droplet_size*1.3f);
			_precipitationEffect->setParticleSpeed( -0.75f - 0.25f*_snow_intensity);
        		_precipitationEffect->setMaximumParticleDensity(_snow_intensity * 10.0f);
			}
		else // this was snow in the first place
			{
			_precipitationEffect->setParticleSize(_snow_flake_size);
			_precipitationEffect->setParticleSpeed( -0.75f - 0.25f*_snow_intensity);
        		_precipitationEffect->setMaximumParticleDensity(_snow_intensity * 7.2f);
			}
		}
	else
		{
        	_precipitationEffect->setMaximumParticleDensity(_snow_intensity * 7.2f);
        	_precipitationEffect->setParticleSize(0.02f + 0.03f*_snow_intensity);
		_precipitationEffect->setParticleSpeed( -0.75f - 0.25f*_snow_intensity);
		}


        _precipitationEffect->setCellSize(osg::Vec3(5.0f / (0.25f+_snow_intensity), 5.0f / (0.25f+_snow_intensity), 5.0f));
		
        _precipitationEffect->setNearTransition(25.f);
        _precipitationEffect->setFarTransition(100.0f - 60.0f*sqrtf(_snow_intensity));
		
        _precipitationEffect->setParticleColor(osg::Vec4(0.85 * _illumination, 0.85 * _illumination, 0.85 * _illumination, 1.0) - osg::Vec4(0.1, 0.1, 0.1, 1.0) * _snow_intensity);
    } else if (_enabled && this->_rain_intensity > 0) {

        _precipitationEffect->setWind(_wind_vec);

		
	if(_droplet_external)
		{
		_precipitationEffect->setParticleSize(_rain_droplet_size);
		_precipitationEffect->setParticleSpeed( -1.0f - 22.36f*sqrtf(_rain_droplet_size));
		}
	else
		{
		_precipitationEffect->setParticleSize(0.01 + 0.02*_rain_intensity);
        	_precipitationEffect->setParticleSpeed( -2.0f + -5.0f*_rain_intensity);
		}
		
        _precipitationEffect->setMaximumParticleDensity(_rain_intensity * 7.5f);
        _precipitationEffect->setCellSize(osg::Vec3(5.0f / (0.25f+_rain_intensity), 5.0f / (0.25f+_rain_intensity), 5.0f));
		
        _precipitationEffect->setNearTransition(25.f);
        _precipitationEffect->setFarTransition(100.0f - 60.0f*sqrtf(_rain_intensity));
		
	_precipitationEffect->setParticleColor( osg::Vec4(0.64 * _illumination, 0.64 * _illumination, 0.64 * _illumination, 0.5));
    } else {
        _precipitationEffect->snow(0);	
        _precipitationEffect->rain(0);	
    }

    return true;
}
