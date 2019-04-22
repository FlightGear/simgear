// stars.cxx -- model the stars
//
// Written by Durk Talsma. Originally started October 1997, for distribution  
// with the FlightGear project. Version 2 was written in August and 
// September 1998. This code is based upon algorithms and data kindly 
// provided by Mr. Paul Schlyter. (pausch@saaf.se). 
//
// Separated out rendering pieces and converted to ssg by Curt Olson,
// March 2000
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
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/props/props.hxx>

#include <stdio.h>
#include <iostream>
#include <algorithm>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Image>
#include <osg/Material>
#include <osg/Point>
#include <osg/ShadeModel>
#include <osg/Node>

#include "stars.hxx"

// Constructor
SGStars::SGStars( SGPropertyNode* props ) :
    old_phase(-1)
{
    if (props) {
        // don't create here - if it's not defined, we won't use the cutoff
        // from a property
        _cutoffProperty = props->getNode("star-magnitude-cutoff");
    }
}


// Destructor
SGStars::~SGStars( void ) {
}


// initialize the stars object and connect it into our scene graph root
osg::Node*
SGStars::build( int num, const SGVec3d star_data[], double star_dist ) {
    osg::Geode* geode = new osg::Geode;
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-9, "RenderBin");

    // set up the star state
    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setFunction(osg::BlendFunc::SRC_ALPHA,
                           osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    stateSet->setAttributeAndModes(blendFunc);

//     osg::Point* point = new osg::Point;
//     point->setSize(5);
//     stateSet->setAttributeAndModes(point);

    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);

    // Build scenegraph structure
    
    cl = new osg::Vec4Array;
    osg::Vec3Array* vl = new osg::Vec3Array;

    // Build scenegraph structure
    for ( int i = 0; i < num; ++i ) {
        // position seeded to arbitrary values
        vl->push_back(osg::Vec3(star_dist * cos( star_data[i][0])
                                * cos( star_data[i][1] ),
                                star_dist * sin( star_data[i][0])
                                * cos( star_data[i][1] ),
                                star_dist * sin( star_data[i][1])));

        // color (magnitude)
        cl->push_back(osg::Vec4(1, 1, 1, 1));
    }

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(vl);
    geometry->setColorArray(cl.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vl->size()));
    geode->addDrawable(geometry);

    return geode;
}


// repaint the sun colors based on current value of sun_angle in
// degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight

bool SGStars::repaint( double sun_angle, int num, const SGVec3d star_data[] )
{
    double mag, nmag, alpha, factor, cutoff;

    /*
    maximal magnitudes under dark sky on Earth, from Eq.(90) and (91) of astro-ph/1405.4209
	For (18 < musky < 20)
	mmax = 0.27 musky + 0.8 - 2.5 * log(F)

	For (19.5  µsky  22)
	mmax = 0.383 musky - 1.44 - 2.5 * log(F)

    
	Let's take F = 1.4 for healthy young pilot
	mudarksky ~ 22 mag/arcsec^2 => mmax=6.2
	muastrotwilight ~ 20 mag/arsec^2 => mmax=5.4
	mu99deg ~ 17.5 mag/arcsec^2 => mmax=4.7
	mu97.5deg ~ 16 mag/arcsec^2 => ? let's keep it rough
    */

    double mag_nakedeye = 6.2;
    double mag_twilight_astro = 5.4;
    double mag_twilight_nautic = 4.7;

    // sirius, brightest star (not brightest object)
    double mag_min = -1.46;
    
    int phase;

    // determine which star structure to draw
    if ( sun_angle > (SGD_PI_2 + 18.0 * SGD_DEGREES_TO_RADIANS ) ) {
        // deep night, atmosphere is not lighten by the sun
        factor = 1.0;
        cutoff = mag_nakedeye;
        phase = 0;
    } else if ( sun_angle > (SGD_PI_2 + 12.0 * SGD_DEGREES_TO_RADIANS ) ) {
        // less than 18deg and more than 12deg is astronomical twilight
        factor = 1.0;
        cutoff = mag_twilight_astro;
        phase = 1;
    } else if ( sun_angle > (SGD_PI_2 + 9.0 * SGD_DEGREES_TO_RADIANS ) ) {
        // less 12deg and more than 6deg is is nautical twilight 
        factor = 1.0;
        cutoff = mag_twilight_nautic;
        phase = 2;
    } else if ( sun_angle > (SGD_PI_2 + 7.5 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.95;
        cutoff = 3.1;
        phase = 3;
    } else if ( sun_angle > (SGD_PI_2 + 7.0 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.9;
        cutoff = 2.4;
        phase = 4;
    } else if ( sun_angle > (SGD_PI_2 + 6.5 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.85;
        cutoff = 1.8;
        phase = 5;
    } else if ( sun_angle > (SGD_PI_2 + 6.0 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.8;
        cutoff = 1.2;
        phase = 6;
    } else if ( sun_angle > (SGD_PI_2 + 5.5 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.75;
        cutoff = 0.6;
        phase = 7;
    } else {
        // early dusk or late dawn
        factor = 0.7;
        cutoff = 0.0;
        phase = 8;
    }
    
    if (_cutoffProperty) {
        double propCutoff = _cutoffProperty->getDoubleValue();
        cutoff = std::min(propCutoff, cutoff);
    }
    
    if ((phase != old_phase) || (cutoff != _cachedCutoff)) {
	// cout << "  phase change, repainting stars, num = " << num << endl;
        old_phase = phase;
        _cachedCutoff = cutoff;

        for ( int i = 0; i < num; ++i ) {
            // if ( star_data[i][2] < min ) { min = star_data[i][2]; }
            // if ( star_data[i][2] > max ) { max = star_data[i][2]; }

            // magnitude ranges from -1 (bright) to 6 (dim).  The
            // range of star and planet magnitudes can actually go
            // outside of this, but for our purpose, if it is brighter
            // that magmin, we'll color it full white/alpha anyway

            // color (magnitude)
            mag = star_data[i][2];
            if ( mag < cutoff ) {
                nmag = ( cutoff - mag ) / (cutoff - mag_min); // translate to 0 ... 1.0 scale
                alpha = nmag * 0.85 + 0.15; // translate to a 0.15 ... 1.0 scale
                alpha *= factor;          // dim when the sun is brighter
            } else {
                alpha = 0.0;
            }

            if (alpha > 1.0) { alpha = 1.0; }
            if (alpha < 0.0) { alpha = 0.0; }

            (*cl)[i] = osg::Vec4(1, 1, 1, alpha);
        }
        cl->dirty();
    }

    // cout << "min = " << min << " max = " << max << " count = " << num 
    //      << endl;

    return true;
}
