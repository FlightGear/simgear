// cloud.cxx -- model a single cloud layer
//
// Written by Curtis Olson, started June 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#if defined (__APPLE__)
// any C++ header file undefines isinf and isnan
// so this should be included before <iostream>
inline int (isinf)(double r) { return isinf(r); }
inline int (isnan)(double r) { return isnan(r); } 
#endif

#include <plib/sg.h>
#include <plib/ssg.h>

#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/screen/extensions.hxx>
#include <simgear/screen/texture.hxx>

#include "cloud.hxx"

#if defined(__MINGW32__)
#define isnan(x) _isnan(x)
#endif

#if defined (__FreeBSD__)
#  if __FreeBSD_version < 500000
     extern "C" {
       inline int isnan(double r) { return !(r <= 0 || r >= 0); }
     }
#  endif
#endif


static ssgStateSelector *layer_states[SGCloudLayer::SG_MAX_CLOUD_COVERAGES];
static bool state_initialized = false;
static bool bump_mapping = false;
static int nb_texture_unit = 0;
static ssgTexture *normal_map[SGCloudLayer::SG_MAX_CLOUD_COVERAGES][2] = { 0 };
static ssgTexture *color_map[SGCloudLayer::SG_MAX_CLOUD_COVERAGES][2] = { 0 };
static GLuint normalization_cube_map;

static glActiveTextureProc glActiveTexturePtr = 0;
static glClientActiveTextureProc glClientActiveTexturePtr = 0;
static glBlendColorProc glBlendColorPtr = 0;

bool SGCloudLayer::enable_bump_mapping = false;

static void
generateNormalizationCubeMap()
{
    unsigned char data[ 32 * 32 * 3 ];
    const int size = 32;
    const float half_size = 16.0f,
                offset = 0.5f;
    sgVec3 zero_normal;
    sgSetVec3( zero_normal, 0.5f, 0.5f, 0.5f );
    int i, j;

    unsigned char *ptr = data;
    for ( j = 0; j < size; j++ ) {
        for ( i = 0; i < size; i++ ) {
            sgVec3 tmp;
            sgSetVec3( tmp, half_size,
                            -( j + offset - half_size ),
                            -( i + offset - half_size ) );
            sgNormalizeVec3( tmp );
            sgScaleVec3( tmp, 0.5f );
            sgAddVec3( tmp, zero_normal );

            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
        }
    }
    glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
                  0, GL_RGBA8, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data );

    ptr = data;
    for ( j = 0; j < size; j++ ) {
        for ( i = 0; i < size; i++ ) {
            sgVec3 tmp;
            sgSetVec3( tmp, -half_size,
                            -( j + offset - half_size ),
                            ( i + offset - half_size ) );
            sgNormalizeVec3( tmp );
            sgScaleVec3( tmp, 0.5f );
            sgAddVec3( tmp, zero_normal );

            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
        }
    }
    glTexImage2D( GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
                  0, GL_RGBA8, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data );

    ptr = data;
    for ( j = 0; j < size; j++ ) {
        for ( i = 0; i < size; i++ ) {
            sgVec3 tmp;
            sgSetVec3( tmp, ( i + offset - half_size ),
                            half_size,
                            ( j + offset - half_size ) );
            sgNormalizeVec3( tmp );
            sgScaleVec3( tmp, 0.5f );
            sgAddVec3( tmp, zero_normal );

            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
        }
    }
    glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
                  0, GL_RGBA8, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data );

    ptr = data;
    for ( j = 0; j < size; j++ ) {
        for ( i = 0; i < size; i++ ) {
            sgVec3 tmp;
            sgSetVec3( tmp, ( i + offset - half_size ),
                            -half_size,
                            -( j + offset - half_size ) );
            sgNormalizeVec3( tmp );
            sgScaleVec3( tmp, 0.5f );
            sgAddVec3( tmp, zero_normal );

            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
        }
    }
    glTexImage2D( GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
                  0, GL_RGBA8, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data );

    ptr = data;
    for ( j = 0; j < size; j++ ) {
        for ( i = 0; i < size; i++ ) {
            sgVec3 tmp;
            sgSetVec3( tmp, ( i + offset - half_size ),
                            -( j + offset - half_size ),
                            half_size );
            sgNormalizeVec3( tmp );
            sgScaleVec3( tmp, 0.5f );
            sgAddVec3( tmp, zero_normal );

            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
        }
    }
    glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
                  0, GL_RGBA8, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data );

    ptr = data;
    for ( j = 0; j < size; j++ ) {
        for ( i = 0; i < size; i++ ) {
            sgVec3 tmp;
            sgSetVec3( tmp, -( i + offset - half_size ),
                            -( j + offset - half_size ),
                            -half_size );
            sgNormalizeVec3( tmp );
            sgScaleVec3( tmp, 0.5f );
            sgAddVec3( tmp, zero_normal );

            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
        }
    }
    glTexImage2D( GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,
                  0, GL_RGBA8, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data );
}


// Constructor
SGCloudLayer::SGCloudLayer( const string &tex_path ) :
    vertices(0),
    indices(0),
    layer_root(new ssgRoot),
    layer_transform(new ssgTransform),
    state_sel(0),
    cloud_alpha(1.0),
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
    delete vertices;
    delete indices;
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

        bump_mapping = SGIsOpenGLExtensionSupported("GL_ARB_multitexture") &&
                       SGIsOpenGLExtensionSupported("GL_ARB_texture_cube_map") &&
                       SGIsOpenGLExtensionSupported("GL_ARB_texture_env_combine") &&
                       SGIsOpenGLExtensionSupported("GL_ARB_texture_env_dot3") && 
                       SGIsOpenGLExtensionSupported("GL_ARB_imaging");

        if ( bump_mapping ) {
            glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &nb_texture_unit );
            if ( nb_texture_unit < 2 ) {
                bump_mapping = false;
            }
            //nb_texture_unit = 2; // Force the number of units for now
        }

        if ( bump_mapping ) {

            // This bump mapping code was inspired by the tutorial available at 
            // http://www.paulsprojects.net/tutorials/simplebump/simplebump.html
            // and a NVidia white paper 
            //  http://developer.nvidia.com/object/bumpmappingwithregistercombiners.html
            // The normal map textures were generated by the normal map Gimp plugin :
            //  http://nifelheim.dyndns.org/~cocidius/normalmap/
            //
            SGPath cloud_path;

            glActiveTexturePtr = (glActiveTextureProc)SGLookupFunction("glActiveTextureARB");
            glClientActiveTexturePtr = (glClientActiveTextureProc)SGLookupFunction("glClientActiveTextureARB");
            glBlendColorPtr = (glBlendColorProc)SGLookupFunction("glBlendColor");

            cloud_path.set(texture_path.str());
            cloud_path.append("overcast.rgb");
            color_map[ SG_CLOUD_OVERCAST ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            color_map[ SG_CLOUD_OVERCAST ][ 0 ]->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("overcast_n.rgb");
            normal_map[ SG_CLOUD_OVERCAST ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            normal_map[ SG_CLOUD_OVERCAST ][ 0 ]->ref();

            cloud_path.set(texture_path.str());
            cloud_path.append("overcast_top.rgb");
            color_map[ SG_CLOUD_OVERCAST ][ 1 ] = new ssgTexture( cloud_path.str().c_str() );
            color_map[ SG_CLOUD_OVERCAST ][ 1 ]->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("overcast_top_n.rgb");
            normal_map[ SG_CLOUD_OVERCAST ][ 1 ] = new ssgTexture( cloud_path.str().c_str() );
            normal_map[ SG_CLOUD_OVERCAST ][ 1 ]->ref();

            cloud_path.set(texture_path.str());
            cloud_path.append("broken.rgba");
            color_map[ SG_CLOUD_BROKEN ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            color_map[ SG_CLOUD_BROKEN ][ 0 ]->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("broken_n.rgb");
            normal_map[ SG_CLOUD_BROKEN ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            normal_map[ SG_CLOUD_BROKEN ][ 0 ]->ref();

            cloud_path.set(texture_path.str());
            cloud_path.append("scattered.rgba");
            color_map[ SG_CLOUD_SCATTERED ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            color_map[ SG_CLOUD_SCATTERED ][ 0 ]->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("scattered_n.rgb");
            normal_map[ SG_CLOUD_SCATTERED ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            normal_map[ SG_CLOUD_SCATTERED ][ 0 ]->ref();

            cloud_path.set(texture_path.str());
            cloud_path.append("few.rgba");
            color_map[ SG_CLOUD_FEW ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            color_map[ SG_CLOUD_FEW ][ 0 ]->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("few_n.rgb");
            normal_map[ SG_CLOUD_FEW ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            normal_map[ SG_CLOUD_FEW ][ 0 ]->ref();

            cloud_path.set(texture_path.str());
            cloud_path.append("cirrus.rgba");
            color_map[ SG_CLOUD_CIRRUS ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            color_map[ SG_CLOUD_CIRRUS ][ 0 ]->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("cirrus_n.rgb");
            normal_map[ SG_CLOUD_CIRRUS ][ 0 ] = new ssgTexture( cloud_path.str().c_str() );
            normal_map[ SG_CLOUD_CIRRUS ][ 0 ]->ref();

            glGenTextures( 1, &normalization_cube_map );
            glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, normalization_cube_map );
            generateNormalizationCubeMap();
            glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
            glTexParameteri( GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
        } /* else */ {
            SGPath cloud_path;
            ssgStateSelector *state_sel;
            ssgSimpleState *state;

            state_sel = new ssgStateSelector( 2 );
            state_sel->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("overcast.rgb");
            state_sel->setStep( 0, sgCloudMakeState(cloud_path.str()) );
            cloud_path.set(texture_path.str());
            cloud_path.append("overcast_top.rgb");
            state_sel->setStep( 1, sgCloudMakeState(cloud_path.str()) );
            layer_states[SG_CLOUD_OVERCAST] = state_sel;

            state_sel = new ssgStateSelector( 2 );
            state_sel->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("broken.rgba");
            state = sgCloudMakeState(cloud_path.str());
            state_sel->setStep( 0, state );
            state_sel->setStep( 1, state );
            layer_states[SG_CLOUD_BROKEN] = state_sel;

            state_sel = new ssgStateSelector( 2 );
            state_sel->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("scattered.rgba");
            state = sgCloudMakeState(cloud_path.str());
            state_sel->setStep( 0, state );
            state_sel->setStep( 1, state );
            layer_states[SG_CLOUD_SCATTERED] = state_sel;

            state_sel = new ssgStateSelector( 2 );
            state_sel->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("few.rgba");
            state = sgCloudMakeState(cloud_path.str());
            state_sel->setStep( 0, state );
            state_sel->setStep( 1, state );
            layer_states[SG_CLOUD_FEW] = state_sel;

            state_sel = new ssgStateSelector( 2 );
            state_sel->ref();
            cloud_path.set(texture_path.str());
            cloud_path.append("cirrus.rgba");
            state = sgCloudMakeState(cloud_path.str());
            state_sel->setStep( 0, state );
            state_sel->setStep( 1, state );
            layer_states[SG_CLOUD_CIRRUS] = state_sel;

            layer_states[SG_CLOUD_CLEAR] = 0;
        }
    }

    if ( bump_mapping ) {

        if ( !vertices ) {
            vertices = new CloudVertex[ 25 ];
            indices = new unsigned int[ 40 ];
        }

        sgVec2 base;
        sgSetVec2( base, sg_random(), sg_random() );

        const float layer_scale = layer_span / scale;
        const float layer_to_core = (SG_EARTH_RAD * 1000 + layer_asl);
        const float half_angle = 0.5 * layer_span / layer_to_core;

        int i;
        for ( i = -2; i <= 2; i++ ) {
            for ( int j = -2; j <= 2; j++ ) {
                CloudVertex &v1 = vertices[ (i+2)*5 + (j+2) ];
                sgSetVec3( v1.position,
                           0.5 * i * layer_span,
                           0.5 * j * layer_span,
                           -layer_to_core * ( 1 - cos( i * half_angle ) * cos( j * half_angle ) ) );
                sgSetVec2( v1.texCoord,
                           base[0] + layer_scale * i * 0.25,
                           base[1] + layer_scale * j * 0.25 );
                sgSetVec3( v1.sTangent,
                           cos( i * half_angle ),
                           0.f,
                           -sin( i * half_angle ) );
                sgSetVec3( v1.tTangent,
                           0.f,
                           cos( j * half_angle ),
                           -sin( j * half_angle ) );
                sgVectorProductVec3( v1.normal, v1.tTangent, v1.sTangent );
                sgSetVec4( v1.color, 1.0f, 1.0f, 1.0f, (i == 0) ? 0.0f : cloud_alpha * 0.15f );
            }
        }
        /*
         * 0 1 5 6 10 11 15 16 20 21
         * 1 2 6 7 11 12 16 17 21 22
         * 2 3 7 8 12 13 17 18 22 23
         * 3 4 8 9 13 14 18 19 23 24
         */
        for ( i = 0; i < 4; i++ ) {
            for ( int j = 0; j < 5; j++ ) {
                indices[ i*10 + (j*2) ]     =     i + 5 * j;
                indices[ i*10 + (j*2) + 1 ] = 1 + i + 5 * j;
            }
        }

    } /* else */ {

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
        const float layer_angle = 0.5*layer_span / layer_to_core; // The angle is half the span
        const float border_to_core = layer_to_core * cos(layer_angle);
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
            state_sel = layer_states[layer_coverage];
        }

        // force a repaint of the sky colors with arbitrary defaults
        repaint( color );
    }
}


// repaint the cloud layer colors
bool SGCloudLayer::repaint( sgVec3 fog_color ) {

    if ( bump_mapping && enable_bump_mapping ) {

        for ( int i = 0; i < 25; i++ ) {
            sgCopyVec3( vertices[ i ].color, fog_color );
        }

    } else {
        float *color;

        for ( int i = 0; i < 4; i++ ) {
            color = cl[i]->get( 0 );
            sgCopyVec3( color, fog_color );
            color[3] = (i == 0) ? 0.0f : 0.15f;

            for ( int j = 0; j < 4; ++j ) {
                color = cl[i]->get( (2*j) );
                sgCopyVec3( color, fog_color );
                color[3] = 
                    ((j == 0) || (i == 3)) ?
                    ((j == 0) && (i == 3)) ? 0.0f : 0.15f : 1.0f;

                color = cl[i]->get( (2*j) + 1 );
                sgCopyVec3( color, fog_color );
                color[3] = 
                    ((j == 3) || (i == 0)) ?
                    ((j == 3) && (i == 0)) ? 0.0f : 0.15f : 1.0f;
            }

            color = cl[i]->get( 9 );
            sgCopyVec3( color, fog_color );
            color[3] = (i == 3) ? 0.0f : 0.15f;
        }
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
    sgMakeTransMat4( T1, asl_offset );

    // printf("  Translated to %.2f %.2f %.2f\n", 
    //        zero_elev.x, zero_elev.y, zero_elev.z );

    // Rotate to proper orientation
    // printf("  lon = %.2f  lat = %.2f\n", 
    //        lon * SGD_RADIANS_TO_DEGREES,
    //        lat * SGD_RADIANS_TO_DEGREES);
    sgSetVec3( axis, 0.0, 0.0, 1.0 );
    sgMakeRotMat4( LON, lon * SGD_RADIANS_TO_DEGREES, axis );

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
            bx = cos((180.0-direction) * SGD_DEGREES_TO_RADIANS) * sp_dist;
            by = sin((180.0-direction) * SGD_DEGREES_TO_RADIANS) * sp_dist;
        }


        double xoff = (ax + bx) / (2 * scale);
        double yoff = (ay + by) / (2 * scale);

        const float layer_scale = layer_span / scale;

        // cout << "xoff = " << xoff << ", yoff = " << yoff << endl;

        float *base;
        if ( bump_mapping && enable_bump_mapping ) {
            base = vertices[12].texCoord;
        } else {
            base = tl[0]->get( 0 );
        }
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

        if ( bump_mapping && enable_bump_mapping ) {

            for ( int i = -2; i <= 2; i++ ) {
                for ( int j = -2; j <= 2; j++ ) {
                    if ( i == 0 && j == 0 )
                        continue; // Already done on base
                    CloudVertex &v1 = vertices[ (i+2)*5 + (j+2) ];
                    sgSetVec2( v1.texCoord,
                            base[0] + layer_scale * i * 0.25,
                            base[1] + layer_scale * j * 0.25 );
                }
            }

        } else {
                // cout << "base = " << base[0] << "," << base[1] << endl;

            float *tc;
            for (int i = 0; i < 4; i++) {
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
        }

        last_lon = lon;
        last_lat = lat;
    }

    return true;
}


void SGCloudLayer::draw( bool top ) {
    if ( layer_coverage != SG_CLOUD_CLEAR ) {

        if ( bump_mapping && enable_bump_mapping ) {

            sgMat4 modelview,
                   tmp,
                   transform;
            ssgGetModelviewMatrix( modelview );
            layer_transform->getTransform( transform );

            sgTransposeNegateMat4( tmp, transform );

            sgPostMultMat4( transform, modelview );
            ssgLoadModelviewMatrix( transform );

            sgVec3 lightVec;
            ssgGetLight( 0 )->getPosition( lightVec );
            sgNegateVec3( lightVec );
            sgXformVec3( lightVec, tmp );

            for ( int i = 0; i < 25; i++ ) {
                CloudVertex &v = vertices[ i ];
                sgSetVec3( v.tangentSpLight,
                           sgScalarProductVec3( v.sTangent, lightVec ),
                           sgScalarProductVec3( v.tTangent, lightVec ),
                           sgScalarProductVec3( v.normal, lightVec ) );
            }

            ssgTexture *decal = color_map[ layer_coverage ][ top ? 1 : 0 ];
            if ( top && decal == 0 ) {
                decal = color_map[ layer_coverage ][ 0 ];
            }
            ssgTexture *normal = normal_map[ layer_coverage ][ top ? 1 : 0 ];
            if ( top && normal == 0 ) {
                normal = normal_map[ layer_coverage ][ 0 ];
            }

            glDisable( GL_LIGHTING );
            glDisable( GL_CULL_FACE );
//            glDisable( GL_ALPHA_TEST );
            if ( layer_coverage == SG_CLOUD_FEW ) {
                glEnable( GL_ALPHA_TEST );
                glAlphaFunc ( GL_GREATER, 0.01 );
            }
            glEnable( GL_BLEND ); 
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            glShadeModel( GL_SMOOTH );
            glEnable( GL_COLOR_MATERIAL ); 
            sgVec4 color;
            float emis = 0.05;
            if ( 1 ) {
                ssgGetLight( 0 )->getColour( GL_DIFFUSE, color );
                emis = ( color[0]+color[1]+color[2] ) / 3.0;
                if ( emis < 0.05 )
                    emis = 0.05;
            }
            sgSetVec4( color, emis, emis, emis, 0.0 );
            glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, color );
            sgSetVec4( color, 1.0f, 1.0f, 1.0f, 0.0 );
            glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, color );
            sgSetVec4( color, 1.0, 1.0, 1.0, 0.0 );
            glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, color );
            sgSetVec4( color, 0.0, 0.0, 0.0, 0.0 );
            glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, color );

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

            glActiveTexturePtr( GL_TEXTURE0_ARB );
            glBindTexture( GL_TEXTURE_2D, normal->getHandle() );
            glEnable( GL_TEXTURE_2D );

            //Bind normalisation cube map to texture unit 1
            glActiveTexturePtr( GL_TEXTURE1_ARB );
            glBindTexture( GL_TEXTURE_CUBE_MAP_ARB, normalization_cube_map );
            glEnable( GL_TEXTURE_CUBE_MAP_ARB );
            glActiveTexturePtr( GL_TEXTURE0_ARB );

            //Set vertex arrays for cloud
            glVertexPointer( 3, GL_FLOAT, sizeof(CloudVertex), &vertices[0].position );
            glEnableClientState( GL_VERTEX_ARRAY );
/*
            if ( nb_texture_unit >= 3 ) {
                glColorPointer( 4, GL_FLOAT, sizeof(CloudVertex), &vertices[0].color );
                glEnableClientState( GL_COLOR_ARRAY );
            }
*/
            //Send texture coords for normal map to unit 0
            glTexCoordPointer( 2, GL_FLOAT, sizeof(CloudVertex), &vertices[0].texCoord );
            glEnableClientState( GL_TEXTURE_COORD_ARRAY );

            //Send tangent space light vectors for normalisation to unit 1
            glClientActiveTexturePtr( GL_TEXTURE1_ARB );
            glTexCoordPointer( 3, GL_FLOAT, sizeof(CloudVertex), &vertices[0].tangentSpLight );
            glEnableClientState( GL_TEXTURE_COORD_ARRAY );

            //Set up texture environment to do (tex0 dot tex1)*color
            glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
            glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE );
            glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE );
            glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE );
            glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE );

            glActiveTexturePtr( GL_TEXTURE1_ARB );

            glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
            glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE );
            glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGB_ARB );
            glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB );
            glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB );
            glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE );

            if ( nb_texture_unit >= 3 ) {
                glActiveTexturePtr( GL_TEXTURE2_ARB );
                glBindTexture( GL_TEXTURE_2D, decal->getHandle() );

                glClientActiveTexturePtr( GL_TEXTURE2_ARB );
                glTexCoordPointer( 2, GL_FLOAT, sizeof(CloudVertex), &vertices[0].texCoord );
                glEnableClientState( GL_TEXTURE_COORD_ARRAY );

                glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
                glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD );
                glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE );
                glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB );

                glClientActiveTexturePtr( GL_TEXTURE0_ARB );
                glActiveTexturePtr( GL_TEXTURE0_ARB );

                //Draw cloud layer
                glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[0] );
                glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[10] );
                glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[20] );
                glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[30] );

                glDisable( GL_TEXTURE_2D );
                glActiveTexturePtr( GL_TEXTURE1_ARB );
                glDisable( GL_TEXTURE_CUBE_MAP_ARB );
                glActiveTexturePtr( GL_TEXTURE2_ARB );
                glDisable( GL_TEXTURE_2D );
                glActiveTexturePtr( GL_TEXTURE0_ARB );

                glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                glClientActiveTexturePtr( GL_TEXTURE1_ARB );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                glClientActiveTexturePtr( GL_TEXTURE2_ARB );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                glClientActiveTexturePtr( GL_TEXTURE0_ARB );

                glDisableClientState( GL_COLOR_ARRAY );
                glEnable( GL_LIGHTING );

                glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

            } else {
                glClientActiveTexturePtr( GL_TEXTURE0_ARB );
                glActiveTexturePtr( GL_TEXTURE0_ARB );

                //Draw cloud layer
                glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[0] );
                glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[10] );
                glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[20] );
                glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[30] );

                //Disable textures
                glDisable( GL_TEXTURE_2D );

                glActiveTexturePtr( GL_TEXTURE1_ARB );
                glDisable( GL_TEXTURE_CUBE_MAP_ARB );
                glActiveTexturePtr( GL_TEXTURE0_ARB );

                //disable vertex arrays
                glDisableClientState( GL_VERTEX_ARRAY );

                glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                glClientActiveTexturePtr( GL_TEXTURE1_ARB );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                glClientActiveTexturePtr( GL_TEXTURE0_ARB );

                //Return to standard modulate texenv
                glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

                if ( layer_coverage == SG_CLOUD_OVERCAST ) {
	            glDepthFunc(GL_LEQUAL);

                    glEnable( GL_LIGHTING );
                    sgVec4 color;
                    ssgGetLight( 0 )->getColour( GL_DIFFUSE, color );
                    float average = ( color[0] + color[1] + color[2] ) / 3.0f;
                    average = 0.15 + average/10;
                    sgVec4 averageColor;
                    sgSetVec4( averageColor, average, average, average, 1.0f );
                    ssgGetLight( 0 )->setColour( GL_DIFFUSE, averageColor );

                    glBlendColorPtr( average, average, average, 1.0f );
                    glBlendFunc( GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_COLOR );

                    //Perform a second pass to color the torus
                    //Bind decal texture
                    glBindTexture( GL_TEXTURE_2D, decal->getHandle() );
                    glEnable(GL_TEXTURE_2D);

                    //Set vertex arrays for torus
                    glVertexPointer( 3, GL_FLOAT, sizeof(CloudVertex), &vertices[0].position );
                    glEnableClientState( GL_VERTEX_ARRAY );

                    //glColorPointer( 4, GL_FLOAT, sizeof(CloudVertex), &vertices[0].color );
                    //glEnableClientState( GL_COLOR_ARRAY );

                    glNormalPointer( GL_FLOAT, sizeof(CloudVertex), &vertices[0].normal );
                    glEnableClientState( GL_NORMAL_ARRAY );

                    glTexCoordPointer( 2, GL_FLOAT, sizeof(CloudVertex), &vertices[0].texCoord );
                    glEnableClientState( GL_TEXTURE_COORD_ARRAY );

                    //Draw cloud layer
                    glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[0] );
                    glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[10] );
                    glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[20] );
                    glDrawElements( GL_TRIANGLE_STRIP, 10, GL_UNSIGNED_INT, &indices[30] );

                    ssgGetLight( 0 )->setColour( GL_DIFFUSE, color );

                    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
                }
            }
            //Disable texture
            glDisable( GL_TEXTURE_2D );

            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );

            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glEnable( GL_CULL_FACE );
	    glDepthFunc(GL_LESS);

            ssgLoadModelviewMatrix( modelview );

        } else {
            state_sel->selectStep( top ? 1 : 0 );
            ssgCullAndDraw( layer_root );
        }
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

    return state;
}
