/*
 * SPDX-FileName: stars.hxx
 * SPDX-FileComment: model the stars (and planets)
 * SPDX-FileContributor: Written by Durk Talsma. Originally started October 1997.
 * SPDX-FileContributor: Based upon algorithms and data kindly provided by Mr. Paul Schlyter (pausch@saaf.se).
 * SPDX-FileContributor: Separated out rendering pieces and converted to ssg by Curt Olson, March 2000.
 * SPDX-FileContributor: Ported to the OpenGL core profile by Fernando García Liñán, 2024.
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <osg/ref_ptr>
#include <osg/Array>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>
#include <simgear/props/propsfwd.hxx>

namespace simgear {
class SGReaderWriterOptions;
}

class SGStars : public SGReferenced {
public:
    SGStars(SGPropertyNode* props = nullptr);

    // initialize the stars structure
    osg::Node* build(int num, const SGVec3d star_data[], double star_dist,
                     simgear::SGReaderWriterOptions* options);

    /*
     * Repaint the star and planet magnitudes based on current value of
     * sun_angle in degrees relative to verticle (so we can make them
     * relatively dimmer during dawn and dusk
     * 0 degrees = high noon
     * 90 degrees = sun rise/set
     * 180 degrees = darkest midnight
     */
    bool repaint(double sun_angle, double altitude_m, int num,
                 const SGVec3d star_data[]);

private:
    osg::ref_ptr<osg::Vec4Array> cl;

    int old_phase{-1}; // data for optimization

    // the darkest sky at zenith has a brightness equals to (in
    // magnitude per arcsec^2 for the V band)
    const double _magDarkSkyDefault{22.0};

    double _cachedMagDarkSky{0.0};
    SGPropertyNode_ptr _magDarkSkyProperty;
};
