/**
 * \file cloud.hxx
 * Provides a class to model a single cloud layer
 */

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


/**
 * A class layer to model a single cloud layer
 */
class SGCloudLayer {
public:

    /**
     * This is the list of available cloud coverages/textures
     */
    enum Coverage {
	SG_CLOUD_OVERCAST = 0,
	SG_CLOUD_BROKEN,
	SG_CLOUD_SCATTERED,
	SG_CLOUD_FEW,
	SG_CLOUD_CIRRUS,
	SG_CLOUD_CLEAR,
	SG_MAX_CLOUD_COVERAGES
    };

    /**
     * Constructor
     * @param tex_path the path to the set of cloud textures
     */
    SGCloudLayer( const string &tex_path );

    /**
     * Destructor
     */
    ~SGCloudLayer( void );

    /** get the cloud span (in meters) */
    float getSpan_m () const;
    /**
     * set the cloud span
     * @param span_m the cloud span in meters
     */
    void setSpan_m (float span_m);

    /** get the layer elevation (in meters) */
    float getElevation_m () const;
    /**
     * set the layer elevation.  Note that this specifies the bottom
     * of the cloud layer.  The elevation of the top of the layer is
     * elevation_m + thickness_m.
     * @param elevation_m the layer elevation in meters
     */
    void setElevation_m (float elevation_m);

    /** get the layer thickness */
    float getThickness_m () const;
    /**
     * set the layer thickness.
     * @param thickness_m the layer thickness in meters.
     */
    void setThickness_m (float thickness_m);

    /**
     * get the transition/boundary layer depth in meters.  This
     * allows gradual entry/exit from the cloud layer via adjusting
     * visibility.
     */
    float getTransition_m () const;

    /**
     * set the transition layer size in meters
     * @param transition_m the transition layer size in meters
     */
    void setTransition_m (float transition_m);

    /** get coverage type */
    Coverage getCoverage () const;

    /**
     * set coverage type
     * @param coverage the coverage type
     */
    void setCoverage (Coverage coverage);

    /**
     * set the cloud movement direction
     * @param dir the cloud movement direction
     */
    inline void setDirection(float dir) { direction = dir; }

    /** get the cloud movement direction */
    inline float getDirection() { return direction; }

    /**
     * set the cloud movement speed 
     * @param sp the cloud movement speed
     */
    inline void setSpeed(float sp) { speed = sp; }

    /** get the cloud movement speed */
    inline float getSpeed() { return speed; }

    /** build the cloud object */
    void rebuild();

    /**
     * repaint the cloud colors based on the specified fog_color
     * @param fog_color the fog color
     */
    bool repaint( sgVec3 fog_color );

    /**
     * reposition the cloud layer at the specified origin and
     * orientation.
     * @param p position vector
     * @param up the local up vector
     * @param lon specifies a rotation about the Z axis
     * @param lat specifies a rotation about the new Y axis
     * @param spin specifies a rotation about the new Z axis
     *        (and orients the sunrise/set effects)
     * @param dt the time elapsed since the last call
     */
    bool reposition( sgVec3 p, sgVec3 up, double lon, double lat, double alt,
                     double dt = 0.0 );

    /** draw the cloud layer */
    void draw();

private:

    ssgRoot *layer_root;
    ssgTransform *layer_transform;
    ssgLeaf *layer[4];

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
    float speed;
    float direction;

    // for handling texture coordinates to simulate cloud movement
    // from winds, and to simulate the clouds being tied to ground
    // position, not view position
    // double xoff, yoff;
    double last_lon, last_lat;

};


// make an ssgSimpleState for a cloud layer given the named texture
ssgSimpleState *sgCloudMakeState( const string &path );


#endif // _SG_CLOUD_HXX_
