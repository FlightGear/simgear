/*
 * SPDX-FileCopyrightText: Copyright (C) 2024 Fernando García Liñán
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <osg/ref_ptr>

#include <simgear/math/SGVec3.hxx>
#include <simgear/structure/SGReferenced.hxx>

namespace simgear {
class SGReaderWriterOptions;
}

class SGPlanets : public SGReferenced {
public:
    SGPlanets() = default;

    // initialize the planets structure
    osg::Node* build(int num, const SGVec3d* planet_data, double planet_dist,
                     const simgear::SGReaderWriterOptions* options);
};
