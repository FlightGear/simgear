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

#include <simgear/compiler.h>

#include <plib/ssg.h>

#include STL_STRING
SG_USING_STD(string);


class SGCloudLayer {

public:

    enum Coverage {
	SG_CLOUD_OVERCAST = 0,
	SG_CLOUD_BROKEN,
	SG_CLOUD_SCATTERED,
	SG_CLOUD_FEW,
	SG_CLOUD_CIRRUS,
	SG_CLOUD_CLEAR,
	SG_MAX_CLOUD_COVERAGES
    };

    // Constructors
    SGCloudLayer( const string &tex_path );

    // Destructor
    ~SGCloudLayer( void );

    float getSpan_m () const;
    void setSpan_m (float span_m);

    float getElevation_m () const;
    void setElevation_m (float elevation_m);

    float getThickness_m () const;
    void setThickness_m (float thickness_m);

    float getTransition_m () const;
    void setTransition_m (float transition_m);

    Coverage getCoverage () const;
    void setCoverage (Coverage coverage);

    // build the cloud object
    void rebuild();

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

private:

    static ssgSimpleState *layer_states[SG_MAX_CLOUD_COVERAGES];
    static int layer_sizes[SG_MAX_CLOUD_COVERAGES];

    ssgRoot *layer_root;
    ssgTransform *layer_transform;
    ssgLeaf * layer[4];

    ssgColourArray *cl[4]; 
    ssgVertexArray *vl[4];
    ssgTexCoordArray *tl[4];

    // height above sea level (meters)
    SGPath texture_path;
    float layer_span;
    float layer_asl;
    float layer_thickness;
    float layer_transition;
    Coverage layer_coverage;
    float scale;

    // for handling texture coordinates to simulate cloud movement
    // from winds, and to simulate the clouds being tied to ground
    // position, not view position
    // double xoff, yoff;
    double last_lon, last_lat;

};


// make an ssgSimpleState for a cloud layer given the named texture
ssgSimpleState *SGCloudMakeState( const string &path );


#endif // _SG_CLOUD_HXX_
