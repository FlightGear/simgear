// skydome.hxx -- model sky with an upside down "bowl"
//
// Written by Curtis Olson, started December 1997.
// SSG-ified by Curtis Olson, February 2000.
//
// Copyright (C) 1997-2000  Curtis L. Olson  - curt@flightgear.org
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


#ifndef _SKYDOME_HXX
#define _SKYDOME_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <plib/ssg.h>		// plib include


class FGSkyDome {
    // scene graph root for the skydome
    ssgRoot *dome;

    ssgSelector *dome_selector;
    ssgTransform *dome_transform;
    ssgSimpleState *dome_state;

    ssgVertexArray *center_disk_vl;
    ssgColourArray *center_disk_cl;

    ssgVertexArray *upper_ring_vl;
    ssgColourArray *upper_ring_cl;

    ssgVertexArray *middle_ring_vl;
    ssgColourArray *middle_ring_cl;

    ssgVertexArray *lower_ring_vl;
    ssgColourArray *lower_ring_cl;

public:

    // Constructor
    FGSkyDome( void );

    // Destructor
    ~FGSkyDome( void );

    // initialize the sky object and connect it into our scene graph
    // root
    bool initialize();

    // repaint the sky colors based on current value of sun_angle,
    // sky, and fog colors.  This updates the color arrays for
    // ssgVtxTable.
    // sun angle in degrees relative to verticle
    // 0 degrees = high noon
    // 90 degrees = sun rise/set
    // 180 degrees = darkest midnight
    bool repaint( sgVec3 sky_color, sgVec3 fog_color, double sun_angle );

    // reposition the sky at the specified origin and orientation
    // lon specifies a rotation about the Z axis
    // lat specifies a rotation about the new Y axis
    // spin specifies a rotation about the new Z axis (and orients the
    // sunrise/set effects
    bool reposition( sgVec3 p, double lon, double lat, double spin );

    // Draw the skydome
    bool draw();

    // enable the sky in the scene graph (default)
    void enable() { dome_selector->select( 1 ); }

    // disable the sky in the scene graph.  The leaf node is still
    // there, how ever it won't be traversed on the cullandrender
    // phase.
    void disable() { dome_selector->select( 0 ); }

};


extern FGSkyDome current_sky;


// (Re)generate the display list
// void fgSkyInit();

// (Re)calculate the sky colors at each vertex
// void fgSkyColorsInit();

// Draw the Sky
// void fgSkyRender();


#endif // _SKYDOM_HXX


