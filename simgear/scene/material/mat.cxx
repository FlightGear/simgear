// mat.cxx -- class to handle material properties
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2000  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include <map>
SG_USING_STD(map);

#include <simgear/compiler.h>

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/scene/model/loader.hxx>

#include "mat.hxx"


////////////////////////////////////////////////////////////////////////
// Local static functions.
////////////////////////////////////////////////////////////////////////

/**
 * Internal method to test whether a file exists.
 *
 * TODO: this should be moved to a SimGear library of local file
 * functions.
 */
static inline bool
local_file_exists( const string& path ) {
    sg_gzifstream in( path );
    if ( ! in.is_open() ) {
	return false;
    } else {
	return true;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGMaterial::Object.
////////////////////////////////////////////////////////////////////////

SGMaterial::Object::Object (const SGPropertyNode * node, double range_m)
  : _models_loaded(false),
    _coverage_m2(node->getDoubleValue("coverage-m2", 1000000)),
    _range_m(range_m)
{
				// Sanity check
  if (_coverage_m2 < 1000) {
    SG_LOG(SG_INPUT, SG_ALERT, "Random object coverage " << _coverage_m2
	   << " is too small, forcing, to 1000");
    _coverage_m2 = 1000;
  }

				// Note all the model paths
  vector <SGPropertyNode_ptr> path_nodes = node->getChildren("path");
  for (unsigned int i = 0; i < path_nodes.size(); i++)
    _paths.push_back(path_nodes[i]->getStringValue());

				// Note the heading type
  string hdg = node->getStringValue("heading-type", "fixed");
  if (hdg == "fixed") {
    _heading_type = HEADING_FIXED;
  } else if (hdg == "billboard") {
    _heading_type = HEADING_BILLBOARD;
  } else if (hdg == "random") {
    _heading_type = HEADING_RANDOM;
  } else {
    _heading_type = HEADING_FIXED;
    SG_LOG(SG_INPUT, SG_ALERT, "Unknown heading type: " << hdg
	   << "; using 'fixed' instead.");
  }

  // uncomment to preload models
  // load_models();
}

SGMaterial::Object::~Object ()
{
  for (unsigned int i = 0; i < _models.size(); i++) {
    if (_models[i] != 0) {
      _models[i]->deRef();
      _models[i] = 0;
    }
  }
}

int
SGMaterial::Object::get_model_count( SGModelLoader *loader,
                                   const string &fg_root,
                                   SGPropertyNode *prop_root,
                                   double sim_time_sec )
{
  load_models( loader, fg_root, prop_root, sim_time_sec );
  return _models.size();
}

inline void
SGMaterial::Object::load_models ( SGModelLoader *loader,
                                const string &fg_root,
                                SGPropertyNode *prop_root,
                                double sim_time_sec )
{
				// Load model only on demand
  if (!_models_loaded) {
    for (unsigned int i = 0; i < _paths.size(); i++) {
      ssgEntity *entity = loader->load_model( fg_root, _paths[i],
                                              prop_root, sim_time_sec );
      if (entity != 0) {
                                // FIXME: this stuff can be handled
                                // in the XML wrapper as well (at least,
                                // the billboarding should be handled
                                // there).
	float ranges[] = {0, _range_m};
	ssgRangeSelector * lod = new ssgRangeSelector;
        lod->ref();
        lod->setRanges(ranges, 2);
	if (_heading_type == HEADING_BILLBOARD) {
	  ssgCutout * cutout = new ssgCutout(false);
	  cutout->addKid(entity);
	  lod->addKid(cutout);
	} else {
	  lod->addKid(entity);
	}
	_models.push_back(lod);
      } else {
	SG_LOG(SG_INPUT, SG_ALERT, "Failed to load object " << _paths[i]);
      }
    }
  }
  _models_loaded = true;
}

ssgEntity *
SGMaterial::Object::get_model( int index,
                             SGModelLoader *loader,
                             const string &fg_root,
                             SGPropertyNode *prop_root,
                             double sim_time_sec )
{
  load_models( loader, fg_root, prop_root, sim_time_sec ); // comment this out if preloading models
  return _models[index];
}

ssgEntity *
SGMaterial::Object::get_random_model( SGModelLoader *loader,
                                    const string &fg_root,
                                    SGPropertyNode *prop_root,
                                    double sim_time_sec )
{
  load_models( loader, fg_root, prop_root, sim_time_sec ); // comment this out if preloading models
  int nModels = _models.size();
  int index = int(sg_random() * nModels);
  if (index >= nModels)
    index = 0;
  return _models[index];
}

double
SGMaterial::Object::get_coverage_m2 () const
{
  return _coverage_m2;
}

SGMaterial::Object::HeadingType
SGMaterial::Object::get_heading_type () const
{
  return _heading_type;
}



////////////////////////////////////////////////////////////////////////
// Implementation of SGMaterial::ObjectGroup.
////////////////////////////////////////////////////////////////////////

SGMaterial::ObjectGroup::ObjectGroup (SGPropertyNode * node)
  : _range_m(node->getDoubleValue("range-m", 2000))
{
				// Load the object subnodes
  vector<SGPropertyNode_ptr> object_nodes =
    ((SGPropertyNode *)node)->getChildren("object");
  for (unsigned int i = 0; i < object_nodes.size(); i++) {
    const SGPropertyNode * object_node = object_nodes[i];
    if (object_node->hasChild("path"))
      _objects.push_back(new Object(object_node, _range_m));
    else
      SG_LOG(SG_INPUT, SG_ALERT, "No path supplied for object");
  }
}

SGMaterial::ObjectGroup::~ObjectGroup ()
{
  for (unsigned int i = 0; i < _objects.size(); i++) {
    delete _objects[i];
    _objects[i] = 0;
  }
}

double
SGMaterial::ObjectGroup::get_range_m () const
{
  return _range_m;
}

int
SGMaterial::ObjectGroup::get_object_count () const
{
  return _objects.size();
}

SGMaterial::Object *
SGMaterial::ObjectGroup::get_object (int index) const
{
  return _objects[index];
}



////////////////////////////////////////////////////////////////////////
// Constructors and destructor.
////////////////////////////////////////////////////////////////////////


SGMaterial::SGMaterial( const string &fg_root, const SGPropertyNode *props )
{
    init();
    read_properties( fg_root, props );
    build_ssg_state( false );
}

SGMaterial::SGMaterial( const string &texpath )
{
    init();
    texture_path = texpath;
    build_ssg_state( true );
}

SGMaterial::SGMaterial( ssgSimpleState *s )
{
    init();
    set_ssg_state( s );
}

SGMaterial::~SGMaterial (void)
{
  for (unsigned int i = 0; i < object_groups.size(); i++) {
    delete object_groups[i];
    object_groups[i] = 0;
  }
}



////////////////////////////////////////////////////////////////////////
// Public methods.
////////////////////////////////////////////////////////////////////////

void
SGMaterial::read_properties( const string &fg_root, const SGPropertyNode * props )
{
				// Get the path to the texture
  string tname = props->getStringValue("texture", "unknown.rgb");
  SGPath tpath( fg_root );
  tpath.append("Textures.high");
  tpath.append(tname);
  if (!local_file_exists(tpath.str())) {
    tpath = SGPath( fg_root );
    tpath.append("Textures");
    tpath.append(tname);
  }
  texture_path = tpath.str();

  xsize = props->getDoubleValue("xsize", 0.0);
  ysize = props->getDoubleValue("ysize", 0.0);
  wrapu = props->getBoolValue("wrapu", true);
  wrapv = props->getBoolValue("wrapv", true);
  mipmap = props->getBoolValue("mipmap", true);
  light_coverage = props->getDoubleValue("light-coverage", 0.0);

  ambient[0] = props->getDoubleValue("ambient/r", 0.0);
  ambient[1] = props->getDoubleValue("ambient/g", 0.0);
  ambient[2] = props->getDoubleValue("ambient/b", 0.0);
  ambient[3] = props->getDoubleValue("ambient/a", 0.0);

  diffuse[0] = props->getDoubleValue("diffuse/r", 0.0);
  diffuse[1] = props->getDoubleValue("diffuse/g", 0.0);
  diffuse[2] = props->getDoubleValue("diffuse/b", 0.0);
  diffuse[3] = props->getDoubleValue("diffuse/a", 0.0);

  specular[0] = props->getDoubleValue("specular/r", 0.0);
  specular[1] = props->getDoubleValue("specular/g", 0.0);
  specular[2] = props->getDoubleValue("specular/b", 0.0);
  specular[3] = props->getDoubleValue("specular/a", 0.0);

  emission[0] = props->getDoubleValue("emissive/r", 0.0);
  emission[1] = props->getDoubleValue("emissive/g", 0.0);
  emission[2] = props->getDoubleValue("emissive/b", 0.0);
  emission[3] = props->getDoubleValue("emissive/a", 0.0);

  shininess = props->getDoubleValue("shininess", 0.0);

  vector<SGPropertyNode_ptr> object_group_nodes =
    ((SGPropertyNode *)props)->getChildren("object-group");
  for (unsigned int i = 0; i < object_group_nodes.size(); i++)
    object_groups.push_back(new ObjectGroup(object_group_nodes[i]));
}



////////////////////////////////////////////////////////////////////////
// Private methods.
////////////////////////////////////////////////////////////////////////

void 
SGMaterial::init ()
{
    texture_path = "";
    state = NULL;
    xsize = 0;
    ysize = 0;
    wrapu = true;
    wrapv = true;
    mipmap = true;
    light_coverage = 0.0;
    texture_loaded = false;
    refcount = 0;
    shininess = 0.0;
    for (int i = 0; i < 4; i++) {
        ambient[i] = diffuse[i] = specular[i] = emission[i] = 0.0;
    }
}

bool
SGMaterial::load_texture ()
{
    if ( texture_loaded ) {
        return false;
    } else {
        SG_LOG( SG_GENERAL, SG_INFO, "Loading deferred texture "
                << texture_path );
        state->setTexture( (char *)texture_path.c_str(), wrapu, wrapv, mipmap );
        texture_loaded = true;
        return true;
    }
}


void 
SGMaterial::build_ssg_state( bool defer_tex_load )
{
    GLenum shade_model = GL_SMOOTH;
    
    state = new ssgSimpleState();
    state->ref();

    // Set up the textured state
    state->setShadeModel( shade_model );
    state->enable( GL_LIGHTING );
    state->enable ( GL_CULL_FACE ) ;
    state->enable( GL_TEXTURE_2D );
    state->disable( GL_BLEND );
    state->disable( GL_ALPHA_TEST );
    if ( !defer_tex_load ) {
        SG_LOG(SG_INPUT, SG_INFO, "    " << texture_path );
	state->setTexture( (char *)texture_path.c_str(), wrapu, wrapv );
	texture_loaded = true;
    } else {
	texture_loaded = false;
    }
    state->enable( GL_COLOR_MATERIAL );
#if 0
    state->setColourMaterial( GL_AMBIENT_AND_DIFFUSE );
    state->setMaterial( GL_EMISSION, 0, 0, 0, 1 );
    state->setMaterial( GL_SPECULAR, 0, 0, 0, 1 );
#else
    state->setMaterial ( GL_AMBIENT,
                            ambient[0], ambient[1],
                            ambient[2], ambient[3] ) ;
    state->setMaterial ( GL_DIFFUSE,
                            diffuse[0], diffuse[1],
                            diffuse[2], diffuse[3] ) ;
    state->setMaterial ( GL_SPECULAR,
                            specular[0], specular[1],
                            specular[2], specular[3] ) ;
    state->setMaterial ( GL_EMISSION,
                            emission[0], emission[1],
                            emission[2], emission[3] ) ;
    state->setShininess ( shininess );
#endif
}


void SGMaterial::set_ssg_state( ssgSimpleState *s )
{
    state = s;
    state->ref();
    texture_loaded = true;
}

// end of newmat.cxx
