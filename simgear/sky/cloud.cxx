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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <iostream>

#include <plib/ssg.h>

#include <simgear/constants.h>

#include "cloud.hxx"


// Constructor
SGCloudLayer::SGCloudLayer( void ) {
}


// Destructor
SGCloudLayer::~SGCloudLayer( void ) {
}


// build the moon object
ssgBranch * SGCloudLayer::build( FGPath path, double size, double asl ) {

    layer_asl = asl;

    // set up the cloud state
    path.append( "cloud.rgba" );
    layer_state = new ssgSimpleState();
    layer_state->setTexture( (char *)path.c_str() );
    layer_state->setShadeModel( GL_SMOOTH );
    layer_state->disable( GL_LIGHTING );
    layer_state->disable( GL_CULL_FACE );
    layer_state->enable( GL_TEXTURE_2D );
    layer_state->enable( GL_COLOR_MATERIAL );
    layer_state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    layer_state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    layer_state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
    layer_state->enable( GL_BLEND );
    layer_state->enable( GL_ALPHA_TEST );
    layer_state->setAlphaClamp( 0.01 );

    cl = new ssgColourArray( 4 );
    vl = new ssgVertexArray( 4 );
    tl = new ssgTexCoordArray( 4 );

    // build the cloud layer
    sgVec4 color;
    sgVec3 vertex;
    sgVec2 tc;
    sgSetVec4( color, 1.0f, 1.0f, 1.0f, 1.0f );

    sgSetVec3( vertex, -size, -size, 0.0f );
    sgSetVec2( tc, 0.0f, 0.0f );
    cl->add( color );
    vl->add( vertex );
    tl->add( tc );

    sgSetVec3( vertex, size, -size, 0.0f );
    sgSetVec2( tc, size / 1000.0f, 0.0f );
    cl->add( color );
    vl->add( vertex );
    tl->add( tc );

    sgSetVec3( vertex, -size, size, 0.0f );
    sgSetVec2( tc, 0.0f, size / 1000.0f );
    cl->add( color );
    vl->add( vertex );
    tl->add( tc );

    sgSetVec3( vertex, size, size, 0.0f );
    sgSetVec2( tc, size / 1000.0f, size / 1000.0f );
    cl->add( color );
    vl->add( vertex );
    tl->add( tc );

    ssgLeaf *layer = 
	new ssgVtxTable ( GL_TRIANGLE_STRIP, vl, NULL, tl, cl );
    layer->setState( layer_state );

    // force a repaint of the moon colors with arbitrary defaults
    repaint( color );

    // build the ssg scene graph sub tree for the sky and connected
    // into the provide scene graph branch
    layer_transform = new ssgTransform;

    // moon_transform->addKid( halo );
    layer_transform->addKid( layer );

    return layer_transform;
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
bool SGCloudLayer::reposition( sgVec3 p, sgVec3 up, double lon, double lat ) {
    sgMat4 T1, LON, LAT;
    sgVec3 axis;

    // combine p and asl (meters) to get translation offset
    sgVec3 asl_offset;
    sgCopyVec3( asl_offset, up );
    sgNormalizeVec3( asl_offset );
    sgScaleVec3( asl_offset, layer_asl );
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
    // printf("  lon = %.2f  lat = %.2f\n", FG_Longitude * RAD_TO_DEG,
    //        FG_Latitude * RAD_TO_DEG);
    // xglRotatef( f->get_Longitude() * RAD_TO_DEG, 0.0, 0.0, 1.0 );
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( LON, lon * RAD_TO_DEG, axis );

    // xglRotatef( 90.0 - f->get_Latitude() * RAD_TO_DEG, 0.0, 1.0, 0.0 );
    sgSetVec3( axis, 0.0, 1.0, 0.0 );
    sgMakeRotMat4( LAT, 90.0 - lat * RAD_TO_DEG, axis );

    sgMat4 TRANSFORM;

    sgCopyMat4( TRANSFORM, T1 );
    sgPreMultMat4( TRANSFORM, LON );
    sgPreMultMat4( TRANSFORM, LAT );

    sgCoord layerpos;
    sgSetCoord( &layerpos, TRANSFORM );

    layer_transform->setTransform( &layerpos );

    return true;
}
