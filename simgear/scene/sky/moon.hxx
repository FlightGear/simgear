/*
 * SPDX-FileName: moon.hxx
 * SPDX-FileComment: model earth's moon
 * SPDX-FileContributor: Written by Durk Talsma. Originally started October 1997.
 * SPDX-FileContributor: Based upon algorithms and data kindly provided by Mr. Paul Schlyter (pausch@saaf.se).
 * SPDX-FileContributor: Separated out rendering pieces and converted to ssg by Curt Olson, March 2000.
 * SPDX-FileContributor: Ported to the OpenGL core profile by Fernando García Liñán, 2024.
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <osg/ref_ptr>
#include <osg/MatrixTransform>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>

namespace simgear {
class SGReaderWriterOptions;
}

class SGMoon : public SGReferenced {
public:
    SGMoon() = default;

    osg::Node* build(double moon_size, const simgear::SGReaderWriterOptions* options);

    /*
     * Reposition the moon at the specified right ascension and declination, at
     * moon_dist_bare*moon_dist_factor from the center of Earth. lst, lat and
     * alt are need to estimate where is the center of Earth from the current
     * view).
     */
    bool reposition(double rightAscension, double declination,
                    double moon_dist_bare, double moon_dist_factor,
                    double lst, double lat, double alt);

private:
    osg::ref_ptr<osg::MatrixTransform> moon_transform;
};
