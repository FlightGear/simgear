// sky.hxx -- model sky with an upside down "bowl"
//
// Written by Curtis Olson, started December 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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


#ifndef _SKY_HXX
#define _SKY_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <plib/ssg.h>		// plib include


class FGSky {
    double sun_angle;		// sun angle in degrees relative to verticle
				// 0 degrees = high noon
				// 90 degrees = sun rise/set
				// 180 degrees = darkest midnight

    sgVec3 sky_color;		// base sky color
    sgVec3 fog_color;		// fog color

    sgVec3 origin;		// coordinates of sky placement origin
				// I recommend (lon, lat, 0) relative to
				// your world coordinate scheme

    double lon, lat;		// current lon and lat (for properly rotating
				// sky)


    ssgSelector *sky_selector;
    ssgTransform *sky_transform;

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
    FGSky( void );

    // Destructor
    ~FGSky( void );

    // initialize the sky object and connect it into the scene graph
    bool initialize();

    // repaint the sky colors based on current value of sun_angle,
    // sky, and fog colors.  This updates the color arrays for
    // ssgVtxTable.
    bool repaint();

    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    bool build( ssgBranch *branch );

    // enable the sky in the scene graph (default)
    bool enable() { sky_selector->select( 1 ); }

    // disable the sky in the scene graph.  The leaf node is still
    // there, how ever it won't be traversed on the cullandrender
    // phase.
    bool disable() { sky_selector->select( 0 ); }

    inline void set_sun_angle( double a ) { sun_angle = a; }
    inline void set_sky_color( sgVec3 color ) { 
	sgCopyVec3(sky_color, color);
    }
    inline void set_fog_color( sgVec3 color ) { 
	sgCopyVec3(fog_color, color);
    }
    inline void set_origin( sgVec3 p ) { 
	sgCopyVec3(origin, p);
    }
    inline void set_lon( double l ) { lon = l; }
    inline void set_lat( double l ) { lat = l; }
};


// (Re)generate the display list
void fgSkyInit();

// (Re)calculate the sky colors at each vertex
void fgSkyColorsInit();

// Draw the Sky
void fgSkyRender();


#endif // _SKY_HXX


