/*
 * SPDX-FileName: galaxy.hxx
 * SPDX-FileComment: model the celestial sphere brightness by unresolved sources
 * SPDX-FileContributor: Chris Ringeval. Started November 2021.
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <osg/ref_ptr>
#include <osg/MatrixTransform>
#include <osg/Node>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>

namespace simgear {
class SGReaderWriterOptions;
}

class SGGalaxy : public SGReferenced {
public:
    SGGalaxy(SGPropertyNode* props = nullptr);

    // build the galaxy object
    osg::Node* build(double galaxy_size, const simgear::SGReaderWriterOptions* options);

    // basic repainting according to sky lighting
    bool repaint(double sun_angle, double altitude_m);

private:
    osg::ref_ptr<osg::MatrixTransform> galaxy_transform;
    osg::ref_ptr<osg::Uniform> zenith_brightness_magnitude;

    SGPropertyNode_ptr _magDarkSkyProperty;

    // the darkest sky at zenith has a brightness equals to (in
    // magnitude per arcsec^2 for the V band)
    const double _magDarkSkyDefault{22.0};
};
