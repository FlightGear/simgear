// cloud.hxx -- model a single cloud layer
//
// Written by Curtis Olson, started June 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _SG_CLOUD_HXX_
#define _SG_CLOUD_HXX_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>

#include <plib/ssg.h>

#include STL_STRING
FG_USING_STD(string);


#define SG_MAX_CLOUD_TYPES 4	// change this if we add/remove cloud
				// types en the enum below

enum SGCloudType {
    SG_CLOUD_OVERCAST = 0,
    SG_CLOUD_MOSTLY_CLOUDY = 1,
    SG_CLOUD_MOSTLY_SUNNY = 2,
    SG_CLOUD_CIRRUS = 3,
};


class SGCloudLayer {

private:

    ssgRoot *layer_root;
    ssgTransform *layer_transform;
    ssgSimpleState *layer_state;

    ssgColourArray *cl; 
    ssgVertexArray *vl;
    ssgTexCoordArray *tl;

    // height above sea level (meters)
    float layer_asl;
    float layer_thickness;
    float layer_transition;
    float size;
    float scale;

    // for handling texture coordinates to simulate cloud movement
    // from winds, and to simulate the clouds being tied to ground
    // position, not view position
    // double xoff, yoff;
    double last_lon, last_lat;

public:

    // Constructor
    SGCloudLayer( void );

    // Destructor
    ~SGCloudLayer( void );

    // build the cloud object
    void build( double size, double asl, double thickness,
		double transition, ssgSimpleState *state );

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
    bool reposition( sgVec3 p, sgVec3 up, double lon, double lat, double alt );

    // draw the cloud layer
    void draw();

    inline float get_asl() const { return layer_asl; }
    inline float get_thickness() const { return layer_thickness; }
    inline float get_transition() const { return layer_transition; }
};


// make an ssgSimpleState for a cloud layer given the named texture
ssgSimpleState *SGCloudMakeState( const string &path );


#endif // _SG_CLOUD_HXX_
