// cloud.cxx -- model a single cloud layer
//
// Written by Curtis Olson, started June 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - curt@flightgear.org
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


#include <simgear/compiler.h>

// #include <stdio.h>
#include <math.h>

// #if defined (__APPLE__) 
// // any C++ header file undefines isinf and isnan
// // so this should be included before <iostream>
// inline int (isinf)(double r) { return isinf(r); }
// inline int (isnan)(double r) { return isnan(r); } 
// #endif

// #include STL_IOSTREAM

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include "cloud.hxx"


static ssgSimpleState *layer_states[SGCloudLayer::SG_MAX_CLOUD_COVERAGES];
static bool state_initialized = false;


// Constructor
SGCloudLayer::SGCloudLayer( const string &tex_path ) :
    layer_root(new ssgRoot),
    layer_transform(new ssgTransform),
    texture_path(tex_path),
    layer_span(0.0),
    layer_asl(0.0),
    layer_thickness(0.0),
    layer_transition(0.0),
    layer_coverage(SG_CLOUD_CLEAR),
    scale(4000.0),
    speed(0.0),
    direction(0.0),
    last_lon(0.0),
    last_lat(0.0)
{
    cl[0] = cl[1] = cl[2] = cl[3] = NULL;
    vl[0] = vl[1] = vl[2] = vl[3] = NULL;
    tl[0] = tl[1] = tl[2] = tl[3] = NULL;
    layer[0] = layer[1] = layer[2] = layer[3] = NULL;

    layer_root->addKid(layer_transform);
    rebuild();
}

// Destructor
SGCloudLayer::~SGCloudLayer()
{
    delete layer_root;		// deletes layer_transform and layer as well
}

float
SGCloudLayer::getSpan_m () const
{
    return layer_span;
}

void
SGCloudLayer::setSpan_m (float span_m)
{
    if (span_m != layer_span) {
        layer_span = span_m;
        rebuild();
    }
}

float
SGCloudLayer::getElevation_m () const
{
    return layer_asl;
}

void
SGCloudLayer::setElevation_m (float elevation_m, bool set_span)
{
    layer_asl = elevation_m;

    if (set_span) {
        if (elevation_m > 4000)
            setSpan_m(  elevation_m * 10 );
        else
            setSpan_m( 40000 );
    }
}

float
SGCloudLayer::getThickness_m () const
{
    return layer_thickness;
}

void
SGCloudLayer::setThickness_m (float thickness_m)
{
    layer_thickness = thickness_m;
}

float
SGCloudLayer::getTransition_m () const
{
    return layer_transition;
}

void
SGCloudLayer::setTransition_m (float transition_m)
{
    layer_transition = transition_m;
}

SGCloudLayer::Coverage
SGCloudLayer::getCoverage () const
{
    return layer_coverage;
}

void
SGCloudLayer::setCoverage (Coverage coverage)
{
    if (coverage != layer_coverage) {
        layer_coverage = coverage;
        rebuild();
    }
}


// build the cloud object
void
SGCloudLayer::rebuild()
{
    // Initialize states and sizes if necessary.
    if ( !state_initialized ) { 
        state_initialized = true;

        SG_LOG(SG_ASTRO, SG_INFO, "initializing cloud layers");

        SGPath cloud_path;

        cloud_path.set(texture_path.str());
        cloud_path.append("overcast.rgb");
        layer_states[SG_CLOUD_OVERCAST] = sgCloudMakeState(cloud_path.str());

        cloud_path.set(texture_path.str());
        cloud_path.append("broken.rgba");
        layer_states[SG_CLOUD_BROKEN]
            = sgCloudMakeState(cloud_path.str());

        cloud_path.set(texture_path.str());
        cloud_path.append("scattered.rgba");
        layer_states[SG_CLOUD_SCATTERED]
            = sgCloudMakeState(cloud_path.str());

        cloud_path.set(texture_path.str());
        cloud_path.append("few.rgba");
        layer_states[SG_CLOUD_FEW]
            = sgCloudMakeState(cloud_path.str());

        cloud_path.set(texture_path.str());
        cloud_path.append("cirrus.rgba");
        layer_states[SG_CLOUD_CIRRUS]
            = sgCloudMakeState(cloud_path.str());

        layer_states[SG_CLOUD_CLEAR] = 0;
    }

    scale = 4000.0;
    last_lon = last_lat = -999.0f;

    sgVec2 base;
    sgSetVec2( base, sg_random(), sg_random() );

    // build the cloud layer
    sgVec4 color;
    sgVec3 vertex;
    sgVec2 tc;

    const float layer_scale = layer_span / scale;
    const float mpi = SG_PI/4;

    // caclculate the difference between a flat-earth model and 
    // a round earth model given the span and altutude ASL of
    // the cloud layer. This is the difference in altitude between
    // the top of the inverted bowl and the edge of the bowl.
    // const float alt_diff = layer_asl * 0.8;
    const float layer_to_core = (SG_EARTH_RAD * 1000 + layer_asl);
    const float layer_angle = acos( 0.5*layer_span / layer_to_core);
    const float border_to_core = layer_to_core * sin(layer_angle);
    const float alt_diff = layer_to_core - border_to_core;

    for (int i = 0; i < 4; i++)
    {
        if ( layer[i] != NULL ) {
            layer_transform->removeKid(layer[i]); // automatic delete
        }

        vl[i] = new ssgVertexArray( 10 );
        cl[i] = new ssgColourArray( 10 );
        tl[i] = new ssgTexCoordArray( 10 );


        sgSetVec3( vertex, layer_span*(i-2)/2, -layer_span,
                           alt_diff * (sin(i*mpi) - 2) );

        sgSetVec2( tc, base[0] + layer_scale * i/4, base[1] );

        sgSetVec4( color, 1.0f, 1.0f, 1.0f, (i == 0) ? 0.0f : 0.15f );

        cl[i]->add( color );
        vl[i]->add( vertex );
        tl[i]->add( tc );

        for (int j = 0; j < 4; j++)
        {
            sgSetVec3( vertex, layer_span*(i-1)/2, layer_span*(j-2)/2,
                               alt_diff * (sin((i+1)*mpi) + sin(j*mpi) - 2) );

            sgSetVec2( tc, base[0] + layer_scale * (i+1)/4,
                           base[1] + layer_scale * j/4 );

            sgSetVec4( color, 1.0f, 1.0f, 1.0f,
                              ( (j == 0) || (i == 3)) ?  
                              ( (j == 0) && (i == 3)) ? 0.0f : 0.15f : 1.0f );

            cl[i]->add( color );
            vl[i]->add( vertex );
            tl[i]->add( tc );


            sgSetVec3( vertex, layer_span*(i-2)/2, layer_span*(j-1)/2,
                               alt_diff * (sin(i*mpi) + sin((j+1)*mpi) - 2) );

            sgSetVec2( tc, base[0] + layer_scale * i/4,
                           base[1] + layer_scale * (j+1)/4 );

            sgSetVec4( color, 1.0f, 1.0f, 1.0f,
                              ((j == 3) || (i == 0)) ?
                              ((j == 3) && (i == 0)) ? 0.0f : 0.15f : 1.0f );
            cl[i]->add( color );
            vl[i]->add( vertex );
            tl[i]->add( tc );
        }

        sgSetVec3( vertex, layer_span*(i-1)/2, layer_span, 
                           alt_diff * (sin((i+1)*mpi) - 2) );

        sgSetVec2( tc, base[0] + layer_scale * (i+1)/4,
                       base[1] + layer_scale );

        sgSetVec4( color, 1.0f, 1.0f, 1.0f, (i == 3) ? 0.0f : 0.15f );

        cl[i]->add( color );
        vl[i]->add( vertex );
        tl[i]->add( tc );

        layer[i] = new ssgVtxTable(GL_TRIANGLE_STRIP, vl[i], NULL, tl[i], cl[i]);
        layer_transform->addKid( layer[i] );

        if ( layer_states[layer_coverage] != NULL ) {
            layer[i]->setState( layer_states[layer_coverage] );
        }
    }

    // force a repaint of the sky colors with arbitrary defaults
    repaint( color );

}


// repaint the cloud layer colors
bool SGCloudLayer::repaint( sgVec3 fog_color ) {
    float *color;

    for ( int i = 0; i < 4; i++ )
        for ( int j = 0; j < 10; ++j ) {
            color = cl[i]->get( j );
            sgCopyVec3( color, fog_color );
        }

    return true;
}


// reposition the cloud layer at the specified origin and orientation
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (and orients the
// sunrise/set effects
bool SGCloudLayer::reposition( sgVec3 p, sgVec3 up, double lon, double lat,
        		       double alt, double dt )
{
    sgMat4 T1, LON, LAT;
    sgVec3 axis;

    // combine p and asl (meters) to get translation offset
    sgVec3 asl_offset;
    sgCopyVec3( asl_offset, up );
    sgNormalizeVec3( asl_offset );
    if ( alt <= layer_asl ) {
        sgScaleVec3( asl_offset, layer_asl );
    } else {
        sgScaleVec3( asl_offset, layer_asl + layer_thickness );
    }
    // cout << "asl_offset = " << asl_offset[0] << "," << asl_offset[1]
    //      << "," << asl_offset[2] << endl;
    sgAddVec3( asl_offset, p );
    // cout << "  asl_offset = " << asl_offset[0] << "," << asl_offset[1]
    //      << "," << asl_offset[2] << endl;

    // Translate to zero elevation
    // Point3D zero_elev = current_view.get_cur_zero_elev();
    // xglTranslatef( zero_elev.x(), zero_elev.y(), zero_elev.z() );
    sgMakeTransMat4( T1, asl_offset );

    // printf("  Translated to %.2f %.2f %.2f\n", 
    //        zero_elev.x, zero_elev.y, zero_elev.z );

    // Rotate to proper orientation
    // printf("  lon = %.2f  lat = %.2f\n", 
    //        lon * SGD_RADIANS_TO_DEGREES,
    //        lat * SGD_RADIANS_TO_DEGREES);
    // xglRotatef( lon * SGD_RADIANS_TO_DEGREES, 0.0, 0.0, 1.0 );
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( LON, lon * SGD_RADIANS_TO_DEGREES, axis );

    // xglRotatef( 90.0 - f->get_Latitude() * SGD_RADIANS_TO_DEGREES,
    //             0.0, 1.0, 0.0 );
    sgSetVec3( axis, 0.0, 1.0, 0.0 );
    sgMakeRotMat4( LAT, 90.0 - lat * SGD_RADIANS_TO_DEGREES, axis );

    sgMat4 TRANSFORM;

    sgCopyMat4( TRANSFORM, T1 );
    sgPreMultMat4( TRANSFORM, LON );
    sgPreMultMat4( TRANSFORM, LAT );

    sgCoord layerpos;
    sgSetCoord( &layerpos, TRANSFORM );

    layer_transform->setTransform( &layerpos );

    // now calculate update texture coordinates
    if ( last_lon < -900 ) {
        last_lon = lon;
        last_lat = lat;
    }

    double sp_dist = speed*dt;

    if ( lon != last_lon || lat != last_lat || sp_dist != 0 ) {
        Point3D start( last_lon, last_lat, 0.0 );
        Point3D dest( lon, lat, 0.0 );
        double course = 0.0, dist = 0.0;

        calc_gc_course_dist( dest, start, &course, &dist );
        // cout << "course = " << course << ", dist = " << dist << endl;

        // if start and dest are too close together,
        // calc_gc_course_dist() can return a course of "nan".  If
        // this happens, lets just use the last known good course.
        // This is a hack, and it would probably be better to make
        // calc_gc_course_dist() more robust.
        if ( isnan(course) ) {
            course = last_course;
        } else {
            last_course = course;
        }

        // calculate cloud movement due to external forces
        double ax = 0.0, ay = 0.0, bx = 0.0, by = 0.0;

        if (dist > 0.0) {
            ax = cos(course) * dist;
            ay = sin(course) * dist;
        }

        if (sp_dist > 0) {
            bx = cos(-direction * SGD_DEGREES_TO_RADIANS) * sp_dist;
            by = sin(-direction * SGD_DEGREES_TO_RADIANS) * sp_dist;
        }


        double xoff = (ax + bx) / (2 * scale);
        double yoff = (ay + by) / (2 * scale);

        const float layer_scale = layer_span / scale;

        // cout << "xoff = " << xoff << ", yoff = " << yoff << endl;

        float *base, *tc;

        base = tl[0]->get( 0 );
        base[0] += xoff;

        // the while loops can lead to *long* pauses if base[0] comes
        // with a bogus value.
        // while ( base[0] > 1.0 ) { base[0] -= 1.0; }
        // while ( base[0] < 0.0 ) { base[0] += 1.0; }
        if ( base[0] > -10.0 && base[0] < 10.0 ) {
            base[0] -= (int)base[0];
        } else {
            SG_LOG(SG_ASTRO, SG_DEBUG,
                   "Error: base = " << base[0] << "," << base[1] <<
                   " course = " << course << " dist = " << dist );
            base[0] = 0.0;
        }

        base[1] += yoff;
        // the while loops can lead to *long* pauses if base[0] comes
        // with a bogus value.
        // while ( base[1] > 1.0 ) { base[1] -= 1.0; }
        // while ( base[1] < 0.0 ) { base[1] += 1.0; }
        if ( base[1] > -10.0 && base[1] < 10.0 ) {
           base[1] -= (int)base[1];
        } else {
           SG_LOG(SG_ASTRO, SG_ALERT,
                  "Error: base = " << base[0] << "," << base[1] <<
                  " course = " << course << " dist = " << dist );
           base[1] = 0.0;
        }

        // cout << "base = " << base[0] << "," << base[1] << endl;

        for (int i = 0; i < 4; i++)
        {
            tc = tl[i]->get( 0 );
            sgSetVec2( tc, base[0] + layer_scale * i/4, base[1] );
            
            for (int j = 0; j < 4; j++)
            {
                tc = tl[i]->get( j*2+1 );
                sgSetVec2( tc, base[0] + layer_scale * (i+1)/4,
                               base[1] + layer_scale * j/4 );
 
        	tc = tl[i]->get( (j+1)*2 );
                sgSetVec2( tc, base[0] + layer_scale * i/4,
                               base[1] + layer_scale * (j+1)/4 );
            }
 
            tc = tl[i]->get( 9 );
            sgSetVec2( tc, base[0] + layer_scale * (i+1)/4,
                           base[1] + layer_scale );
        }
 
        last_lon = lon;
        last_lat = lat;
    }

    return true;
}


void SGCloudLayer::draw() {
    if ( layer_coverage != SG_CLOUD_CLEAR ) {
        ssgCullAndDraw( layer_root );
    }
}


// make an ssgSimpleState for a cloud layer given the named texture
ssgSimpleState *sgCloudMakeState( const string &path ) {
    ssgSimpleState *state = new ssgSimpleState();

    SG_LOG(SG_ASTRO, SG_INFO, " texture = ");

    state->setTexture( (char *)path.c_str() );
    state->setShadeModel( GL_SMOOTH );
    state->disable( GL_LIGHTING );
    state->disable( GL_CULL_FACE );
    state->enable( GL_TEXTURE_2D );
    state->enable( GL_COLOR_MATERIAL );
    state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    state->setMaterial( GL_EMISSION, 0.05, 0.05, 0.05, 0.0 );
    state->setMaterial( GL_AMBIENT, 0.2, 0.2, 0.2, 0.0 );
    state->setMaterial( GL_DIFFUSE, 0.5, 0.5, 0.5, 0.0 );
    state->setMaterial( GL_SPECULAR, 0.0, 0.0, 0.0, 0.0 );
    state->enable( GL_BLEND );
    state->enable( GL_ALPHA_TEST );
    state->setAlphaClamp( 0.01 );

    // ref() the state so it doesn't get deleted if the last layer of
    // it's type is deleted.
    state->ref();

    return state;
}
