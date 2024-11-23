/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "planets.hxx"

#include <osg/Geometry>

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>

using namespace simgear;

osg::Node* SGPlanets::build(int num, const SGVec3d* planet_data, double planet_dist,
                            const SGReaderWriterOptions* options)
{
    osg::ref_ptr<EffectGeode> geode = new EffectGeode;
    geode->setName("Stars");

    Effect* effect = makeEffect("Effects/stars", true, options);
    if (effect) {
        geode->setEffect(effect);
    }

    osg::ref_ptr<osg::Vec4Array> vl = new osg::Vec4Array;
    for (int i = 0; i < num; ++i) {
        // Position the planet arbitrarily far away
        osg::Vec3 pos(planet_dist * std::cos(planet_data[i][0]) * std::cos(planet_data[i][1]),
                      planet_dist * std::sin(planet_data[i][0]) * std::cos(planet_data[i][1]),
                      planet_dist * std::sin(planet_data[i][1]));

        // Compute the irradiance in W * m^-2 given the magnitude. Discount the
        // atmospheric absorption from the magnitude (0.4).
        double irradiance = std::pow(10, 0.4 * (-planet_data[i][2] - 19 + 0.4));

        // The vertex array contains the position in xyz and irradiance in w
        vl->push_back(osg::Vec4(pos, irradiance));
    }

    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    geometry->setUseVertexBufferObjects(true);
    geometry->setVertexArray(vl);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, num));

    geode->addDrawable(geometry);

    return geode.release();
}
