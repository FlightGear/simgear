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

#include <stdio.h>
#include STL_IOSTREAM

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include "cloud.hxx"

ssgSimpleState *
SGCloudLayer::layer_states[SGCloudLayer::SG_MAX_CLOUD_TYPES];


// Constructor
SGCloudLayer::SGCloudLayer( const string &tex_path )
  : layer_root(new ssgRoot),
    layer_transform(new ssgTransform),
    layer(0),
    texture_path(tex_path),
    layer_span(0),
    layer_asl(0),
    layer_thickness(0),
    layer_transition(0),
    layer_type(SG_CLOUD_CLEAR)
{
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
SGCloudLayer::setElevation_m (float elevation_m)
{
    layer_asl = elevation_m;
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

SGCloudLayer::Type
SGCloudLayer::getType () const
{
    return layer_type;
}

void
SGCloudLayer::setType (Type type)
{
    if (type != layer_type) {
	layer_type = type;
	rebuild();
    }
}


// build the cloud object
void
SGCloudLayer::rebuild()
{
				// Initialize states and sizes if necessary.
    if (layer_states[0] == 0) {
      SGPath cloud_path;

      cloud_path.set(texture_path.str());
      cloud_path.append("overcast.rgb");
      layer_states[SG_CLOUD_OVERCAST] = SGCloudMakeState(cloud_path.str());

      cloud_path.set(texture_path.str());
      cloud_path.append("mostlycloudy.rgba");
      layer_states[SG_CLOUD_MOSTLY_CLOUDY] =
	SGCloudMakeState(cloud_path.str());

      cloud_path.set(texture_path.str());
      cloud_path.append("mostlysunny.rgba");
      layer_states[SG_CLOUD_MOSTLY_SUNNY] = SGCloudMakeState(cloud_path.str());

      cloud_path.set(texture_path.str());
      cloud_path.append("cirrus.rgba");
      layer_states[SG_CLOUD_CIRRUS] = SGCloudMakeState(cloud_path.str());

      layer_states[SG_CLOUD_CLEAR] = 0;
    }

    scale = 4000.0;

    last_lon = last_lat = -999.0f;

    cl = new ssgColourArray( 4 );
    vl = new ssgVertexArray( 4 );
    tl = new ssgTexCoordArray( 4 );

    // build the cloud layer
    sgVec4 color;
    sgVec3 vertex;
    sgVec2 tc;
    sgSetVec4( color, 1.0f, 1.0f, 1.0f, 1.0f );

    sgSetVec3( vertex, -layer_span, -layer_span, 0.0f );
    sgVec2 base;
    sgSetVec2( base, sg_random(), sg_random() );
    sgSetVec2( tc, base[0], base[1] );
    cl->add( color );
    vl->add( vertex );
    tl->add( tc );

    sgSetVec3( vertex, layer_span, -layer_span, 0.0f );
    sgSetVec2( tc, base[0] + layer_span / scale, base[1] );
    cl->add( color );
    vl->add( vertex );
    tl->add( tc );

    sgSetVec3( vertex, -layer_span, layer_span, 0.0f );
    sgSetVec2( tc, base[0], base[1] + layer_span / scale );
    cl->add( color );
    vl->add( vertex );
    tl->add( tc );

    sgSetVec3( vertex, layer_span, layer_span, 0.0f );
    sgSetVec2( tc, base[0] + layer_span / scale, base[1] + layer_span / scale );
    cl->add( color );
    vl->add( vertex );
    tl->add( tc );

    if (layer != 0)
      layer_transform->removeKid(layer); // automatic delete
    layer = new ssgVtxTable ( GL_TRIANGLE_STRIP, vl, NULL, tl, cl );
    if (layer_states[layer_type] != 0)
      layer->setState( layer_states[layer_type] );

    // force a repaint of the moon colors with arbitrary defaults
    repaint( color );

    // moon_transform->addKid( halo );
    layer_transform->addKid( layer );
}


// repaint the cloud layer colors
bool SGCloudLayer::repaint( sgVec3 fog_color ) {
    float *color;

    for ( int i = 0; i < 4; ++i ) {
	color = cl->get( i );
	sgCopyVec4( color, fog_color );
    }

    return true;
}


// reposition the cloud layer at the specified origin and orientation
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (and orients the
// sunrise/set effects
bool SGCloudLayer::reposition( sgVec3 p, sgVec3 up, double lon, double lat,
			       double alt )
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

    if ( lon != last_lon || lat != last_lat ) {
	Point3D start( last_lon, last_lat, 0.0 );
	Point3D dest( lon, lat, 0.0 );
	double course, dist;
	calc_gc_course_dist( dest, start, &course, &dist );
	// cout << "course = " << course << ", dist = " << dist << endl;

	double xoff = cos( course ) * dist / (2 * scale);
	double yoff = sin( course ) * dist / (2 * scale);

	// cout << "xoff = " << xoff << ", yoff = " << yoff << endl;

	float *base, *tc;
	base = tl->get( 0 );

	base[0] += xoff;

	// the while loops can lead to *long* pauses if base[0] comes
	// with a bogus value.
        // while ( base[0] > 1.0 ) { base[0] -= 1.0; }
	// while ( base[0] < 0.0 ) { base[0] += 1.0; }
        if ( base[0] > -10.0 && base[0] < 10.0 ) {
            base[0] -= (int)base[0];
        } else {
            base[0] = 0.0;
	    SG_LOG(SG_ASTRO, SG_DEBUG,
		   "Error: base = " << base[0] << "," << base[1]);
        }

	base[1] += yoff;
	// the while loops can lead to *long* pauses if base[0] comes
	// with a bogus value.
	// while ( base[1] > 1.0 ) { base[1] -= 1.0; }
	// while ( base[1] < 0.0 ) { base[1] += 1.0; }
        if ( base[1] > -10.0 && base[1] < 10.0 ) {
            base[1] -= (int)base[1];
        } else {
            base[1] = 0.0;
	    SG_LOG(SG_ASTRO, SG_ALERT,
		   "Error: base = " << base[0] << "," << base[1]);
        }

	// cout << "base = " << base[0] << "," << base[1] << endl;

	tc = tl->get( 1 );
	sgSetVec2( tc, base[0] + layer_span / scale, base[1] );
 
	tc = tl->get( 2 );
	sgSetVec2( tc, base[0], base[1] + layer_span / scale );
 
	tc = tl->get( 3 );
	sgSetVec2( tc, base[0] + layer_span / scale, base[1] + layer_span / scale );
 
	last_lon = lon;
	last_lat = lat;
    }

    return true;
}


void SGCloudLayer::draw() {
    if (layer_type != SG_CLOUD_CLEAR)
      ssgCullAndDraw( layer_root );
}


// make an ssgSimpleState for a cloud layer given the named texture
ssgSimpleState *SGCloudMakeState( const string &path ) {
    ssgSimpleState *state = new ssgSimpleState();

    state->setTexture( (char *)path.c_str() );
    state->setShadeModel( GL_SMOOTH );
    state->disable( GL_LIGHTING );
    state->disable( GL_CULL_FACE );
    state->enable( GL_TEXTURE_2D );
    state->enable( GL_COLOR_MATERIAL );
    state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    state->enable( GL_BLEND );
    state->enable( GL_ALPHA_TEST );
    state->setAlphaClamp( 0.01 );

    // ref() the state so it doesn't get deleted if the last layer of
    // it's type is deleted.
    state->ref();

    return state;
}
