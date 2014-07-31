/**
 * \file cloud.hxx
 * Provides a class to model a single cloud layer
 */

// Written by Curtis Olson, started June 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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


#ifndef _SG_CLOUD_HXX_
#define _SG_CLOUD_HXX_

#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGReferenced.hxx>

#include <string>

#include <osg/ref_ptr>
#include <osg/Array>
#include <osg/Geode>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/Switch>

class SGCloudField;

/**
 * A class layer to model a single cloud layer
 */
class SGCloudLayer : public SGReferenced {
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

    static const std::string SG_CLOUD_OVERCAST_STRING; // "overcast"
    static const std::string SG_CLOUD_BROKEN_STRING; // "broken"
    static const std::string SG_CLOUD_SCATTERED_STRING; // "scattered"
    static const std::string SG_CLOUD_FEW_STRING; // "few"
    static const std::string SG_CLOUD_CIRRUS_STRING; // "cirrus"
    static const std::string SG_CLOUD_CLEAR_STRING; // "clear"

    /**
     * Constructor
     * @param tex_path the path to the set of cloud textures
     */
    SGCloudLayer( const std::string &tex_path );

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
     * @param set_span defines whether it is allowed to adjust the span
     */
    void setElevation_m (float elevation_m, bool set_span = true);

    /** get the layer thickness */
    float getThickness_m () const;
    /**
     * set the layer thickness.
     * @param thickness_m the layer thickness in meters.
     */
    void setThickness_m (float thickness_m);

    /** get the layer visibility */
    float getVisibility_m() const;
    /**
     * set the layer visibility 
     * @param visibility_m the layer minimum visibility in meters.
     */
    void setVisibility_m(float visibility_m);



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

    /** get coverage as string */
    const std::string & getCoverageString() const;

    /** get coverage as string */
    static const std::string & getCoverageString( Coverage coverage );

    /** get coverage type from string */
    static Coverage getCoverageType( const std::string & coverage );

    /** set coverage as string */
    void setCoverageString( const std::string & coverage );

    /**
     * set the cloud movement direction
     * @param dir the cloud movement direction
     */
    inline void setDirection(float dir) { 
        // cout << "cloud dir = " << dir << endl;
        direction = dir;
    }

    /** get the cloud movement direction */
    inline float getDirection() { return direction; }

    /**
     * set the cloud movement speed 
     * @param sp the cloud movement speed
     */
    inline void setSpeed(float sp) {
        // cout << "cloud speed = " << sp << endl;
        speed = sp;
    }

    /** get the cloud movement speed */
    inline float getSpeed() { return speed; }

    /**
     * set the alpha component of the cloud base color.  Normally this
     * should be 1.0, but you can set it anywhere in the range of 0.0
     * to 1.0 to fade a cloud layer in or out.
     * @param alpha cloud alpha value (0.0 to 1.0)
     */
    inline void setAlpha( float alpha ) {
        if ( alpha < 0.0 ) { alpha = 0.0; }
        if ( alpha > max_alpha ) { alpha = max_alpha; }
        cloud_alpha = alpha;
    }

    inline void setMaxAlpha( float alpha ) {
        if ( alpha < 0.0 ) { alpha = 0.0; }
        if ( alpha > 1.0 ) { alpha = 1.0; }
        max_alpha = alpha;
    }

    inline float getMaxAlpha() const {
        return max_alpha;
    }

    /** build the cloud object */
    void rebuild();

    /** Enable/disable 3D clouds in this layer */
    void set_enable3dClouds(bool enable);

    /**
     * repaint the cloud colors based on the specified fog_color
     * @param fog_color the fog color
     */
    bool repaint( const SGVec3f& fog_color );

    /**
     * reposition the cloud layer at the specified origin and
     * orientation.
     * @param p position vector
     * @param up the local up vector
     * @param lon TODO
     * @param lat TODO
     * @param alt TODO
     * @param dt the time elapsed since the last call
     */
    bool reposition( const SGVec3f& p,
                     const SGVec3f& up,
                     double lon, double lat, double alt,
                     double dt = 0.0 );

    osg::Switch* getNode() { return cloud_root.get(); }

    /** return the 3D layer cloud associated with this 2D layer */
    SGCloudField *get_layer3D(void) { return layer3D; }

protected:
    void setTextureOffset(const osg::Vec2& offset);
private:

    osg::ref_ptr<osg::Switch> cloud_root;
    osg::ref_ptr<osg::Switch> layer_root;
    osg::ref_ptr<osg::Group> group_top, group_bottom;
    osg::ref_ptr<osg::MatrixTransform> layer_transform;
    osg::ref_ptr<osg::Geode> layer[4];

    float cloud_alpha;          // 1.0 = drawn fully, 0.0 faded out completely

    osg::ref_ptr<osg::Vec4Array> cl[4];
    osg::ref_ptr<osg::Vec3Array> vl[4];
    osg::ref_ptr<osg::Vec2Array> tl[4];
    osg::ref_ptr<osg::Vec3Array> tl2[4];

    // height above sea level (meters)
    SGPath texture_path;
    float layer_span;
    float layer_asl;
    float layer_thickness;
    float layer_transition;
    float layer_visibility;
    Coverage layer_coverage;
    float scale;
    float speed;
    float direction;

    // for handling texture coordinates to simulate cloud movement
    // from winds, and to simulate the clouds being tied to ground
    // position, not view position
    // double xoff, yoff;
    SGGeod last_pos;
    double last_course;
    double max_alpha;

    osg::Vec2 base;

    SGCloudField *layer3D;

};

#endif // _SG_CLOUD_HXX_
