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

#include <simgear/ephemeris/stardata.hxx>
#include <simgear/structure/SGReferenced.hxx>

namespace simgear {
class SGReaderWriterOptions;
}

class SGStars : public SGReferenced {
public:
    SGStars() = default;

    // initialize the stars structure
    osg::Node* build(int num, const SGStarData::Star* star_data, double star_dist,
                     const simgear::SGReaderWriterOptions* options);
};
