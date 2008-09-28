// cloud.cxx -- model a single cloud layer
//
// Written by Curtis Olson, started June 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <sstream>

#include <math.h>

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/ShadeModel>
#include <osg/TexEnv>
#include <osg/TexEnvCombine>
#include <osg/Texture2D>
#include <osg/TextureCubeMap>
#include <osg/TexMat>

#include <simgear/math/sg_random.h>
#include <simgear/misc/PathOptions.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/RenderConstants.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/math/polar3d.hxx>

#include "newcloud.hxx"
#include "cloudfield.hxx"
#include "cloud.hxx"

using namespace simgear;
#if defined(__MINGW32__)
#define isnan(x) _isnan(x)
#endif

// #if defined (__FreeBSD__)
// #  if __FreeBSD_version < 500000
//      extern "C" {
//        inline int isnan(double r) { return !(r <= 0 || r >= 0); }
//      }
// #  endif
// #endif

#if defined (__CYGWIN__)
#include <ieeefp.h>
#endif

static osg::ref_ptr<osg::StateSet> layer_states[SGCloudLayer::SG_MAX_CLOUD_COVERAGES];
static osg::ref_ptr<osg::StateSet> layer_states2[SGCloudLayer::SG_MAX_CLOUD_COVERAGES];
static osg::ref_ptr<osg::TextureCubeMap> cubeMap;
static bool state_initialized = false;
static bool bump_mapping = false;

bool SGCloudLayer::enable_bump_mapping = false;

// make an StateSet for a cloud layer given the named texture
static osg::StateSet*
SGMakeState(const SGPath &path, const char* colorTexture,
            const char* normalTexture)
{
    osg::StateSet *stateSet = new osg::StateSet;

    osg::ref_ptr<osgDB::ReaderWriter::Options> options
        = makeOptionsFromPath(path);
    stateSet->setTextureAttribute(0, SGLoadTexture2D(colorTexture,
                                                     options.get()));
    stateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    StateAttributeFactory* attribFactory = StateAttributeFactory::instance();
    stateSet->setAttributeAndModes(attribFactory->getSmoothShadeModel());
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    stateSet->setAttributeAndModes(attribFactory->getStandardAlphaFunc());
    stateSet->setAttributeAndModes(attribFactory->getStandardBlendFunc());

//     osg::Material* material = new osg::Material;
//     material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
//     material->setEmission(osg::Material::FRONT_AND_BACK,
//                           osg::Vec4(0.05, 0.05, 0.05, 0));
//     material->setSpecular(osg::Material::FRONT_AND_BACK,
//                           osg::Vec4(0, 0, 0, 1));
//     stateSet->setAttribute(material);

    stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);

    // OSGFIXME: invented by me ...
//     stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
//     stateSet->setMode(GL_LIGHTING, osg::StateAttribute::ON);

//     stateSet->setMode(GL_LIGHT0, osg::StateAttribute::OFF);

    // If the normal texture is given prepare a bumpmapping enabled state
//     if (normalTexture) {
//       SGPath normalPath(path);
//       normalPath.append(normalTexture);
//       stateSet->setTextureAttribute(2, SGLoadTexture2D(normalPath));
//       stateSet->setTextureMode(2, GL_TEXTURE_2D, osg::StateAttribute::ON);
//     }

    return stateSet;
}

// Constructor
SGCloudLayer::SGCloudLayer( const string &tex_path ) :
    layer_root(new osg::Switch),
    group_top(new osg::Group),
    group_bottom(new osg::Group),
    layer_transform(new osg::MatrixTransform),
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
    // XXX
    // Render bottoms before the rest of transparent objects (rendered
    // in bin 10), tops after. The negative numbers on the bottoms
    // RenderBins and the positive numbers on the tops enforce this
    // order.
  layer_root->addChild(group_bottom.get());
  layer_root->addChild(group_top.get());
  osg::StateSet *rootSet = layer_root->getOrCreateStateSet();
  rootSet->setRenderBinDetails(CLOUDS_BIN, "DepthSortedBin");
  rootSet->setTextureAttribute(0, new osg::TexMat);
  // Combiner for fog color and cloud alpha
  osg::TexEnvCombine* combine0 = new osg::TexEnvCombine;
  osg::TexEnvCombine* combine1 = new osg::TexEnvCombine;
  combine0->setCombine_RGB(osg::TexEnvCombine::MODULATE);
  combine0->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
  combine0->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
  combine0->setSource1_RGB(osg::TexEnvCombine::TEXTURE0);
  combine0->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
  combine0->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
  combine0->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
  combine0->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
  combine0->setSource1_Alpha(osg::TexEnvCombine::TEXTURE0);
  combine0->setOperand1_Alpha(osg::TexEnvCombine::SRC_ALPHA);

  combine1->setCombine_RGB(osg::TexEnvCombine::MODULATE);
  combine1->setSource0_RGB(osg::TexEnvCombine::PREVIOUS);
  combine1->setOperand0_RGB(osg::TexEnvCombine::SRC_COLOR);
  combine1->setSource1_RGB(osg::TexEnvCombine::CONSTANT);
  combine1->setOperand1_RGB(osg::TexEnvCombine::SRC_COLOR);
  combine1->setCombine_Alpha(osg::TexEnvCombine::MODULATE);
  combine1->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
  combine1->setOperand0_Alpha(osg::TexEnvCombine::SRC_ALPHA);
  combine1->setSource1_Alpha(osg::TexEnvCombine::CONSTANT);
  combine1->setOperand1_Alpha(osg::TexEnvCombine::SRC_ALPHA);
  combine1->setDataVariance(osg::Object::DYNAMIC);
  rootSet->setTextureAttributeAndModes(0, combine0);
  rootSet->setTextureAttributeAndModes(1, combine1);
  rootSet->setTextureMode(1, GL_TEXTURE_2D, osg::StateAttribute::ON);
  rootSet->setTextureAttributeAndModes(1, StateAttributeFactory::instance()
                                       ->getWhiteTexture(),
                                       osg::StateAttribute::ON);
  rootSet->setDataVariance(osg::Object::DYNAMIC);

  base = osg::Vec2(sg_random(), sg_random());

  group_top->addChild(layer_transform.get());
  group_bottom->addChild(layer_transform.get());

  layer3D = new SGCloudField;
  rebuild();
}

// Destructor
SGCloudLayer::~SGCloudLayer()
{
  delete layer3D;
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

void
SGCloudLayer::setTextureOffset(const osg::Vec2& offset)
{
    osg::StateAttribute* attr = layer_root->getStateSet()
        ->getTextureAttribute(0, osg::StateAttribute::TEXMAT);
    osg::TexMat* texMat = dynamic_cast<osg::TexMat*>(attr);
    if (!texMat)
        return;
    texMat->setMatrix(osg::Matrix::translate(offset[0], offset[1], 0.0));
}

// build the cloud object
void
SGCloudLayer::rebuild()
{
    // Initialize states and sizes if necessary.
    if ( !state_initialized ) { 
        state_initialized = true;

        SG_LOG(SG_ASTRO, SG_INFO, "initializing cloud layers");

        osg::Texture::Extensions* extensions;
        extensions = osg::Texture::getExtensions(0, true);
        // OSGFIXME
        bump_mapping = extensions->isMultiTexturingSupported() &&
          (2 <= extensions->numTextureUnits()) &&
          SGIsOpenGLExtensionSupported("GL_ARB_texture_env_combine") &&
          SGIsOpenGLExtensionSupported("GL_ARB_texture_env_dot3");

        osg::TextureCubeMap::Extensions* extensions2;
        extensions2 = osg::TextureCubeMap::getExtensions(0, true);
        bump_mapping = bump_mapping && extensions2->isCubeMapSupported();

        // This bump mapping code was inspired by the tutorial available at 
        // http://www.paulsprojects.net/tutorials/simplebump/simplebump.html
        // and a NVidia white paper 
        //  http://developer.nvidia.com/object/bumpmappingwithregistercombiners.html
        // The normal map textures were generated by the normal map Gimp plugin :
        //  http://nifelheim.dyndns.org/~cocidius/normalmap/
        //
        cubeMap = new osg::TextureCubeMap;
        cubeMap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        cubeMap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        cubeMap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        cubeMap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        cubeMap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);

        const int size = 32;
        const float half_size = 16.0f;
        const float offset = 0.5f;
        osg::Vec3 zero_normal(0.5, 0.5, 0.5);

        osg::Image* image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        unsigned char *ptr = image->data(0, 0);
        for (int j = 0; j < size; j++ ) {
          for (int i = 0; i < size; i++ ) {
            osg::Vec3 tmp(half_size, -( j + offset - half_size ),
                          -( i + offset - half_size ) );
            tmp.normalize();
            tmp = tmp*0.5 - zero_normal;
            
            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
          }
        }
        cubeMap->setImage(osg::TextureCubeMap::POSITIVE_X, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);
        for (int j = 0; j < size; j++ ) {
          for (int i = 0; i < size; i++ ) {
            osg::Vec3 tmp(-half_size, -( j + offset - half_size ),
                          ( i + offset - half_size ) );
            tmp.normalize();
            tmp = tmp*0.5 - zero_normal;
            
            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
          }
        }
        cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_X, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);
        for (int j = 0; j < size; j++ ) {
          for (int i = 0; i < size; i++ ) {
            osg::Vec3 tmp(( i + offset - half_size ), half_size,
                          ( j + offset - half_size ) );
            tmp.normalize();
            tmp = tmp*0.5 - zero_normal;
            
            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
          }
        }
        cubeMap->setImage(osg::TextureCubeMap::POSITIVE_Y, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);
        for (int j = 0; j < size; j++ ) {
          for (int i = 0; i < size; i++ ) {
            osg::Vec3 tmp(( i + offset - half_size ), -half_size,
                          -( j + offset - half_size ) );
            tmp.normalize();
            tmp = tmp*0.5 - zero_normal;

            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
          }
        }
        cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_Y, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);
        for (int j = 0; j < size; j++ ) {
          for (int i = 0; i < size; i++ ) {
            osg::Vec3 tmp(( i + offset - half_size ),
                          -( j + offset - half_size ), half_size );
            tmp.normalize();
            tmp = tmp*0.5 - zero_normal;
            
            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
          }
        }
        cubeMap->setImage(osg::TextureCubeMap::POSITIVE_Z, image);

        image = new osg::Image;
        image->allocateImage(size, size, 1, GL_RGB, GL_UNSIGNED_BYTE);
        ptr = image->data(0, 0);
        for (int j = 0; j < size; j++ ) {
          for (int i = 0; i < size; i++ ) {
            osg::Vec3 tmp(-( i + offset - half_size ),
                          -( j + offset - half_size ), -half_size );
            tmp.normalize();
            tmp = tmp*0.5 - zero_normal;
            *ptr++ = (unsigned char)( tmp[ 0 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 1 ] * 255 );
            *ptr++ = (unsigned char)( tmp[ 2 ] * 255 );
          }
        }
        cubeMap->setImage(osg::TextureCubeMap::NEGATIVE_Z, image);

        osg::StateSet* state;
        state = SGMakeState(texture_path, "overcast.rgb", "overcast_n.rgb");
        layer_states[SG_CLOUD_OVERCAST] = state;
        state = SGMakeState(texture_path, "overcast_top.rgb", "overcast_top_n.rgb");
        layer_states2[SG_CLOUD_OVERCAST] = state;
        
        state = SGMakeState(texture_path, "broken.rgba", "broken_n.rgb");
        layer_states[SG_CLOUD_BROKEN] = state;
        layer_states2[SG_CLOUD_BROKEN] = state;
        
        state = SGMakeState(texture_path, "scattered.rgba", "scattered_n.rgb");
        layer_states[SG_CLOUD_SCATTERED] = state;
        layer_states2[SG_CLOUD_SCATTERED] = state;
        
        state = SGMakeState(texture_path, "few.rgba", "few_n.rgb");
        layer_states[SG_CLOUD_FEW] = state;
        layer_states2[SG_CLOUD_FEW] = state;
        
        state = SGMakeState(texture_path, "cirrus.rgba", "cirrus_n.rgb");
        layer_states[SG_CLOUD_CIRRUS] = state;
        layer_states2[SG_CLOUD_CIRRUS] = state;
        
        layer_states[SG_CLOUD_CLEAR] = 0;
        layer_states2[SG_CLOUD_CLEAR] = 0;

      // OSGFIXME
// 		SGNewCloud::loadTextures(texture_path.str());
// 		layer3D->buildTestLayer();
    }

    scale = 4000.0;
    last_lon = last_lat = -999.0f;

    setTextureOffset(base);
    // build the cloud layer
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
    
    for (int i = 0; i < 4; i++) {
      if ( layer[i] != NULL ) {
        layer_transform->removeChild(layer[i].get()); // automatic delete
      }
      
      vl[i] = new osg::Vec3Array;
      cl[i] = new osg::Vec4Array;
      tl[i] = new osg::Vec2Array;
      
      
      osg::Vec3 vertex(layer_span*(i-2)/2, -layer_span,
                       alt_diff * (sin(i*mpi) - 2));
      osg::Vec2 tc(layer_scale * i/4, 0.0f);
      osg::Vec4 color(1.0f, 1.0f, 1.0f, (i == 0) ? 0.0f : 0.15f);
      
      cl[i]->push_back(color);
      vl[i]->push_back(vertex);
      tl[i]->push_back(tc);
      
      for (int j = 0; j < 4; j++) {
        vertex = osg::Vec3(layer_span*(i-1)/2, layer_span*(j-2)/2,
                           alt_diff * (sin((i+1)*mpi) + sin(j*mpi) - 2));
        tc = osg::Vec2(layer_scale * (i+1)/4, layer_scale * j/4);
        color = osg::Vec4(1.0f, 1.0f, 1.0f,
                          ( (j == 0) || (i == 3)) ?  
                          ( (j == 0) && (i == 3)) ? 0.0f : 0.15f : 1.0f );
        
        cl[i]->push_back(color);
        vl[i]->push_back(vertex);
        tl[i]->push_back(tc);
        
        vertex = osg::Vec3(layer_span*(i-2)/2, layer_span*(j-1)/2,
                           alt_diff * (sin(i*mpi) + sin((j+1)*mpi) - 2) );
        tc = osg::Vec2(layer_scale * i/4, layer_scale * (j+1)/4 );
        color = osg::Vec4(1.0f, 1.0f, 1.0f,
                          ((j == 3) || (i == 0)) ?
                          ((j == 3) && (i == 0)) ? 0.0f : 0.15f : 1.0f );
        cl[i]->push_back(color);
        vl[i]->push_back(vertex);
        tl[i]->push_back(tc);
      }
      
      vertex = osg::Vec3(layer_span*(i-1)/2, layer_span, 
                         alt_diff * (sin((i+1)*mpi) - 2));
      
      tc = osg::Vec2(layer_scale * (i+1)/4, layer_scale);
      
      color = osg::Vec4(1.0f, 1.0f, 1.0f, (i == 3) ? 0.0f : 0.15f );
      
      cl[i]->push_back( color );
      vl[i]->push_back( vertex );
      tl[i]->push_back( tc );
      
      osg::Geometry* geometry = new osg::Geometry;
      geometry->setUseDisplayList(false);
      geometry->setVertexArray(vl[i].get());
      geometry->setNormalBinding(osg::Geometry::BIND_OFF);
      geometry->setColorArray(cl[i].get());
      geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
      geometry->setTexCoordArray(0, tl[i].get());
      geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLE_STRIP, 0, vl[i]->size()));
      layer[i] = new osg::Geode;
      
      std::stringstream sstr;
      sstr << "Cloud Layer (" << i << ")";
      geometry->setName(sstr.str());
      layer[i]->setName(sstr.str());
      layer[i]->addDrawable(geometry);
      layer_transform->addChild(layer[i].get());
    }
    
    //OSGFIXME: true
    if ( layer_states[layer_coverage].valid() ) {
      osg::CopyOp copyOp;    // shallow copy
      // render bin will be set in reposition
      osg::StateSet* stateSet = static_cast<osg::StateSet*>(layer_states2[layer_coverage]->clone(copyOp));
      stateSet->setDataVariance(osg::Object::DYNAMIC);
      group_top->setStateSet(stateSet);
      stateSet = static_cast<osg::StateSet*>(layer_states2[layer_coverage]->clone(copyOp));
      stateSet->setDataVariance(osg::Object::DYNAMIC);
      group_bottom->setStateSet(stateSet);
    }
}

#if 0
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

// use TexEnvCombine to add the highlights to the original lighting
osg::TexEnvCombine *te = new osg::TexEnvCombine;    
te->setSource0_RGB(osg::TexEnvCombine::TEXTURE);
te->setCombine_RGB(osg::TexEnvCombine::REPLACE);
te->setSource0_Alpha(osg::TexEnvCombine::TEXTURE);
te->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
ss->setTextureAttributeAndModes(0, te, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);


            glActiveTexturePtr( GL_TEXTURE1_ARB );

            glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );
            glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE );
            glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGB_ARB );
            glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB );
            glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB );
            glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE );

osg::TexEnvCombine *te = new osg::TexEnvCombine;    
te->setSource0_RGB(osg::TexEnvCombine::TEXTURE);
te->setCombine_RGB(osg::TexEnvCombine::DOT3_RGB);
te->setSource1_RGB(osg::TexEnvCombine::PREVIOUS);
te->setSource0_Alpha(osg::TexEnvCombine::PREVIOUS);
te->setCombine_Alpha(osg::TexEnvCombine::REPLACE);
ss->setTextureAttributeAndModes(0, te, osg::StateAttribute::OVERRIDE | osg::StateAttribute::ON);


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
#endif

// repaint the cloud layer colors
bool SGCloudLayer::repaint( const SGVec3f& fog_color ) {
    osg::Vec4f combineColor(fog_color.osg(), cloud_alpha);
    osg::TexEnvCombine* combiner
        = dynamic_cast<osg::TexEnvCombine*>(layer_root->getStateSet()
                                            ->getTextureAttribute(1, osg::StateAttribute::TEXENV));
    combiner->setConstantColor(combineColor);
    return true;
}

// reposition the cloud layer at the specified origin and orientation
// lon specifies a rotation about the Z axis
// lat specifies a rotation about the new Y axis
// spin specifies a rotation about the new Z axis (and orients the
// sunrise/set effects
bool SGCloudLayer::reposition( const SGVec3f& p, const SGVec3f& up, double lon, double lat,
        		       double alt, double dt )
{
    // combine p and asl (meters) to get translation offset
    osg::Vec3 asl_offset(up.osg());
    asl_offset.normalize();
    if ( alt <= layer_asl ) {
        asl_offset *= layer_asl;
    } else {
        asl_offset *= layer_asl + layer_thickness;
    }

    // cout << "asl_offset = " << asl_offset[0] << "," << asl_offset[1]
    //      << "," << asl_offset[2] << endl;
    asl_offset += p.osg();
    // cout << "  asl_offset = " << asl_offset[0] << "," << asl_offset[1]
    //      << "," << asl_offset[2] << endl;

    osg::Matrix T, LON, LAT;
    // Translate to zero elevation
    // Point3D zero_elev = current_view.get_cur_zero_elev();
    T.makeTranslate( asl_offset );

    // printf("  Translated to %.2f %.2f %.2f\n", 
    //        zero_elev.x, zero_elev.y, zero_elev.z );

    // Rotate to proper orientation
    // printf("  lon = %.2f  lat = %.2f\n", 
    //        lon * SGD_RADIANS_TO_DEGREES,
    //        lat * SGD_RADIANS_TO_DEGREES);
    LON.makeRotate(lon, osg::Vec3(0, 0, 1));

    // xglRotatef( 90.0 - f->get_Latitude() * SGD_RADIANS_TO_DEGREES,
    //             0.0, 1.0, 0.0 );
    LAT.makeRotate(90.0 * SGD_DEGREES_TO_RADIANS - lat, osg::Vec3(0, 1, 0));

    layer_transform->setMatrix( LAT*LON*T );
    // The layers need to be drawn in order because they are
    // translucent, but OSG transparency sorting doesn't work because
    // the cloud polys are huge. However, the ordering is simple: the
    // bottom polys should be drawn from high altitude to low, and the
    // top polygons from low to high. The altitude can be used
    // directly to order the polygons!
    group_bottom->getStateSet()->setRenderBinDetails(-(int)layer_asl,
                                                     "RenderBin");
    group_top->getStateSet()->setRenderBinDetails((int)layer_asl,
                                                  "RenderBin");
    if ( alt <= layer_asl ) {
      layer_root->setSingleChildOn(0);
    } else if ( alt >= layer_asl + layer_thickness ) {
      layer_root->setSingleChildOn(1);
    } else {
      layer_root->setAllChildrenOff();
    }
        

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
            SG_LOG(SG_ASTRO, SG_DEBUG,
                    "Error: base = " << base[0] << "," << base[1] <<
                    " course = " << course << " dist = " << dist );
            base[1] = 0.0;
        }

        // cout << "base = " << base[0] << "," << base[1] << endl;

        setTextureOffset(base);
        last_lon = lon;
        last_lat = lat;
    }

// 	layer3D->reposition( p, up, lon, lat, alt, dt, direction, speed);
    return true;
}
