// sky.cxx -- ssg based sky model
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "sky.hxx"
#include "cloudfield.hxx"
#include "newcloud.hxx"

#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/sg_inlines.h>

#include <osg/StateSet>
#include <osg/Depth>

// Constructor
SGSky::SGSky( void ) {
    effective_visibility = visibility = 10000.0;

    // near cloud visibility state variables
    in_puff = false;
    puff_length = 0;
    puff_progression = 0;
    ramp_up = 0.15;
    ramp_down = 0.15;

    in_cloud  = -1;
    
    clouds_3d_enabled = false;
    clouds_3d_density = 0.8;

    pre_root = new osg::Group;
    pre_root->setNodeMask(simgear::BACKGROUND_BIT);
    osg::StateSet* preStateSet = new osg::StateSet;
    preStateSet->setAttribute(new osg::Depth(osg::Depth::LESS, 0.0, 1.0,
                                             false));
    pre_root->setStateSet(preStateSet);
    cloud_root = new osg::Switch;
    cloud_root->setNodeMask(simgear::MODEL_BIT);
    cloud_root->setName("SGSky-cloud-root");

    pre_transform = new osg::Group;
    _ephTransform = new osg::MatrixTransform;
    
    // Set up a RNG that is repeatable within 10 minutes to ensure that clouds
    // are synced up in multi-process deployments.
    mt_init_time_10(&seed);
}


// Destructor
SGSky::~SGSky( void )
{
}


// initialize the sky and connect the components to the scene graph at
// the provided branch
void SGSky::build( double h_radius_m,
                   double v_radius_m,
                   double sun_size,
                   double moon_size,
                   const SGEphemeris& eph,
                   SGPropertyNode *property_tree_node,
                   simgear::SGReaderWriterOptions* options )
{
    dome = new SGSkyDome;
    pre_transform->addChild( dome->build( h_radius_m, v_radius_m, options ) );

    pre_transform->addChild(_ephTransform.get());
    planets = new SGStars;
    _ephTransform->addChild( planets->build(eph.getNumPlanets(), eph.getPlanets(), h_radius_m) );

    stars = new SGStars(property_tree_node);
    _ephTransform->addChild( stars->build(eph.getNumStars(), eph.getStars(), h_radius_m) );
    
    moon = new SGMoon;
    _ephTransform->addChild( moon->build(tex_path, moon_size) );

    oursun = new SGSun;
    _ephTransform->addChild( oursun->build(tex_path, sun_size, property_tree_node ) );

    pre_root->addChild( pre_transform.get() );
}


// repaint the sky components based on current value of sun_angle,
// sky, and fog colors.
//
// sun angle in degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool SGSky::repaint( const SGSkyColor &sc, const SGEphemeris& eph )
{
	dome->repaint( sc.adj_sky_color, sc.sky_color, sc.fog_color,
                       sc.sun_angle, effective_visibility );

        stars->repaint( sc.sun_angle, eph.getNumStars(), eph.getStars() );
        planets->repaint( sc.sun_angle, eph.getNumPlanets(), eph.getPlanets() );
	oursun->repaint( sc.sun_angle, effective_visibility );
	moon->repaint( sc.moon_angle );

	for ( unsigned i = 0; i < cloud_layers.size(); ++i ) {
            if (cloud_layers[i]->getCoverage() != SGCloudLayer::SG_CLOUD_CLEAR){
                cloud_layers[i]->repaint( sc.cloud_color );
            }
	}

    SGCloudField::updateFog((double)effective_visibility,
                            osg::Vec4f(toOsg(sc.fog_color), 1.0f));
    return true;
}

// reposition the sky at the specified origin and orientation
//
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (this allows
// additional orientation for the sunrise/set effects and is used by
// the skydome and perhaps clouds.
bool SGSky::reposition( const SGSkyState &st, const SGEphemeris& eph, double dt )
{
    double angle = st.gst * 15;	// degrees
    double angleRad = SGMiscd::deg2rad(angle);

    SGVec3f zero_elev, view_up;
    double lon, lat, alt;

    SGGeod geodZeroViewPos = SGGeod::fromGeodM(st.pos_geod, 0);
    zero_elev = toVec3f( SGVec3d::fromGeod(geodZeroViewPos) );

    // calculate the scenery up vector
    SGQuatd hlOr = SGQuatd::fromLonLat(st.pos_geod);
    view_up = toVec3f(hlOr.backTransform(-SGVec3d::e3()));

    // viewer location
    lon = st.pos_geod.getLongitudeRad();
    lat = st.pos_geod.getLatitudeRad();
    alt = st.pos_geod.getElevationM();

    dome->reposition( zero_elev, alt, lon, lat, st.spin );

    osg::Matrix m = osg::Matrix::rotate(angleRad, osg::Vec3(0, 0, -1));
    m.postMultTranslate(toOsg(st.pos));
    _ephTransform->setMatrix(m);

    double sun_ra = eph.getSunRightAscension();
    double sun_dec = eph.getSunDeclination();
    oursun->reposition( sun_ra, sun_dec, st.sun_dist, lat, alt, st.sun_angle );

    double moon_ra = eph.getMoonRightAscension();
    double moon_dec = eph.getMoonDeclination();
    moon->reposition( moon_ra, moon_dec, st.moon_dist );

    for ( unsigned i = 0; i < cloud_layers.size(); ++i ) {
        if ( cloud_layers[i]->getCoverage() != SGCloudLayer::SG_CLOUD_CLEAR ||
               cloud_layers[i]->get_layer3D()->isDefined3D() ) {
            cloud_layers[i]->reposition( zero_elev, view_up, lon, lat, alt, dt);
        } else {
          cloud_layers[i]->getNode()->setAllChildrenOff();
    }
    }

    return true;
}

void
SGSky::set_visibility( float v )
{
    visibility = std::max(v, 25.0f);
}

void
SGSky::add_cloud_layer( SGCloudLayer * layer )
{
    cloud_layers.push_back(layer);
    cloud_root->addChild(layer->getNode(), true);

    layer->set_enable3dClouds(clouds_3d_enabled);
}

const SGCloudLayer *
SGSky::get_cloud_layer (int i) const
{
    return cloud_layers[i];
}

SGCloudLayer *
SGSky::get_cloud_layer (int i)
{
    return cloud_layers[i];
}

int
SGSky::get_cloud_layer_count () const
{
    return cloud_layers.size();
}

double SGSky::get_3dCloudDensity() const {
    return SGNewCloud::getDensity();
}

void SGSky::set_3dCloudDensity(double density)
{
    SGNewCloud::setDensity(density);
}

float SGSky::get_3dCloudVisRange() const {
    return SGCloudField::getVisRange();
}

void SGSky::set_3dCloudVisRange(float vis)
{
    SGCloudField::setVisRange(vis);
    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
        cloud_layers[i]->get_layer3D()->applyVisAndLoDRange();
    }
}

float SGSky::get_3dCloudImpostorDistance() const {
    return SGCloudField::getImpostorDistance();
}

void SGSky::set_3dCloudImpostorDistance(float vis)
{
    SGCloudField::setImpostorDistance(vis);
    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
        cloud_layers[i]->get_layer3D()->applyVisAndLoDRange();
    }
}

float SGSky::get_3dCloudLoD1Range() const {
    return SGCloudField::getLoD1Range();
}

void SGSky::set_3dCloudLoD1Range(float vis)
{
    SGCloudField::setLoD1Range(vis);
    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
        cloud_layers[i]->get_layer3D()->applyVisAndLoDRange();
    }
}

float SGSky::get_3dCloudLoD2Range() const {
    return SGCloudField::getLoD2Range();
}

void SGSky::set_3dCloudLoD2Range(float vis)
{
    SGCloudField::setLoD2Range(vis);
    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
        cloud_layers[i]->get_layer3D()->applyVisAndLoDRange();
    }
}

bool SGSky::get_3dCloudWrap() const {
    return SGCloudField::getWrap();
}

void SGSky::set_3dCloudWrap(bool wrap)
{
    SGCloudField::setWrap(wrap);
}

bool SGSky::get_3dCloudUseImpostors() const {
    return SGCloudField::getUseImpostors();
}

void SGSky::set_3dCloudUseImpostors(bool imp)
{
    SGCloudField::setUseImpostors(imp);
}

void SGSky::set_texture_path( const SGPath& path ) 
{
	tex_path = path;
}

// modify the current visibility based on cloud layers, thickness,
// transition range, and simulated "puffs".
void SGSky::modify_vis( float alt, float time_factor ) {
    float effvis = visibility;

    for ( int i = 0; i < (int)cloud_layers.size(); ++i ) {
	float asl = cloud_layers[i]->getElevation_m();
	float thickness = cloud_layers[i]->getThickness_m();
	float transition = cloud_layers[i]->getTransition_m();

	double ratio = 1.0;

        if ( cloud_layers[i]->getCoverage() == SGCloudLayer::SG_CLOUD_CLEAR ) {
	    // less than 50% coverage -- assume we're in the clear for now
	    ratio = 1.0;
        } else if ( alt < asl - transition ) {
	    // below cloud layer
	    ratio = 1.0;
	} else if ( alt < asl ) {
	    // in lower transition
	    ratio = (asl - alt) / transition;
	} else if ( alt < asl + thickness ) {
	    // in cloud layer
	    ratio = 0.0;
	} else if ( alt < asl + thickness + transition ) {
	    // in upper transition
	    ratio = (alt - (asl + thickness)) / transition;
	} else {
	    // above cloud layer
	    ratio = 1.0;
	}

        if ( cloud_layers[i]->getCoverage() == SGCloudLayer::SG_CLOUD_CLEAR ||
             cloud_layers[i]->get_layer3D()->isDefined3D()) {
            // do nothing, clear layers aren't drawn, don't affect
            // visibility andn dont' need to be faded in or out.
        } else if ( (cloud_layers[i]->getCoverage() == 
                     SGCloudLayer::SG_CLOUD_FEW)
                    || (cloud_layers[i]->getCoverage() ==
                        SGCloudLayer::SG_CLOUD_SCATTERED) )
        {
            // set the alpha fade value for the cloud layer.  For less
            // dense cloud layers we fade the layer to nothing as we
            // approach it because we stay clear visibility-wise as we
            // pass through it.
            float temp = ratio * 2.0;
            if ( temp > 1.0 ) { temp = 1.0; }
            cloud_layers[i]->setAlpha( temp );

            // don't touch visibility
        } else {
            // maintain full alpha for denser cloud layer types.
            // Let's set the value explicitly in case someone changed
            // the layer type.
            cloud_layers[i]->setAlpha( 1.0 );

            // lower visibility as we approach the cloud layer.
            // accumulate effects from multiple cloud layers
            effvis *= ratio;
        }

#if 0
	if ( ratio < 1.0 ) {
	    if ( ! in_puff ) {
		// calc chance of entering cloud puff
		double rnd = mt_rand(&seed);
		double chance = rnd * rnd * rnd;
		if ( chance > 0.95 /* * (diff - 25) / 50.0 */ ) {
		    in_puff = true;
		    puff_length = mt_rand(&seed) * 2.0; // up to 2 seconds
		    puff_progression = 0.0;
		}
	    }

	    if ( in_puff ) {
		// modify actual_visibility based on puff envelope

		if ( puff_progression <= ramp_up ) {
		    double x = SGD_PI_2 * puff_progression / ramp_up;
		    double factor = 1.0 - sin( x );
		    // cout << "ramp up = " << puff_progression
		    //      << "  factor = " << factor << endl;
		    effvis = effvis * factor;
		} else if ( puff_progression >= ramp_up + puff_length ) {
		    double x = SGD_PI_2 * 
			(puff_progression - (ramp_up + puff_length)) /
			ramp_down;
		    double factor = sin( x );
		    // cout << "ramp down = " 
		    //      << puff_progression - (ramp_up + puff_length) 
		    //      << "  factor = " << factor << endl;
		    effvis = effvis * factor;
		} else {
		    effvis = 0.0;
		}

		/* cout << "len = " << puff_length
		   << "  x = " << x 
		   << "  factor = " << factor
		   << "  actual_visibility = " << actual_visibility 
		   << endl; */

		// time_factor = ( global_multi_loop * 
		//                 current_options.get_speed_up() ) /
		//                (double)current_options.get_model_hz();

		puff_progression += time_factor;
		// cout << "time factor = " << time_factor << endl;

		/* cout << "gml = " << global_multi_loop 
		   << "  speed up = " << current_options.get_speed_up()
		   << "  hz = " << current_options.get_model_hz() << endl;
		   */ 

		if ( puff_progression > puff_length + ramp_up + ramp_down) {
		    in_puff = false; 
		}
	    }
	}
#endif

        // never let visibility drop below the layer's configured visibility
       effvis = SG_MAX2<float>(cloud_layers[i]->getVisibility_m(), effvis );

    } // for

    effective_visibility = effvis;
}

void SGSky::set_clouds_enabled(bool enabled)
{
    if (enabled) {
        cloud_root->setAllChildrenOn();
    } else {
        cloud_root->setAllChildrenOff();
    }
}
