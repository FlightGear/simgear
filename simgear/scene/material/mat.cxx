// mat.cxx -- class to handle material properties
//
// Written by Curtis Olson, started May 1998.
//
// Copyright (C) 1998 - 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#include <simgear/compiler.h>

#include <string.h>
#include <map>
#include <vector>
#include<string>

#include "mat.hxx"

#include <osg/CullFace>
#include <osg/Material>
#include <osg/ShadeModel>
#include <osg/StateSet>
#include <osg/TexEnv>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>

#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>

#include "Effect.hxx"
#include "Technique.hxx"
#include "Pass.hxx"

using std::map;
using namespace simgear;


////////////////////////////////////////////////////////////////////////
// Constructors and destructor.
////////////////////////////////////////////////////////////////////////

SGMaterial::_internal_state::_internal_state(osg::StateSet *s,
                                             const std::string &t, bool l ) :
    state(s), texture_path(t), texture_loaded(l)
{
}

SGMaterial::SGMaterial( const string &fg_root, const SGPropertyNode *props )
{
    init();
    read_properties( fg_root, props );
    build_state( false );
}

SGMaterial::SGMaterial( const string &texpath )
{
    init();

    _internal_state st( NULL, texpath, false );
    _status.push_back( st );

    build_state( true );
}

SGMaterial::SGMaterial( osg::StateSet *s )
{
    init();
    set_state( s );
}

SGMaterial::~SGMaterial (void)
{
}


////////////////////////////////////////////////////////////////////////
// Public methods.
////////////////////////////////////////////////////////////////////////

void
SGMaterial::read_properties( const string &fg_root, const SGPropertyNode *props)
{
				// Gather the path(s) to the texture(s)
  vector<SGPropertyNode_ptr> textures = props->getChildren("texture");
  for (unsigned int i = 0; i < textures.size(); i++)
  {
    string tname = textures[i]->getStringValue();
    if (tname.empty()) {
        tname = "unknown.rgb";
    }

    SGPath tpath( fg_root );
    tpath.append("Textures.high");
    tpath.append(tname);
    if ( !osgDB::fileExists(tpath.str()) ) {
      tpath = SGPath( fg_root );
      tpath.append("Textures");
      tpath.append(tname);
    }

    if ( osgDB::fileExists(tpath.str()) ) {
      _internal_state st( NULL, tpath.str(), false );
      _status.push_back( st );
    }
  }

  if (textures.size() == 0) {
    string tname = "unknown.rgb";
    SGPath tpath( fg_root );
    tpath.append("Textures");
    tpath.append("Terrain");
    tpath.append(tname);
    _internal_state st( NULL, tpath.str(), true );
    _status.push_back( st );
  }

  xsize = props->getDoubleValue("xsize", 0.0);
  ysize = props->getDoubleValue("ysize", 0.0);
  wrapu = props->getBoolValue("wrapu", true);
  wrapv = props->getBoolValue("wrapv", true);
  mipmap = props->getBoolValue("mipmap", true);
  light_coverage = props->getDoubleValue("light-coverage", 0.0);
  tree_coverage = props->getDoubleValue("tree-coverage", 0.0);
  tree_height = props->getDoubleValue("tree-height-m", 0.0);
  tree_width = props->getDoubleValue("tree-width-m", 0.0);
  tree_range = props->getDoubleValue("tree-range-m", 0.0);
  tree_varieties = props->getIntValue("tree-varieties", 1);

  SGPath tpath( fg_root );
  tpath.append(props->getStringValue("tree-texture"));
  tree_texture = tpath.str();

  // surface values for use with ground reactions
  solid = props->getBoolValue("solid", true);
  friction_factor = props->getDoubleValue("friction-factor", 1.0);
  rolling_friction = props->getDoubleValue("rolling-friction", 0.02);
  bumpiness = props->getDoubleValue("bumpiness", 0.0);
  load_resistance = props->getDoubleValue("load-resistance", 1e30);

  // Taken from default values as used in ac3d
  ambient[0] = props->getDoubleValue("ambient/r", 0.2);
  ambient[1] = props->getDoubleValue("ambient/g", 0.2);
  ambient[2] = props->getDoubleValue("ambient/b", 0.2);
  ambient[3] = props->getDoubleValue("ambient/a", 1.0);

  diffuse[0] = props->getDoubleValue("diffuse/r", 0.8);
  diffuse[1] = props->getDoubleValue("diffuse/g", 0.8);
  diffuse[2] = props->getDoubleValue("diffuse/b", 0.8);
  diffuse[3] = props->getDoubleValue("diffuse/a", 1.0);

  specular[0] = props->getDoubleValue("specular/r", 0.0);
  specular[1] = props->getDoubleValue("specular/g", 0.0);
  specular[2] = props->getDoubleValue("specular/b", 0.0);
  specular[3] = props->getDoubleValue("specular/a", 1.0);

  emission[0] = props->getDoubleValue("emissive/r", 0.0);
  emission[1] = props->getDoubleValue("emissive/g", 0.0);
  emission[2] = props->getDoubleValue("emissive/b", 0.0);
  emission[3] = props->getDoubleValue("emissive/a", 1.0);

  shininess = props->getDoubleValue("shininess", 1.0);

  vector<SGPropertyNode_ptr> object_group_nodes =
    ((SGPropertyNode *)props)->getChildren("object-group");
  for (unsigned int i = 0; i < object_group_nodes.size(); i++)
    object_groups.push_back(new SGMatModelGroup(object_group_nodes[i]));

  // read glyph table for taxi-/runway-signs
  vector<SGPropertyNode_ptr> glyph_nodes = props->getChildren("glyph");
  for (unsigned int i = 0; i < glyph_nodes.size(); i++) {
    const char *name = glyph_nodes[i]->getStringValue("name");
    if (name)
      glyphs[name] = new SGMaterialGlyph(glyph_nodes[i]);
  }
}



////////////////////////////////////////////////////////////////////////
// Private methods.
////////////////////////////////////////////////////////////////////////

void 
SGMaterial::init ()
{
    _status.clear();
    _current_ptr = 0;
    xsize = 0;
    ysize = 0;
    wrapu = true;
    wrapv = true;

    mipmap = true;
    light_coverage = 0.0;

    solid = true;
    friction_factor = 1;
    rolling_friction = 0.02;
    bumpiness = 0;
    load_resistance = 1e30;

    shininess = 1.0;
    for (int i = 0; i < 4; i++) {
        ambient[i]  = (i < 3) ? 0.2 : 1.0;
        specular[i] = (i < 3) ? 0.0 : 1.0;
        diffuse[i]  = (i < 3) ? 0.8 : 1.0;
        emission[i] = (i < 3) ? 0.0 : 1.0;
    }
}

Effect* SGMaterial::get_effect(int n)
{
    if (_status.size() == 0) {
        SG_LOG( SG_GENERAL, SG_WARN, "No effect available.");
        return 0;
    }
    int i = n >= 0 ? n : _current_ptr;
    if(!_status[i].texture_loaded) {
        assignTexture(_status[i].state.get(), _status[i].texture_path,
                      wrapu, wrapv, mipmap);
        _status[i].texture_loaded = true;
    }
    // XXX This business of returning a "random" alternate texture is
    // really bogus. It means that the appearance of the terrain
    // depends on the order in which it is paged in!
    _current_ptr = (_current_ptr + 1) % _status.size();
    return _status[i].effect.get();
}

void 
SGMaterial::build_state( bool defer_tex_load )
{
    StateAttributeFactory *attrFact = StateAttributeFactory::instance();
    SGMaterialUserData* user = new SGMaterialUserData(this);
    for (unsigned int i = 0; i < _status.size(); i++)
    {
        Pass *pass = new Pass;
        pass->setUserData(user);

        // Set up the textured state
        pass->setAttribute(attrFact->getSmoothShadeModel());
        pass->setAttributeAndModes(attrFact->getCullFaceBack());

        pass->setMode(GL_LIGHTING, osg::StateAttribute::ON);

        _status[i].texture_loaded = false;

        osg::Material* material = new osg::Material;
        material->setColorMode(osg::Material::AMBIENT_AND_DIFFUSE);
        material->setAmbient(osg::Material::FRONT_AND_BACK, ambient.osg());
        material->setDiffuse(osg::Material::FRONT_AND_BACK, diffuse.osg());
        material->setSpecular(osg::Material::FRONT_AND_BACK, specular.osg());
        material->setEmission(osg::Material::FRONT_AND_BACK, emission.osg());
        material->setShininess(osg::Material::FRONT_AND_BACK, shininess );
        pass->setAttribute(material);

        if (ambient[3] < 1 || diffuse[3] < 1 ||
            specular[3] < 1 || emission[3] < 1) {
          pass->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
          pass->setMode(GL_BLEND, osg::StateAttribute::ON);
          pass->setMode(GL_ALPHA_TEST, osg::StateAttribute::ON);
        } else {
          pass->setRenderingHint(osg::StateSet::OPAQUE_BIN);
          pass->setMode(GL_BLEND, osg::StateAttribute::OFF);
          pass->setMode(GL_ALPHA_TEST, osg::StateAttribute::OFF);
        }

        _status[i].state = pass;
        Technique* tniq = new Technique(true);
        tniq->passes.push_back(pass);
        Effect* effect = new Effect;
        effect->techniques.push_back(tniq);
        effect->setUserData(user);
        _status[i].effect = effect;
    }
}


void SGMaterial::set_state( osg::StateSet *s )
{
    _status.push_back( _internal_state( s, "", true ) );
}

void SGMaterial::assignTexture( osg::StateSet *state, const std::string &fname,
                 bool _wrapu, bool _wrapv, bool _mipmap )
{
   osg::Texture2D* texture = SGLoadTexture2D(fname, 0, _wrapu, _wrapv,
                                             mipmap ? -1 : 0);
   texture->setMaxAnisotropy( SGGetTextureFilter());
   state->setTextureAttributeAndModes(0, texture);

   StateAttributeFactory *attrFact = StateAttributeFactory::instance();
   state->setTextureAttributeAndModes(0, attrFact->getStandardTexEnv());
}

SGMaterialGlyph* SGMaterial::get_glyph (const string& name) const
{
  map<string, SGSharedPtr<SGMaterialGlyph> >::const_iterator it;
  it = glyphs.find(name);
  if (it == glyphs.end())
    return 0;

  return it->second;
}


////////////////////////////////////////////////////////////////////////
// SGMaterialGlyph.
////////////////////////////////////////////////////////////////////////

SGMaterialGlyph::SGMaterialGlyph(SGPropertyNode *p) :
    _left(p->getDoubleValue("left", 0.0)),
    _right(p->getDoubleValue("right", 1.0))
{
}

void
SGSetTextureFilter( int max) {
	SGSceneFeatures::instance()->setTextureFilter( max);
}

int
SGGetTextureFilter() {
	return SGSceneFeatures::instance()->getTextureFilter();
}
