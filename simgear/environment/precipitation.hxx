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
 * @par CVS
 *   $Id$
 */

#ifndef _PRECIPITATION_HXX
#define _PRECIPITATION_HXX

#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgUtil/Optimizer>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>

#include <osg/Depth>
#include <osg/Stencil>
#include <osg/ClipPlane>
#include <osg/ClipNode>
#include <osg/MatrixTransform>
#include <osgUtil/TransformCallback>
#include <osgParticle/PrecipitationEffect>


class SGPrecipitation {
private:
	bool _freeze;

	float _snow_intensity;
	float _rain_intensity;
	
	int _wind_dir;
	osg::Vec3 _wind_vec;
	
	osg::Group *group;
	osg::ref_ptr<osgParticle::PrecipitationEffect> precipitationEffect;

public:
	SGPrecipitation();
	~SGPrecipitation();
	osg::Group* build(void);
	bool update(void);
	
	void setWindProperty(double, double);
	void setFreezing(bool);
	void setRainIntensity(float);
	void setSnowIntensity(float);
};

#endif
