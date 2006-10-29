// dome.hxx -- model sky with an upside down "bowl"
//
// Written by Curtis Olson, started December 1997.
// SSG-ified by Curtis Olson, February 2000.
//
// Copyright (C) 1997-2000  Curtis L. Olson  - http://www.flightgear.org/~curt
//
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


#ifndef _SKYDOME_HXX
#define _SKYDOME_HXX


#ifndef __cplusplus
# error This library requires C++
#endif

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/MatrixTransform>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/math/SGMath.hxx>

class SGSkyDome : public SGReferenced {
    osg::ref_ptr<osg::MatrixTransform> dome_transform;

    osg::ref_ptr<osg::Vec3Array> center_disk_vl;
    osg::ref_ptr<osg::Vec3Array> center_disk_cl;

    osg::ref_ptr<osg::Vec3Array> upper_ring_vl;
    osg::ref_ptr<osg::Vec3Array> upper_ring_cl;

    osg::ref_ptr<osg::Vec3Array> middle_ring_vl;
    osg::ref_ptr<osg::Vec3Array> middle_ring_cl;

    osg::ref_ptr<osg::Vec3Array> lower_ring_vl;
    osg::ref_ptr<osg::Vec3Array> lower_ring_cl;

    double asl;

public:

    // Constructor
    SGSkyDome( void );

    // Destructor
    ~SGSkyDome( void );

    // initialize the sky object and connect it into our scene graph
    // root
    osg::Node *build( double hscale = 80000.0, double vscale = 80000.0 );

    // repaint the sky colors based on current value of sun_angle,
    // sky, and fog colors.  This updates the color arrays for
    // ssgVtxTable.
    // sun angle in degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( const SGVec3f& sky_color, const SGVec3f& fog_color,
                  double sun_angle, double vis );

    // reposition the sky at the specified origin and orientation
    // lon specifies a rotation about the Z axis
    // lat specifies a rotation about the new Y axis
    // spin specifies a rotation about the new Z axis (and orients the
    // sunrise/set effects
    bool reposition( const SGVec3f& p, double asl,
                     double lon, double lat, double spin );
};


#endif // _SKYDOME_HXX
