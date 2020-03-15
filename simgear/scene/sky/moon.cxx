// moon.hxx -- model earth's moon
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

#include <stdio.h>
#include <iostream>

#include <simgear/structure/OSGVersion.hxx>
#include <osg/Array>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Node>
#include <osg/ShadeModel>
#include <osg/TexEnv>
#include <osg/Texture2D>

#include <simgear/constants.h>
#include <simgear/screen/colors.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "sphere.hxx"
#include "moon.hxx"

using namespace simgear;

// Constructor
SGMoon::SGMoon( void ) :
    prev_moon_angle(-1)
{
}


// Destructor
SGMoon::~SGMoon( void ) {
}


// build the moon object
osg::Node*
SGMoon::build( SGPath path, double moon_size ) {

    osg::Node* orb = SGMakeSphere(moon_size, 15, 15);
    osg::StateSet* stateSet = orb->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-5, "RenderBin");

    // set up the orb state
    osg::ref_ptr<SGReaderWriterOptions> options;
    options = SGReaderWriterOptions::fromPath(path);

    osg::Texture2D* texture = SGLoadTexture2D("moon.png", options.get());
    stateSet->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    osg::TexEnv* texEnv = new osg::TexEnv;
    texEnv->setMode(osg::TexEnv::MODULATE);
    stateSet->setTextureAttribute(0, texEnv, osg::StateAttribute::ON);

    orb_material = new osg::Material;
    orb_material->setColorMode(osg::Material::DIFFUSE);
    orb_material->setDiffuse(osg::Material::FRONT_AND_BACK,
                             osg::Vec4(1, 1, 1, 1));
    orb_material->setAmbient(osg::Material::FRONT_AND_BACK,
                             osg::Vec4(0, 0, 0, 1));
    orb_material->setEmission(osg::Material::FRONT_AND_BACK,
                              osg::Vec4(0, 0, 0, 1));
    orb_material->setSpecular(osg::Material::FRONT_AND_BACK,
                              osg::Vec4(0, 0, 0, 1));
    orb_material->setShininess(osg::Material::FRONT_AND_BACK, 0);
    stateSet->setAttribute(orb_material.get());
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    osg::ShadeModel* shadeModel = new osg::ShadeModel;
    shadeModel->setMode(osg::ShadeModel::SMOOTH);
    stateSet->setAttributeAndModes(shadeModel);
    osg::CullFace* cullFace = new osg::CullFace;
    cullFace->setMode(osg::CullFace::BACK);
    stateSet->setAttributeAndModes(cullFace);

    osg::BlendFunc* blendFunc = new osg::BlendFunc;
    blendFunc->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE);
    stateSet->setAttributeAndModes(blendFunc);

    osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
    alphaFunc->setFunction(osg::AlphaFunc::GREATER);
    alphaFunc->setReferenceValue(0.01);
    stateSet->setAttribute(alphaFunc);
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);

    // force a repaint of the moon colors with arbitrary defaults
    repaint( 0.0 );

    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    moon_transform = new osg::MatrixTransform;

    moon_transform->addChild( orb );

    return moon_transform.get();
}


// repaint the moon colors based on current value of moon_angle in
// degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = moon rise/set
// 180 degrees = darkest midnight
bool SGMoon::repaint( double moon_angle ) {

    if (prev_moon_angle == moon_angle)
        return true;

    prev_moon_angle = moon_angle;

    float moon_factor = 4*cos(moon_angle);
    
    if (moon_factor > 1) moon_factor = 1.0;
    if (moon_factor < -1) moon_factor = -1.0;
    moon_factor = moon_factor/2 + 0.5;
    
    osg::Vec4 color;
    color[1] = sqrt(moon_factor);
    color[0] = sqrt(color[1]);
    color[2] = moon_factor * moon_factor;
    color[2] *= color[2];
    color[3] = 1.0;
    
    gamma_correct_rgb( color._v );

    orb_material->setDiffuse(osg::Material::FRONT_AND_BACK, color);

    return true;
}


// reposition the moon at the specified right ascension and
// declination, offset by our current position (p) so that it appears
// fixed at a great distance from the viewer.  Also add in an optional
// rotation (i.e. for the current time of day.)
bool SGMoon::reposition( double rightAscension, double declination,
			 double moon_dist )
{
    osg::Matrix T2, RA, DEC;

    RA.makeRotate(rightAscension - 90.0 * SGD_DEGREES_TO_RADIANS,
                  osg::Vec3(0, 0, 1));

    DEC.makeRotate(declination, osg::Vec3(1, 0, 0));

    T2.makeTranslate(osg::Vec3(0, moon_dist, 0));

    moon_transform->setMatrix(T2*DEC*RA);

    return true;
}

