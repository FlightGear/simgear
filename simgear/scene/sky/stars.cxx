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

#include <stdio.h>
#include STL_IOSTREAM

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Image>
#include <osg/Material>
#include <osg/Point>
#include <osg/ShadeModel>

#include "stars.hxx"

// Constructor
SGStars::SGStars( void ) :
old_phase(-1)
{
}


// Destructor
SGStars::~SGStars( void ) {
}


// initialize the stars object and connect it into our scene graph root
osg::Node*
SGStars::build( int num, const SGVec3d star_data[], double star_dist ) {
    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    stars_transform = new osg::MatrixTransform;

    osg::Geode* geode = new osg::Geode;
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-9, "RenderBin");

    // set up the star state

    // Ok, the old implementation did have a color material set.
    // But with lighting off, I don't see how this should change the result
    osg::Material* material = new osg::Material;
//     material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
//     material->setEmission(osg::Material::FRONT_AND_BACK,
//                           osg::Vec4(0, 0, 0, 1));
//     material->setSpecular(osg::Material::FRONT_AND_BACK,
//                               osg::Vec4(0, 0, 0, 1));
    stateSet->setAttribute(material);
//     stateSet->setMode(GL_COLOR_MATERIAL, osg::StateAttribute::OFF);

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

    stars_transform->addChild(geode);

    SG_LOG( SG_EVENT, SG_INFO, "stars = " << stars_transform.get());

    return stars_transform.get();
}


// repaint the sun colors based on current value of sun_angle in
// degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool SGStars::repaint( double sun_angle, int num, const SGVec3d star_data[] ) {
    // cout << "repainting stars" << endl;
    // double min = 100;
    // double max = -100;
    double mag, nmag, alpha, factor, cutoff;

    int phase;

    // determine which star structure to draw
    if ( sun_angle > (SGD_PI_2 + 10.0 * SGD_DEGREES_TO_RADIANS ) ) {
        // deep night
        factor = 1.0;
        cutoff = 4.5;
        phase = 0;
    } else if ( sun_angle > (SGD_PI_2 + 8.8 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 1.0;
        cutoff = 3.8;
        phase = 1;
    } else if ( sun_angle > (SGD_PI_2 + 7.5 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.95;
        cutoff = 3.1;
        phase = 2;
    } else if ( sun_angle > (SGD_PI_2 + 7.0 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.9;
        cutoff = 2.4;
        phase = 3;
    } else if ( sun_angle > (SGD_PI_2 + 6.5 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.85;
        cutoff = 1.8;
        phase = 4;
    } else if ( sun_angle > (SGD_PI_2 + 6.0 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.8;
        cutoff = 1.2;
        phase = 5;
    } else if ( sun_angle > (SGD_PI_2 + 5.5 * SGD_DEGREES_TO_RADIANS ) ) {
        factor = 0.75;
        cutoff = 0.6;
        phase = 6;
    } else {
        // early dusk or late dawn
        factor = 0.7;
        cutoff = 0.0;
        phase = 7;
    }

    if( phase != old_phase ) {
	// cout << "  phase change, repainting stars, num = " << num << endl;
        old_phase = phase;
        for ( int i = 0; i < num; ++i ) {
            // if ( star_data[i][2] < min ) { min = star_data[i][2]; }
            // if ( star_data[i][2] > max ) { max = star_data[i][2]; }

            // magnitude ranges from -1 (bright) to 4 (dim).  The
            // range of star and planet magnitudes can actually go
            // outside of this, but for our purpose, if it is brighter
            // that -1, we'll color it full white/alpha anyway and 4
            // is a convenient cutoff point which keeps the number of
            // stars drawn at about 500.

            // color (magnitude)
            mag = star_data[i][2];
            if ( mag < cutoff ) {
                nmag = ( 4.5 - mag ) / 5.5; // translate to 0 ... 1.0 scale
                // alpha = nmag * 0.7 + 0.3; // translate to a 0.3 ... 1.0 scale
                alpha = nmag * 0.85 + 0.15; // translate to a 0.15 ... 1.0 scale
                alpha *= factor;          // dim when the sun is brighter
            } else {
                alpha = 0.0;
            }

            if (alpha > 1.0) { alpha = 1.0; }
            if (alpha < 0.0) { alpha = 0.0; }

            (*cl)[i] = osg::Vec4(1, 1, 1, alpha);
            // cout << "alpha[" << i << "] = " << alpha << endl;
        }
        cl->dirty();
    } else {
	// cout << "  no phase change, skipping" << endl;
    }

    // cout << "min = " << min << " max = " << max << " count = " << num 
    //      << endl;

    return true;
}


// reposition the stars for the specified time (GST rotation),
// offset by our current position (p) so that it appears fixed at a
// great distance from the viewer.
bool
SGStars::reposition( const SGVec3f& p, double angle )
{
    osg::Matrix T1, GST;
    T1.makeTranslate(p.osg());

    GST.makeRotate(angle*SGD_DEGREES_TO_RADIANS, osg::Vec3(0, 0, -1));

    stars_transform->setMatrix(GST*T1);

    return true;
}
