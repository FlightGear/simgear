// cloud.hxx -- model a single cloud layer
//
// Written by Curtis Olson, started June 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _SG_CLOUD_HXX_
#define _SG_CLOUD_HXX_


#include <plib/ssg.h>

#include <simgear/misc/fgpath.hxx>


class SGCloudLayer {

    ssgTransform *layer_transform;
    ssgSimpleState *layer_state;

    ssgColourArray *cl; 
    ssgVertexArray *vl;
    ssgTexCoordArray *tl;

    float layer_asl;		// height above sea level (meters)

public:

    // Constructor
    SGCloudLayer( void );

    // Destructor
    ~SGCloudLayer( void );

    // build the cloud object
    ssgBranch *build( FGPath path, double size, double asl );

    // repaint the cloud colors based on current value of sun_angle,
    // sky, and fog colors.  This updates the color arrays for
    // ssgVtxTable.
    // sun angle in degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( sgVec3 fog_color );

    // reposition the cloud layer at the specified origin and
    // orientation
    // lon specifies a rotation about the Z axis
    // lat specifies a rotation about the new Y axis
    // spin specifies a rotation about the new Z axis (and orients the
    // sunrise/set effects
    bool reposition( sgVec3 p, sgVec3 up, double lon, double lat );
};


#endif // _SG_CLOUD_HXX_
