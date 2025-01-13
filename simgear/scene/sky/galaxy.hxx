/*
 * SPDX-FileName: galaxy.hxx
 * SPDX-FileComment: model the celestial sphere brightness by unresolved sources
 * SPDX-FileContributor: Chris Ringeval. Started November 2021.
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <osg/ref_ptr>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>

namespace simgear {
class SGReaderWriterOptions;
}

class SGGalaxy : public SGReferenced {
public:
    SGGalaxy() = default;

    // build the galaxy object
    osg::Node* build(double galaxy_size, const simgear::SGReaderWriterOptions* options);
};
