/**
 * \file sky.hxx
 * Provides a class to model a realistic (time/date/position) based sky.
 */

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


#ifndef _SG_SKY_HXX
#define _SG_SKY_HXX


#include <simgear/compiler.h>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>

#include <vector>

#include <osg/ref_ptr>
#include <osg/MatrixTransform>
#include <osg/Node>

#include <simgear/ephemeris/ephemeris.hxx>
#include <simgear/math/SGMath.hxx>

#include <simgear/scene/sky/cloud.hxx>
#include <simgear/scene/sky/dome.hxx>
#include <simgear/scene/sky/moon.hxx>
#include <simgear/scene/sky/oursun.hxx>
#include <simgear/scene/sky/stars.hxx>

namespace simgear {
class SGReaderWriterOptions;
}

struct SGSkyState
{
  SGVec3d pos;     //!< View position in world Cartesian coordinates.
  SGGeod pos_geod;
  SGQuatd ori;
  double spin;     //!< An offset angle for orienting the sky effects with the
                   //   sun position so sunset and sunrise effects look correct.
  double gst;      //!< GMT side real time.
  double sun_dist; //!< the sun's distance from the current view point
                   //   (to keep it inside your view volume).
  double moon_dist;//!< The moon's distance from the current view point.
  double sun_angle;
};

struct SGSkyColor
{
  SGVec3f sky_color;
  SGVec3f adj_sky_color;
  SGVec3f fog_color;
  SGVec3f cloud_color;
  double sun_angle,
         moon_angle;
};

/**
 * \anchor SGSky-details
 *
 * A class to model a realistic (time/date/position) based sky.
 *
 * Introduction
 *
 * The SGSky class models a blended sky dome, a haloed sun, a textured
 * moon with phase that properly matches the date, stars and planets,
 * and cloud layers. SGSky is designed to be dropped into existing
 * plib based applications and depends heavily on plib's scene graph
 * library, ssg. The sky implements various time of day lighting
 * effects, it plays well with fog and visibility effects, and
 * implements scudded cloud fly-through effects. Additionally, you can
 * wire in the output of the SGEphemeris class to accurately position
 * all the objects in the sky.
 *
 * Building the sky 
 *

 * Once you have created an instance of SGSky you must call the
 * build() method.  Building the sky requires several textures. So,
 * you must specify the path/directory where these textures reside
 * before building the sky.  You do this first by calling the
 * texture_path() method.

 * The arguments you pass to the build() method allow you to specify
 * the horizontal and vertical radiuses of the sky dome, the size of
 * your sun sphere and moon sphere, a number of planets, and a
 * multitude of stars.  For the planets and stars you pass in an array
 * of right ascensions, declinations, and magnitudes.

 * Cloud Layers 

 * Cloud layers can be added, changed, or removed individually. To add
 * a cloud layer use the add_cloud_layer() method.  The arguments
 * allow you to specify base height above sea level, layer thickness,
 * a transition zone for entering/leaving the cloud layer, the size of
 * the cloud object, and the type of cloud texture. All distances are
 * in meters. There are additional forms of this method that allow you
 * to specify your own ssgSimpleState or texture name for drawing the
 * cloud layer.

 * Repainting the Sky 

 * As the sun circles the globe, you can call the repaint() method to
 * recolor the sky objects to simulate sunrise and sunset effects,
 * visibility, and other lighting changes.  The arguments allow you to
 * specify a base sky color (for the top of the dome), a fog color
 * (for the horizon), the sun angle with the horizon (for
 * sunrise/sunset effects), the moon angle (so we can make it more
 * yellow at the horizon), and new star and planet data so that we can
 * optionally change the magnitude of these (for day / night
 * transitions.)

 * Positioning Sky Objects 

 * As time progresses and as you move across the surface of the earth,
 * the apparent position of the objects and the various lighting
 * effects can change. the reposition() method allows you to specify
 * the positions of all the sky objects as well as your view position.
 * The arguments allow you to specify your view position in world
 * Cartesian coordinates, the zero elevation position in world
 * Cartesian coordinates (your longitude, your latitude, sea level),
 * the ``up'' vector in world Cartesian coordinates, current
 * longitude, latitude, and altitude. A ``spin'' angle can be
 * specified for orienting the sky with the sun position so sunset and
 * sunrise effects look correct. You must specify GMT side real time,
 * the sun right ascension, sun declination, and sun distance from
 * view point (to keep it inside your view volume.) You also must
 * specify moon right ascension, moon declination, and moon distance
 * from view point.

 * Rendering the Sky 

 * The sky is designed to be rendered in three stages. The first stage
 * renders the parts that form your back drop - the sky dome, the
 * stars and planets, the sun, and the moon.  These should be rendered
 * before the rest of your scene by calling the preDraw() method. The
 * second stage renders the clouds that are above the viewer. This stage 
 * is done before translucent objects in the main scene are drawn. It 
 * is seperated from the preDraw routine to enable to implement a 
 * multi passes technique and is located in the drawUpperClouds() method.
 * The third stage renders the clouds that are below the viewer an which 
 * are likely to be translucent (depending on type) and should be drawn 
 * after your scene has been rendered.  Use the drawLowerClouds() method 
 * to draw the second stage of the sky.

 * A typical application might do the following: 

 * \li thesky->preDraw( my_altitude );
 * \li thesky->drawUpperClouds();
 * \li ssgCullAndDraw ( myscene ) ;
 * \li thesky->drawLowerClouds();

 * The current altitude in meters is passed to the preDraw() method
 * so the clouds layers can be rendered correction from most distant
 * to closest.

 * Visibility Effects 

 * Visibility and fog is important for correctly rendering the
 * sky. You can inform SGSky of the current visibility by calling the
 * set_visibility() method.

 * When transitioning through clouds, it is nice to pull in the fog as
 * you get close to the cloud layer to hide the fact that the clouds
 * are drawn as a flat polygon. As you get nearer to the cloud layer
 * it is also nice to temporarily pull in the visibility to simulate
 * the effects of flying in and out of the puffy edge of the
 * cloud. These effects can all be accomplished by calling the
 * modify_vis() method.  The arguments allow you to specify your
 * current altitude (which is then compared to the altitudes of the
 * various cloud layers.) You can also specify a time factor which
 * should be the length in seconds since the last time you called
 * modify_vis(). The time_factor value allows the puffy cloud effect
 * to be calculated correctly.

 * The modify_vis() method alters the SGSky's internal idea of
 * visibility, so you should subsequently call get_visibility() to get
 * the actual modified visibility. You should then make the
 * appropriate glFog() calls to setup fog properly for your scene.

 * Accessor Methods 

 * Once an instance of SGSky has been successfully initialized, there
 * are a couple accessor methods you can use such as get_num_layers()
 * to return the number of cloud layers, get_cloud_layer(i) to return
 * cloud layer number i, get_visibility() to return the actual
 * visibility as modified by the sky/cloud model.

 */

class SGSky {

private:
    typedef std::vector<SGSharedPtr<SGCloudLayer> > layer_list_type;
    typedef layer_list_type::iterator layer_list_iterator;
    typedef layer_list_type::const_iterator layer_list_const_iterator;

    // components of the sky
    SGSharedPtr<SGSkyDome> dome;
    SGSharedPtr<SGSun> oursun;
    SGSharedPtr<SGMoon> moon;
    SGSharedPtr<SGStars> planets;
    SGSharedPtr<SGStars> stars;
    layer_list_type cloud_layers;

    osg::ref_ptr<osg::Group> pre_root, pre_transform;
    osg::ref_ptr<osg::Switch> cloud_root;

    osg::ref_ptr<osg::MatrixTransform> _ephTransform;

    SGPath tex_path;

    // visibility
    float visibility;
    float effective_visibility;

    int in_cloud;

    // near cloud visibility state variables
    bool in_puff;
    double puff_length;		// in seconds
    double puff_progression;	// in seconds
    double ramp_up;		// in seconds
    double ramp_down;		// in seconds

    // 3D clouds enabled
    bool clouds_3d_enabled;

    // 3D cloud density
    double clouds_3d_density;
    
    // RNG seed
    mt seed;

public:

    /** Constructor */
    SGSky( void );

    /** Destructor */
    ~SGSky( void );

    /**
     * Initialize the sky and connect the components to the scene
     * graph at the provided branch.
     *
     * @note See discussion in \ref SGSky-details "detailed class description".
     *
     * @param h_radius_m    Horizontal radius of sky dome
     * @param v_radius_m    Vertical radius of sky dome
     * @param sun_size      Size of sun
     * @param moon_size     Size of moon
     * @param eph           Current positions of planets and stars
     * @param node          Property node connecting sun with environment
     * @param options
     */
    void build( double h_radius_m,
                double v_radius_m,
                double sun_size,
                double moon_size,
                const SGEphemeris& eph,
                SGPropertyNode *node,
                simgear::SGReaderWriterOptions* options );

    /**
     * Repaint the sky components based on current sun angle, and sky and fog
     * colors.
     *
     * @note See discussion in \ref SGSky-details "detailed class description".
     *
     * @param sky_color The base sky color (for the top of the dome)
     * @param eph       Current positions of planets and stars
     */
    bool repaint( const SGSkyColor &sky_color,
                  const SGEphemeris& eph );

    /**
     * Reposition the sky at the specified origin and orientation.
     *
     * @note See discussion in \ref SGSky-details "detailed class description".
     *
     */
    bool reposition( const SGSkyState& sky_state,
                     const SGEphemeris& eph,
                     double dt = 0.0 );

    /**
     * Modify the given visibility based on cloud layers, thickness,
     * transition range, and simulated "puffs".
     *
     * @note See discussion in \ref SGSky-details "detailed class description".
     *
     * @param alt current altitude
     * @param time_factor amount of time since modify_vis() last called so
     *        we can scale effect rates properly despite variable frame rates.
     */
    void modify_vis( float alt, float time_factor );

    osg::Group* getPreRoot() { return pre_root.get(); }
    osg::Group* getCloudRoot() { return cloud_root.get(); }

    /** 
     * Specify the texture path (optional, defaults to current directory)
     *
     * @param path Base path to texture locations
     */
    void texture_path( const std::string& path );

    /**
     * Get the current sun color
     */
    inline SGVec4f get_sun_color() { return oursun->get_color(); }

    /**
     * Get the current scene color
     */
    inline SGVec4f get_scene_color() { return oursun->get_scene_color(); }

    /**
     * Add a cloud layer.
     *
     * Transfer pointer ownership to this object.
     *
     * @param layer The new cloud layer to add.
     */
    void add_cloud_layer (SGCloudLayer * layer);


    /**
     * Get a cloud layer (const).
     *
     * Pointer ownership remains with this object.
     *
     * @param i The index of the cloud layer, zero-based.
     * @return A const pointer to the cloud layer.
     */
    const SGCloudLayer * get_cloud_layer (int i) const;


    /**
     * Get a cloud layer (non-const).
     *
     * Pointer ownership remains with this object.
     *
     * @param i The index of the cloud layer, zero-based.
     * @return A non-const pointer to the cloud layer.
     */
    SGCloudLayer * get_cloud_layer (int i);


    /**
     * Return the number of cloud layers currently available.
     *
     * @return The cloud layer count.
     */
    int get_cloud_layer_count () const;


    /** @return current effective visibility */
    float get_visibility() const { return effective_visibility; }

    /** Set desired clear air visibility.
     * @param v visibility in meters
     */
    void set_visibility( float v );

    /** Get 3D cloud density */
    double get_3dCloudDensity() const;

    /** Set 3D cloud density 
     * @param density 3D cloud density
     */
    void set_3dCloudDensity(double density);

    /** Get 3D cloud visibility range*/
    float get_3dCloudVisRange() const;

    /** Set 3D cloud visibility range
     *
     * @param vis 3D cloud visibility range
     */
    void set_3dCloudVisRange(float vis);

    /** Get 3D cloud impostor distance*/
    float get_3dCloudImpostorDistance() const;

    /** Set 3D cloud impostor distance
     *
     * @param vis 3D cloud impostor distance
     */
    void set_3dCloudImpostorDistance(float vis);

    /** Get 3D cloud LoD1 Range*/
    float get_3dCloudLoD1Range() const;

    /** Set 3D cloud LoD1 Range
     * @param vis LoD1 Range
     */
    void set_3dCloudLoD1Range(float vis);

    /** Get 3D cloud LoD2 Range*/
    float get_3dCloudLoD2Range() const;

    /** Set 3D cloud LoD2 Range
     * @param vis LoD2 Range
     */
    void set_3dCloudLoD2Range(float vis);

    /** Get 3D cloud impostor usage */
    bool get_3dCloudUseImpostors() const;

    /** Set 3D cloud impostor usage
     *
     * @param imp whether use impostors for 3D clouds
     */
    void set_3dCloudUseImpostors(bool imp);

    /** Get 3D cloud wrapping */
    bool get_3dCloudWrap() const;

    /** Set 3D cloud wrapping
     * @param wrap whether to wrap 3D clouds
     */
    void set_3dCloudWrap(bool wrap);

    void set_clouds_enabled(bool enabled);

};
#endif // _SG_SKY_HXX
