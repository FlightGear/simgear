/*
 * SPDX-FileName: dome.hxx
 * SPDX-FileComment: model sky with an upside down "bowl"
 * SPDX-FileCopyrightText: Copyright (C) 1997-2000  Curtis L. Olson  - http://www.flightgear.org/~curt
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#pragma once

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/MatrixTransform>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/math/SGMath.hxx>

namespace osg {
class DrawElementsUShort;
}

namespace simgear {
class SGReaderWriterOptions;
}

class SGSkyDome : public SGReferenced {
public:
    SGSkyDome() = default;

    /*
     * Initialize the sky object and connect it into our scene graph root.
     */
    osg::Node* build(double hscale = 80000.0, double vscale = 80000.0,
                     const simgear::SGReaderWriterOptions* options = nullptr);

    /*
     * Reposition the sky at the specified origin and orientation
     * lon specifies a rotation about the Z axis
     * lat specifies a rotation about the new Y axis
     * spin specifies a rotation about the new Z axis (and orients the
     * sunrise/set effects).
     */
    bool reposition(const SGVec3f& p, double asl,
                    double lon, double lat, double spin);
private:
    void makeDome(int rings, int bands, osg::DrawElementsUShort& elements);

    double asl{0.0};
    osg::ref_ptr<osg::MatrixTransform> dome_transform;
    osg::ref_ptr<osg::Vec3Array> dome_vl;
};
