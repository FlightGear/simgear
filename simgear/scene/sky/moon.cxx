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

    osg::Node* orb = SGMakeSphere(moon_size, 40, 20);
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
// declination from the center of Earth. Because the view is actually
// offset by our current position (p), we first evaluate our current
// position w.r.t the Moon and then shift to the articial center of
// earth before shifting to the rendered moon distance. This allows to
// implement any parallax effects.  moon_dist_bare is expected to not
// change during the rendering, it gives us the normalisation factors
// between real distances and units used in the
// rendering. moon_dist_fact is any extra factors to put the moon
// further or closer.


bool SGMoon::reposition( double rightAscension, double declination,
			 double moon_dist_bare, double moon_dist_factor,
			 double lst, double lat, double alt )
{
    osg::Matrix TE, T2, RA, DEC;

    // semi mayor axis
    const double moon_a_in_rearth = 60.266600;
    // average (we could also account for equatorial streching like in
    // oursun.cxx if required)
    const double earth_radius_in_meters = 6371000.0;
    //local hour angle in radians
    double lha;
    // in unit of the rendering
    double moon_dist;
    double earth_radius, viewer_radius;
    double xp,yp,zp;

    //rendered earth radius according to what has been specified by
    //moon_dist_bare
    earth_radius = moon_dist_bare/moon_a_in_rearth;

    //how far are we from the center of Earth
    viewer_radius = (1.0 + alt/earth_radius_in_meters)*earth_radius;

    // the local hour angle of the moon, .i.e. its angle with respect
    // to the meridian of the viewer
    lha = lst - rightAscension;

    // the shift vector of the observer w.r.t. earth center (funny
    // convention on x?)
    zp = viewer_radius * sin(lat);
    yp = viewer_radius * cos(lat)*cos(lha);
    xp = viewer_radius * cos(lat)*sin(-lha);

    //rotate along the z axis
    RA.makeRotate(rightAscension - 90.0 * SGD_DEGREES_TO_RADIANS,
                  osg::Vec3(0, 0, 1));
    //rotate along the rotated x axis
    DEC.makeRotate(declination, osg::Vec3(1, 0, 0));

    //move to the center of Earth
    TE.makeTranslate(osg::Vec3(-xp,-yp,-zp));

    //move the moon from the center of Earth to moon_dist
    moon_dist = moon_dist_bare * moon_dist_factor;
    T2.makeTranslate(osg::Vec3(0, moon_dist, 0));

    // cout << " viewer radius= " << viewer_radius << endl;
    // cout << " xp yp zp= " << xp <<" " << yp << " " << zp << endl;
    // cout << " lha= " << lha << endl;
    // cout << " moon_dist_bare= " << moon_dist_bare << endl;
    // cout << " moon_dist_factor= " << moon_dist_factor << endl;
    // cout << " moon_dist= " << moon_dist << endl;

    moon_transform->setMatrix(T2*TE*DEC*RA);


    return true;
}

