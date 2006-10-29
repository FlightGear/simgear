// materialmgr.cxx -- class to handle material properties
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
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

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/structure/exception.hxx>

#include SG_GL_H

#include <string.h>
#include STL_STRING

#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/CullFace>
#include <osg/Material>
// #include <osg/Multisample>
#include <osg/Point>
#include <osg/PointSprite>
#include <osg/PolygonMode>
#include <osg/PolygonOffset>
#include <osg/StateSet>
#include <osg/TexEnv>
#include <osg/TexGen>
#include <osg/Texture2D>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/scene/tgdb/userdata.hxx>

#include "mat.hxx"

#include "matlib.hxx"

SG_USING_NAMESPACE(std);
SG_USING_STD(string);

extern bool SGPointLightsUseSprites;
extern bool SGPointLightsEnhancedLighting;
extern bool SGPointLightsDistanceAttenuation;

// FIXME: should make this configurable
static const bool sprite_lighting = true;

// Constructor
SGMaterialLib::SGMaterialLib ( void ) {
}

// generate standard colored directional light environment texture map
static osg::Texture2D*
gen_standard_dir_light_map( int r, int g, int b, int alpha ) {
    const int env_tex_res = 32;
    int half_res = env_tex_res / 2;

    osg::Image* image = new osg::Image;
    image->allocateImage(env_tex_res, env_tex_res, 1,
                         GL_RGBA, GL_UNSIGNED_BYTE);
    for ( int i = 0; i < env_tex_res; ++i ) {
        for ( int j = 0; j < env_tex_res; ++j ) {
            double x = (i - half_res) / (double)half_res;
            double y = (j - half_res) / (double)half_res;
            double dist = sqrt(x*x + y*y);
            if ( dist > 1.0 ) { dist = 1.0; }
            double bright = cos( dist * SGD_PI_2 );
            if ( bright < 0.3 ) { bright = 0.3; }
            unsigned char* env_map = image->data(i, j);
            env_map[0] = r;
            env_map[1] = g;
            env_map[2] = b;
            env_map[3] = (int)(bright * alpha);
        }
    }

    osg::Texture2D* texture = new osg::Texture2D;
    texture->setImage(image);

    return texture;
}


// generate standard colored directional light environment texture map
static osg::Texture2D*
gen_taxiway_dir_light_map( int r, int g, int b, int alpha ) {
    const int env_tex_res = 32;
    int half_res = env_tex_res / 2;

    osg::Image* image = new osg::Image;
    image->allocateImage(env_tex_res, env_tex_res, 1,
                         GL_RGBA, GL_UNSIGNED_BYTE);

    for ( int i = 0; i < env_tex_res; ++i ) {
        for ( int j = 0; j < env_tex_res; ++j ) {
            double x = (i - half_res) / (double)half_res;
            double y = (j - half_res) / (double)half_res;
            double tmp = sqrt(x*x + y*y);
            double dist = tmp * tmp;
            if ( dist > 1.0 ) { dist = 1.0; }
            double bright = sin( dist * SGD_PI_2 );
            if ( bright < 0.2 ) { bright = 0.2; }
            unsigned char* env_map = image->data(i, j);
            env_map[0] = r;
            env_map[1] = g;
            env_map[2] = b;
            env_map[3] = (int)(bright * alpha);
        }
    }

    osg::Texture2D* texture = new osg::Texture2D;
    texture->setImage(image);

    return texture;
}

static osg::Texture2D*
gen_standard_light_sprite( int r, int g, int b, int alpha ) {
    const int env_tex_res = 32;
    int half_res = env_tex_res / 2;

    osg::Image* image = new osg::Image;
    image->allocateImage(env_tex_res, env_tex_res, 1,
                         GL_RGBA, GL_UNSIGNED_BYTE);

    for ( int i = 0; i < env_tex_res; ++i ) {
        for ( int j = 0; j < env_tex_res; ++j ) {
            double x = (i - half_res) / (double)half_res;
            double y = (j - half_res) / (double)half_res;
            double dist = sqrt(x*x + y*y);
            if ( dist > 1.0 ) { dist = 1.0; }
            double bright = cos( dist * SGD_PI_2 );
            if ( bright < 0.01 ) { bright = 0.0; }
            unsigned char* env_map = image->data(i, j);
            env_map[0] = r;
            env_map[1] = g;
            env_map[2] = b;
            env_map[3] = (int)(bright * alpha);
        }
    }

    osg::Texture2D* texture = new osg::Texture2D;
    texture->setImage(image);

    return texture;
}


// Load a library of material properties
bool SGMaterialLib::load( const string &fg_root, const string& mpath, const char *season ) {

    SGPropertyNode materials;

    SG_LOG( SG_INPUT, SG_INFO, "Reading materials from " << mpath );
    try {
        readProperties( mpath, &materials );
    } catch (const sg_exception &ex) {
        SG_LOG( SG_INPUT, SG_ALERT, "Error reading materials: "
                << ex.getMessage() );
        throw;
    }

    SGSharedPtr<SGMaterial> m;

    int nMaterials = materials.nChildren();
    for (int i = 0; i < nMaterials; i++) {
        const SGPropertyNode * node = materials.getChild(i);
        if (!strcmp(node->getName(), "material")) {
            m = new SGMaterial( fg_root, node, season );

            vector<SGPropertyNode_ptr>names = node->getChildren("name");
            for ( unsigned int j = 0; j < names.size(); j++ ) {
                string name = names[j]->getStringValue();
                // cerr << "Material " << name << endl;
                matlib[name] = m;
                m->add_name(name);
                SG_LOG( SG_TERRAIN, SG_INFO, "  Loading material "
                        << names[j]->getStringValue() );
            }
        } else {
            SG_LOG(SG_INPUT, SG_WARN,
                   "Skipping bad material entry " << node->getName());
        }
    }

    osg::ref_ptr<osg::StateSet> lightStateSet = new osg::StateSet;
    {
//       lightStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
//       lightStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
      lightStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
//       lightStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

      lightStateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
//       lightStateSet->setAttribute(new osg::AlphaFunc(osg::AlphaFunc::GREATER, 0));
//       lightStateSet->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);

      osg::CullFace* cullFace = new osg::CullFace;
      cullFace->setMode(osg::CullFace::BACK);
      lightStateSet->setAttributeAndModes(cullFace, osg::StateAttribute::ON);

      osg::BlendFunc* blendFunc = new osg::BlendFunc;
      blendFunc->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
      lightStateSet->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);

      osg::PolygonMode* polygonMode = new osg::PolygonMode;
      polygonMode->setMode(osg::PolygonMode::FRONT, osg::PolygonMode::POINT);
      lightStateSet->setAttribute(polygonMode);

//       if (SGPointLightsUseSprites) {
        osg::PointSprite* pointSprite = new osg::PointSprite;
        lightStateSet->setTextureAttributeAndModes(0, pointSprite, osg::StateAttribute::ON);
//       }

//       if (SGPointLightsDistanceAttenuation) {
        osg::Point* point = new osg::Point;
        point->setMinSize(2);
        point->setSize(8);
        point->setDistanceAttenuation(osg::Vec3(1.0, 0.001, 0.000001));
        lightStateSet->setAttribute(point);
//       }

      osg::PolygonOffset* polygonOffset = new osg::PolygonOffset;
      polygonOffset->setFactor(-1);
      polygonOffset->setUnits(-1);
      lightStateSet->setAttributeAndModes(polygonOffset, osg::StateAttribute::ON);
      
      osg::TexGen* texGen = new osg::TexGen;
      texGen->setMode(osg::TexGen::SPHERE_MAP);
      lightStateSet->setTextureAttribute(0, texGen);
      lightStateSet->setTextureMode(0, GL_TEXTURE_GEN_S, osg::StateAttribute::ON);
      lightStateSet->setTextureMode(0, GL_TEXTURE_GEN_T, osg::StateAttribute::ON);
      osg::TexEnv* texEnv = new osg::TexEnv;
      texEnv->setMode(osg::TexEnv::MODULATE);
      lightStateSet->setTextureAttribute(0, texEnv);

      osg::Material* material = new osg::Material;
      lightStateSet->setAttribute(material);
//       lightStateSet->setMode(GL_COLOR_MATERIAL, osg::StateAttribute::OFF);
    }
    

    // hard coded ground light state
    osg::StateSet *gnd_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
//     if (SGPointLightsDistanceAttenuation) {
    osg::Point* point = new osg::Point;
      point->setMinSize(1);
      point->setSize(2);
      point->setMaxSize(4);
      point->setDistanceAttenuation(osg::Vec3(1.0, 0.01, 0.0001));
      while (gnd_lights->getAttribute(osg::StateAttribute::POINT)) {
        gnd_lights->removeAttribute(osg::StateAttribute::POINT);
      }
      gnd_lights->setAttribute(point);
//     }
    m = new SGMaterial( gnd_lights );
    m->add_name("GROUND_LIGHTS");
    matlib["GROUND_LIGHTS"] = m;

    // hard coded runway white light state
    osg::Texture2D* texture;
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite(235, 235, 195, 255);
    } else {
      texture = gen_standard_dir_light_map(235, 235, 195, 255);
    }
    osg::StateSet *rwy_white_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_white_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

    m = new SGMaterial( rwy_white_lights );
    m->add_name("RWY_WHITE_LIGHTS");
    matlib["RWY_WHITE_LIGHTS"] = m;
    // For backwards compatibility ... remove someday
    m->add_name("RUNWAY_LIGHTS");
    matlib["RUNWAY_LIGHTS"] = m;
    m->add_name("RWY_LIGHTS");
    matlib["RWY_LIGHTS"] = m;
    // end of backwards compatitibilty

    // hard coded runway medium intensity white light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 235, 235, 195, 205 );
    } else {
      texture = gen_standard_dir_light_map( 235, 235, 195, 205 );
    }
    osg::StateSet *rwy_white_medium_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_white_medium_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);

    m = new SGMaterial( rwy_white_medium_lights );
    m->add_name("RWY_WHITE_MEDIUM_LIGHTS");
    matlib["RWY_WHITE_MEDIUM_LIGHTS"] = m;

    // hard coded runway low intensity white light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 235, 235, 195, 155 );
    } else {
      texture = gen_standard_dir_light_map( 235, 235, 195, 155 );
    }
    osg::StateSet *rwy_white_low_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_white_medium_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_white_low_lights );
    m->add_name("RWY_WHITE_LOW_LIGHTS");
    matlib["RWY_WHITE_LOW_LIGHTS"] = m;

    // hard coded runway yellow light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 235, 215, 20, 255 );
    } else {
      texture = gen_standard_dir_light_map( 235, 215, 20, 255 );
    }
    osg::StateSet *rwy_yellow_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_yellow_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_yellow_lights );
    m->add_name("RWY_YELLOW_LIGHTS");
    matlib["RWY_YELLOW_LIGHTS"] = m;

    // hard coded runway medium intensity yellow light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 235, 215, 20, 205 );
    } else {
      texture = gen_standard_dir_light_map( 235, 215, 20, 205 );
    }
    osg::StateSet *rwy_yellow_medium_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_yellow_medium_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_yellow_medium_lights );
    m->add_name("RWY_YELLOW_MEDIUM_LIGHTS");
    matlib["RWY_YELLOW_MEDIUM_LIGHTS"] = m;

    // hard coded runway low intensity yellow light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 235, 215, 20, 155 );
    } else {
      texture = gen_standard_dir_light_map( 235, 215, 20, 155 );
    }
    osg::StateSet *rwy_yellow_low_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_yellow_low_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_yellow_low_lights );
    m->add_name("RWY_YELLOW_LOW_LIGHTS");
    matlib["RWY_YELLOW_LOW_LIGHTS"] = m;

    // hard coded runway red light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 235, 90, 90, 255 );
    } else {
      texture = gen_standard_dir_light_map( 235, 90, 90, 255 );
    }
    osg::StateSet *rwy_red_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_red_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_red_lights );
    m->add_name("RWY_RED_LIGHTS");
    matlib["RWY_RED_LIGHTS"] = m;

    // hard coded medium intensity runway red light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 235, 90, 90, 205 );
    } else {
      texture = gen_standard_dir_light_map( 235, 90, 90, 205 );
    }
    osg::StateSet *rwy_red_medium_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_red_medium_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_red_medium_lights );
    m->add_name("RWY_RED_MEDIUM_LIGHTS");
    matlib["RWY_RED_MEDIUM_LIGHTS"] = m;

    // hard coded low intensity runway red light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 235, 90, 90, 155 );
    } else {
      texture = gen_standard_dir_light_map( 235, 90, 90, 155 );
    }
    osg::StateSet *rwy_red_low_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_red_low_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_red_low_lights );
    m->add_name("RWY_RED_LOW_LIGHTS");
    matlib["RWY_RED_LOW_LIGHTS"] = m;

    // hard coded runway green light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 20, 235, 20, 255 );
    } else {
      texture = gen_standard_dir_light_map( 20, 235, 20, 255 );
    }
    osg::StateSet *rwy_green_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_green_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_green_lights );
    m->add_name("RWY_GREEN_LIGHTS");
    matlib["RWY_GREEN_LIGHTS"] = m;

    // hard coded medium intensity runway green light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 20, 235, 20, 205 );
    } else {
      texture = gen_standard_dir_light_map( 20, 235, 20, 205 );
    }
    osg::StateSet *rwy_green_medium_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_green_medium_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_green_medium_lights );
    m->add_name("RWY_GREEN_MEDIUM_LIGHTS");
    matlib["RWY_GREEN_MEDIUM_LIGHTS"] = m;

    // hard coded low intensity runway green light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 20, 235, 20, 155 );
    } else {
      texture = gen_standard_dir_light_map( 20, 235, 20, 155 );
    }
    osg::StateSet *rwy_green_low_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    rwy_green_low_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_green_low_lights );
    m->add_name("RWY_GREEN_LOW_LIGHTS");
    matlib["RWY_GREEN_LOW_LIGHTS"] = m;
    m->add_name("RWY_GREEN_TAXIWAY_LIGHTS");
    matlib["RWY_GREEN_TAXIWAY_LIGHTS"] = m;

    // hard coded low intensity taxiway blue light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 90, 90, 235, 205 );
    } else {
      texture = gen_taxiway_dir_light_map( 90, 90, 235, 205 );
    }
    osg::StateSet *taxiway_blue_low_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
    taxiway_blue_low_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( taxiway_blue_low_lights );
    m->add_name("RWY_BLUE_TAXIWAY_LIGHTS");
    matlib["RWY_BLUE_TAXIWAY_LIGHTS"] = m;

    // hard coded runway vasi light state
    if ( sprite_lighting ) {
      texture = gen_standard_light_sprite( 235, 235, 195, 255 );
    } else {
      texture = gen_standard_dir_light_map( 235, 235, 195, 255 );
    }
    osg::StateSet *rwy_vasi_lights = static_cast<osg::StateSet*>(lightStateSet->clone(osg::CopyOp::DEEP_COPY_ALL));
//     if (SGPointLightsDistanceAttenuation) {
      point = new osg::Point;
      point->setMinSize(4);
      point->setSize(10);
      point->setDistanceAttenuation(osg::Vec3(1.0, 0.01, 0.0001));
      while (rwy_vasi_lights->getAttribute(osg::StateAttribute::POINT)) {
        rwy_vasi_lights->removeAttribute(osg::StateAttribute::POINT);
      }
      rwy_vasi_lights->setAttribute(point);
//     }
    rwy_vasi_lights->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
    m = new SGMaterial( rwy_vasi_lights );
    m->add_name("RWY_VASI_LIGHTS");
    matlib["RWY_VASI_LIGHTS"] = m;

    return true;
}


// Load a library of material properties
bool SGMaterialLib::add_item ( const string &tex_path )
{
    string material_name = tex_path;
    int pos = tex_path.rfind( "/" );
    material_name = material_name.substr( pos + 1 );

    return add_item( material_name, tex_path );
}


// Load a library of material properties
bool SGMaterialLib::add_item ( const string &mat_name, const string &full_path )
{
    int pos = full_path.rfind( "/" );
    string tex_name = full_path.substr( pos + 1 );
    string tex_path = full_path.substr( 0, pos );

    SG_LOG( SG_TERRAIN, SG_INFO, "  Loading material " 
	    << mat_name << " (" << full_path << ")");

    matlib[mat_name] = new SGMaterial( full_path );
    matlib[mat_name]->add_name(mat_name);

    return true;
}


// Load a library of material properties
bool SGMaterialLib::add_item ( const string &mat_name, osg::StateSet *state )
{
    matlib[mat_name] = new SGMaterial( state );
    matlib[mat_name]->add_name(mat_name);

    SG_LOG( SG_TERRAIN, SG_INFO, "  Loading material given a premade "
	    << "osg::StateSet = " << mat_name );

    return true;
}


// find a material record by material name
SGMaterial *SGMaterialLib::find( const string& material ) {
    SGMaterial *result = NULL;
    material_map_iterator it = matlib.find( material );
    if ( it != end() ) {
	result = it->second;
	return result;
    }

    return NULL;
}


// Destructor
SGMaterialLib::~SGMaterialLib ( void ) {
}


// Load one pending "deferred" texture.  Return true if a texture
// loaded successfully, false if no pending, or error.
void SGMaterialLib::load_next_deferred() {
    // container::iterator it = begin();
    for ( material_map_iterator it = begin(); it != end(); it++ ) {
	/* we don't need the key, but here's how we'd get it if we wanted it. */
        // const string &key = it->first;
	SGMaterial *slot = it->second;
	if (slot->load_texture())
	  return;
    }
}

// Return the material from that given leaf
const SGMaterial* SGMaterialLib::findMaterial(const osg::Node* leaf) const
{
  if (!leaf)
    return 0;
  
  const osg::Referenced* base = leaf->getUserData();
  if (!base)
    return 0;

  const SGMaterialUserData* matUserData
    = dynamic_cast<const SGMaterialUserData*>(base);
  if (!matUserData)
    return 0;

  return matUserData->getMaterial();
}
