/*
 * SPDX-FileName: oursun.cxx
 * SPDX-FileComment: model earth's sun
 * SPDX-FileContributor: Written by Durk Talsma. Originally started October 1997.
 * SPDX-FileContributor: Based upon algorithms and data kindly provided by Mr. Paul Schlyter (pausch@saaf.se).
 * SPDX-FileContributor: Separated out rendering pieces and converted to ssg by Curt Olson, March 2000.
 * SPDX-FileContributor: Ported to the OpenGL core profile by Fernando García Liñán, 2024.
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <osg/Array>

#include <simgear/math/SGMath.hxx>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

#include "oursun.hxx"

using namespace simgear;

osg::Node* SGSun::build(double sun_size, SGPropertyNode *property_tree_Node,
                        const SGReaderWriterOptions* options)
{
    env_node = property_tree_Node;

    osg::ref_ptr<EffectGeode> geode = new EffectGeode;
    geode->setName("Sun");

    Effect* effect = makeEffect("Effects/oursun", true, options);
    if (effect) {
        geode->setEffect(effect);
    }

    osg::ref_ptr<osg::Vec3Array> sun_vl = new osg::Vec3Array;
    sun_vl->push_back(osg::Vec3(-sun_size, 0.0f, -sun_size));
    sun_vl->push_back(osg::Vec3( sun_size, 0.0f, -sun_size));
    sun_vl->push_back(osg::Vec3(-sun_size, 0.0f,  sun_size));
    sun_vl->push_back(osg::Vec3( sun_size, 0.0f,  sun_size));

    osg::ref_ptr<osg::Vec2Array> sun_tl = new osg::Vec2Array;
    sun_tl->push_back(osg::Vec2(0.0f, 0.0f));
    sun_tl->push_back(osg::Vec2(1.0f, 0.0f));
    sun_tl->push_back(osg::Vec2(0.0f, 1.0f));
    sun_tl->push_back(osg::Vec2(1.0f, 1.0f));

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    geometry->setUseVertexBufferObjects(true);
    geometry->setVertexArray(sun_vl);
    geometry->setTexCoordArray(0, sun_tl, osg::Array::BIND_PER_VERTEX);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 4));

    geode->addDrawable(geometry);

    sun_transform = new osg::MatrixTransform;
    sun_transform->setName("Sun transform");
    sun_transform->addChild(geode);

    return sun_transform.get();
}

bool SGSun::reposition(double rightAscension, double declination,
                       double sun_dist, double lat, double alt_asl, double sun_angle)
{
    // GST - GMT sidereal time
    osg::Matrix T2, RA, DEC;
    RA.makeRotate(rightAscension - 90*SGD_DEGREES_TO_RADIANS, osg::Vec3(0, 0, 1));
    DEC.makeRotate(declination, osg::Vec3(1, 0, 0));
    T2.makeTranslate(osg::Vec3(0, sun_dist, 0));
    sun_transform->setMatrix(T2*DEC*RA);

    // Push some data to the property tree, so it can be used in the enviromental code
    if (prev_sun_angle != sun_angle) {
        prev_sun_angle = sun_angle;
        if (sun_angle == 0.0) sun_angle = 0.1;

        const double r_earth_pole = 6356752.314;
        const double r_tropo_pole = 6356752.314 + 8000.0;
        const double epsilon_earth2 = 6.694380066E-3;
        const double epsilon_tropo2 = 9.170014946E-3;

        double r_tropo = r_tropo_pole / sqrt(1.0 - (epsilon_tropo2 * pow(cos(lat), 2)));
        double r_earth = r_earth_pole / sqrt(1.0 - (epsilon_earth2 * pow(cos(lat), 2)));

        double position_radius = r_earth + alt_asl;

        double gamma = SG_PI - sun_angle;
        double sin_beta =  (position_radius * sin(gamma)) / r_tropo;
        if (sin_beta > 1.0) sin_beta = 1.0;
        double alpha =  SG_PI - gamma - asin(sin_beta);

        // OK, now let's calculate the distance the light travels
        double path_distance = sqrt(pow(position_radius, 2) + pow(r_tropo, 2)
                                    - (2.0 * position_radius * r_tropo * cos(alpha)));

        double alt_half = sqrt(pow(r_tropo, 2) + pow(path_distance / 2.0, 2) - r_tropo * path_distance
                               * cos(asin(sin_beta))) - r_earth;

        if (alt_half < 0.0) alt_half = 0.0;

        if (env_node){
            env_node->setDoubleValue("atmosphere/altitude-troposphere-top", r_tropo - r_earth);
            env_node->setDoubleValue("atmosphere/altitude-half-to-sun", alt_half);
        }
    }

    return true;
}
