// dome.cxx -- model sky with an upside down "bowl"
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

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <math.h>

#include <simgear/compiler.h>

#include <osg/Array>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Node>
#include <osg/MatrixTransform>
#include <osg/Material>
#include <osg/ShadeModel>

#include <simgear/debug/logstream.hxx>

#include "dome.hxx"


#ifdef __MWERKS__
#  pragma global_optimizer off
#endif


// proportions of max dimensions fed to the build() routine
static const float center_elev = 1.0;

static const float upper_radius = 0.6;
static const float upper_elev = 0.15;

static const float middle_radius = 0.9;
static const float middle_elev = 0.08;

static const float lower_radius = 1.0;
static const float lower_elev = 0.0;

static const float bottom_radius = 0.8;
static const float bottom_elev = -0.1;


// Constructor
SGSkyDome::SGSkyDome( void ) {
    asl = 0;
}


// Destructor
SGSkyDome::~SGSkyDome( void ) {
}


// initialize the sky object and connect it into our scene graph
osg::Node*
SGSkyDome::build( double hscale, double vscale ) {

    osg::Geode* geode = new osg::Geode;

    // set up the state
    osg::StateSet* stateSet = geode->getOrCreateStateSet();
    stateSet->setRenderBinDetails(-10, "RenderBin");

    osg::ShadeModel* shadeModel = new osg::ShadeModel;
    shadeModel->setMode(osg::ShadeModel::SMOOTH);
    stateSet->setAttributeAndModes(shadeModel);
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
    stateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
    osg::Material* material = new osg::Material;
//     material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
//     material->setEmission(osg::Material::FRONT_AND_BACK,
//                           osg::Vec4(0, 0, 0, 1));
//     material->setSpecular(osg::Material::FRONT_AND_BACK,
//                           osg::Vec4(0, 0, 0, 1));
//     material->setShininess(osg::Material::FRONT_AND_BACK, 0);
    stateSet->setAttribute(material);

    // initialize arrays
    // initially seed to all blue
    center_disk_vl = new osg::Vec3Array;
    center_disk_cl = new osg::Vec3Array;
    center_disk_cl->assign(14, osg::Vec3(0, 0, 1));
				       
    upper_ring_vl = new osg::Vec3Array;
    upper_ring_cl = new osg::Vec3Array;
    upper_ring_cl->assign(26, osg::Vec3(0, 0, 1));

    middle_ring_vl = new osg::Vec3Array;
    middle_ring_cl = new osg::Vec3Array;
    middle_ring_cl->assign(26, osg::Vec3(0, 0, 1));

    lower_ring_vl = new osg::Vec3Array;
    lower_ring_cl = new osg::Vec3Array;
    lower_ring_cl->assign(26, osg::Vec3(0, 0, 1));


    // generate the raw vertex data
    osg::Vec3 center_vertex(0.0, 0.0, center_elev*vscale);
    osg::Vec3 upper_vertex[12];
    osg::Vec3 middle_vertex[12];
    osg::Vec3 lower_vertex[12];
    osg::Vec3 bottom_vertex[12];

    for ( int i = 0; i < 12; ++i ) {
        double theta = (i * 30) * SGD_DEGREES_TO_RADIANS;
        double sTheta = hscale*sin(theta);
        double cTheta = hscale*cos(theta);

	upper_vertex[i] = osg::Vec3(cTheta * upper_radius,
                                    sTheta * upper_radius,
                                    upper_elev * vscale);

	middle_vertex[i] = osg::Vec3(cTheta * middle_radius,
                                     sTheta * middle_radius,
                                     middle_elev * vscale);

	lower_vertex[i] = osg::Vec3(cTheta * lower_radius,
                                    sTheta * lower_radius,
                                    lower_elev * vscale);

	bottom_vertex[i] = osg::Vec3(cTheta * bottom_radius,
                                     sTheta * bottom_radius,
                                     bottom_elev * vscale);
    }

    // generate the center disk vertex/color arrays
    center_disk_vl->push_back(center_vertex);
    for ( int i = 11; i >= 0; --i )
	center_disk_vl->push_back(upper_vertex[i]);
    center_disk_vl->push_back(upper_vertex[11]);

    // generate the upper ring
    for ( int i = 0; i < 12; ++i ) {
	upper_ring_vl->push_back( middle_vertex[i] );
	upper_ring_vl->push_back( upper_vertex[i] );
    }
    upper_ring_vl->push_back( middle_vertex[0] );
    upper_ring_vl->push_back( upper_vertex[0] );

    // generate middle ring
    for ( int i = 0; i < 12; i++ ) {
	middle_ring_vl->push_back( lower_vertex[i] );
	middle_ring_vl->push_back( middle_vertex[i] );
    }
    middle_ring_vl->push_back( lower_vertex[0] );
    middle_ring_vl->push_back( middle_vertex[0] );

    // generate lower ring
    for ( int i = 0; i < 12; i++ ) {
	lower_ring_vl->push_back( bottom_vertex[i] );
	lower_ring_vl->push_back( lower_vertex[i] );
    }
    lower_ring_vl->push_back( bottom_vertex[0] );
    lower_ring_vl->push_back( lower_vertex[0] );

    // force a repaint of the sky colors with ugly defaults
    repaint(SGVec3f(1, 1, 1), SGVec3f(1, 1, 1), 0.0, 5000.0 );

    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    osg::Geometry* geometry = new osg::Geometry;
    geometry->setName("Dome Center");
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(center_disk_vl.get());
    geometry->setColorArray(center_disk_cl.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_FAN, 0, 14));
    geode->addDrawable(geometry);

    geometry = new osg::Geometry;
    geometry->setName("Dome Upper Ring");
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(upper_ring_vl.get());
    geometry->setColorArray(upper_ring_cl.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 26));
    geode->addDrawable(geometry);

    geometry = new osg::Geometry;
    geometry->setName("Dome Middle Ring");
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(middle_ring_vl.get());
    geometry->setColorArray(middle_ring_cl.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 26));
    geode->addDrawable(geometry);

    geometry = new osg::Geometry;
    geometry->setName("Dome Lower Ring");
    geometry->setUseDisplayList(false);
    geometry->setVertexArray(lower_ring_vl.get());
    geometry->setColorArray(lower_ring_cl.get());
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setNormalBinding(osg::Geometry::BIND_OFF);
    geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, 26));
    geode->addDrawable(geometry);

    dome_transform = new osg::MatrixTransform;
    dome_transform->addChild(geode);

    return dome_transform.get();
}

static void fade_to_black(osg::Vec3 sky_color[], float asl, int count) {
    const float ref_asl = 10000.0f;
    float d = exp( - asl / ref_asl );
    for(int i = 0; i < count ; i++) {
        float f = 1 - d;
        sky_color[i][0] = sky_color[i][0] - f * sky_color[i][0] ;
        sky_color[i][1] = sky_color[i][1] - f * sky_color[i][1] ;
        sky_color[i][2] = sky_color[i][2] - f * sky_color[i][2] ;
    }
}

// repaint the sky colors based on current value of sun_angle, sky,
// and fog colors.  This updates the color arrays for ssgVtxTable.
// sun angle in degrees relative to verticle
// 0 degrees = high noon
// 90 degrees = sun rise/set
// 180 degrees = darkest midnight
bool
SGSkyDome::repaint( const SGVec3f& sky_color, const SGVec3f& fog_color,
                    double sun_angle, double vis )
{
    SGVec3f outer_param, outer_diff;
    SGVec3f middle_param, middle_diff;

    // Check for sunrise/sunset condition
    if (sun_angle > 80)
    {
	// 0.0 - 0.4
        outer_param[0] = (10.0 - fabs(90.0 - sun_angle)) / 20.0;
        outer_param[1] = (10.0 - fabs(90.0 - sun_angle)) / 40.0;
        outer_param[2] = -(10.0 - fabs(90.0 - sun_angle)) / 30.0;

	middle_param[0] = (10.0 - fabs(90.0 - sun_angle)) / 40.0;
        middle_param[1] = (10.0 - fabs(90.0 - sun_angle)) / 80.0;
        middle_param[2] = 0.0;

	outer_diff = (1.0 / 6.0) * outer_param;
	middle_diff = (1.0 / 6.0) * middle_param;
    } else {
        outer_param = SGVec3f(0, 0, 0);
	middle_param = SGVec3f(0, 0, 0);

	outer_diff = SGVec3f(0, 0, 0);
	middle_diff = SGVec3f(0, 0, 0);
    }
    // printf("  outer_red_param = %.2f  outer_red_diff = %.2f\n", 
    //        outer_red_param, outer_red_diff);

    // calculate transition colors between sky and fog
    SGVec3f outer_amt = outer_param;
    SGVec3f middle_amt = middle_param;

    //
    // First, recalulate the basic colors
    //

    osg::Vec3 center_color;
    osg::Vec3 upper_color[12];
    osg::Vec3 middle_color[12];
    osg::Vec3 lower_color[12];
    osg::Vec3 bottom_color[12];

    double vis_factor, cvf = vis;

    if (cvf > 45000)
        cvf = 45000;

    vis_factor = (vis - 1000.0) / 2000.0;
    if ( vis_factor < 0.0 ) {
        vis_factor = 0.0;
    } else if ( vis_factor > 1.0) {
        vis_factor = 1.0;
    }

    center_color = sky_color.osg();

    for ( int i = 0; i < 6; i++ ) {
	for ( int j = 0; j < 3; j++ ) {
            double saif = sun_angle/SG_PI;
	    double diff = (sky_color[j] - fog_color[j])
              * (0.8 + j * 0.2) * (0.8 + saif - ((6-i)/10));

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        l->sky_color[j], l->fog_color[j], diff);

	    upper_color[i][j] = sky_color[j] - diff *
              ( 1.0 - vis_factor * (0.7 + 0.3 * cvf/45000) );
	    middle_color[i][j] = sky_color[j] - diff *
              ( 1.0 - vis_factor * (0.1 + 0.85 * cvf/45000) ) + middle_amt[j];
	    lower_color[i][j] = fog_color[j] + outer_amt[j];

	    if ( upper_color[i][j] > 1.0 ) { upper_color[i][j] = 1.0; }
	    if ( upper_color[i][j] < 0.0 ) { upper_color[i][j] = 0.0; }
	    if ( middle_color[i][j] > 1.0 ) { middle_color[i][j] = 1.0; }
	    if ( middle_color[i][j] < 0.0 ) { middle_color[i][j] = 0.0; }
	    if ( lower_color[i][j] > 1.0 ) { lower_color[i][j] = 1.0; }
	    if ( lower_color[i][j] < 0.0 ) { lower_color[i][j] = 0.0; }
	}

        outer_amt -= outer_diff;
        middle_amt -= middle_diff;

	/*
	printf("upper_color[%d] = %.2f %.2f %.2f %.2f\n", i, upper_color[i][0],
	       upper_color[i][1], upper_color[i][2], upper_color[i][3]);
	printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	       middle_color[i][3]);
	printf("lower_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       lower_color[i][0], lower_color[i][1], lower_color[i][2], 
	       lower_color[i][3]);
	*/
    }

    outer_amt = SGVec3f(0, 0, 0);
    middle_amt = SGVec3f(0, 0, 0);

    for ( int i = 6; i < 12; i++ ) {
	for ( int j = 0; j < 3; j++ ) {
            double saif = sun_angle/SGD_PI;
            double diff = (sky_color[j] - fog_color[j])
              * (0.8 + j * 0.2) * (0.8 + saif - ((-i+12)/10));

	    // printf("sky = %.2f  fog = %.2f  diff = %.2f\n", 
	    //        sky_color[j], fog_color[j], diff);

	    upper_color[i][j] = sky_color[j] - diff *
              ( 1.0 - vis_factor * (0.7 + 0.3 * cvf/45000) );
	    middle_color[i][j] = sky_color[j] - diff *
              ( 1.0 - vis_factor * (0.1 + 0.85 * cvf/45000) ) + middle_amt[j];
	    lower_color[i][j] = fog_color[j] + outer_amt[j];

	    if ( upper_color[i][j] > 1.0 ) { upper_color[i][j] = 1.0; }
	    if ( upper_color[i][j] < 0.0 ) { upper_color[i][j] = 0.0; }
	    if ( middle_color[i][j] > 1.0 ) { middle_color[i][j] = 1.0; }
	    if ( middle_color[i][j] < 0.0 ) { middle_color[i][j] = 0.0; }
	    if ( lower_color[i][j] > 1.0 ) { lower_color[i][j] = 1.0; }
	    if ( lower_color[i][j] < 0.0 ) { lower_color[i][j] = 0.0; }
	}

        outer_amt += outer_diff;
        middle_amt += middle_diff;

	/*
	printf("upper_color[%d] = %.2f %.2f %.2f %.2f\n", i, upper_color[i][0],
	       upper_color[i][1], upper_color[i][2], upper_color[i][3]);
	printf("middle_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       middle_color[i][0], middle_color[i][1], middle_color[i][2], 
	       middle_color[i][3]);
	printf("lower_color[%d] = %.2f %.2f %.2f %.2f\n", i, 
	       lower_color[i][0], lower_color[i][1], lower_color[i][2], 
	       lower_color[i][3]);
 	*/
   }

    fade_to_black( &center_color, asl * center_elev, 1);
    fade_to_black( upper_color, (asl+0.05f) * upper_elev, 12);
    fade_to_black( middle_color, (asl+0.05f) * middle_elev, 12);
    fade_to_black( lower_color, (asl+0.05f) * lower_elev, 12);

    for ( int i = 0; i < 12; i++ )
        bottom_color[i] = fog_color.osg();

    //
    // Second, assign the basic colors to the object color arrays
    //

    // update the center disk color arrays
    int counter = 0;
    (*center_disk_cl)[counter++] = center_color;
    for ( int i = 11; i >= 0; i-- ) {
        (*center_disk_cl)[counter++] = upper_color[i];
    }
    (*center_disk_cl)[counter++] = upper_color[11];
    center_disk_cl->dirty();

    // generate the upper ring
    counter = 0;
    for ( int i = 0; i < 12; i++ ) {
        (*upper_ring_cl)[counter++] = middle_color[i];
	(*upper_ring_cl)[counter++] = upper_color[i];
    }
    (*upper_ring_cl)[counter++] = middle_color[0];
    (*upper_ring_cl)[counter++] = upper_color[0];
    upper_ring_cl->dirty();

    // generate middle ring
    counter = 0;
    for ( int i = 0; i < 12; i++ ) {
        (*middle_ring_cl)[counter++] = lower_color[i];
        (*middle_ring_cl)[counter++] = middle_color[i];
    }
    (*middle_ring_cl)[counter++] = lower_color[0];
    (*middle_ring_cl)[counter++] = middle_color[0];
    middle_ring_cl->dirty();

    // generate lower ring
    counter = 0;
    for ( int i = 0; i < 12; i++ ) {
        (*lower_ring_cl)[counter++] = bottom_color[i];
        (*lower_ring_cl)[counter++] = lower_color[i];
    }
    (*lower_ring_cl)[counter++] = bottom_color[0];
    (*lower_ring_cl)[counter++] = lower_color[0];
    lower_ring_cl->dirty();

    return true;
}


// reposition the sky at the specified origin and orientation
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (and orients the
// sunrise/set effects
bool
SGSkyDome::reposition( const SGVec3f& p, double _asl,
                       double lon, double lat, double spin ) {
    asl = _asl;

    osg::Matrix T, LON, LAT, SPIN;

    // Translate to view position
    // Point3D zero_elev = current_view.get_cur_zero_elev();
    // xglTranslatef( zero_elev.x(), zero_elev.y(), zero_elev.z() );
    T.makeTranslate( p.osg() );

    // printf("  Translated to %.2f %.2f %.2f\n", 
    //        zero_elev.x, zero_elev.y, zero_elev.z );

    // Rotate to proper orientation
    // printf("  lon = %.2f  lat = %.2f\n",
    //        lon * SGD_RADIANS_TO_DEGREES,
    //        lat * SGD_RADIANS_TO_DEGREES);
    // xglRotatef( lon * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0 );
    LON.makeRotate(lon, osg::Vec3(0, 0, 1));

    // xglRotatef( 90.0 - f->get_Latitude() * SGD_RADIANS_TO_DEGREES,
    //             0.0, 1.0, 0.0 );
    LAT.makeRotate(90.0 * SGD_DEGREES_TO_RADIANS - lat, osg::Vec3(0, 1, 0));

    // xglRotatef( l->sun_rotation * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0 );
    SPIN.makeRotate(spin, osg::Vec3(0, 0, 1));

    dome_transform->setMatrix( SPIN*LAT*LON*T );
    return true;
}
