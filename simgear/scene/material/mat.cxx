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

#include <plib/ul.h>

#ifdef SG_MATH_EXCEPTION_CLASH
#  include <math.h>
#endif

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>

#include "mat.hxx"


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

    _internal_state st( NULL, texpath, false );
    _status.push_back( st );

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
				// Gather the path(s) to the texture(s)
  vector<SGPropertyNode_ptr> textures = props->getChildren("texture");
  for (int i = 0; i < textures.size(); i++)
  {
    string tname = textures[i]->getStringValue();
    if (tname == "")
        tname = "unknown.rgb";

    SGPath tpath( fg_root );
    tpath.append("Textures.high");
    tpath.append(tname);
    if (!ulFileExists(tpath.c_str())) {
      tpath = SGPath( fg_root );
      tpath.append("Textures");
      tpath.append(tname);
    }
    _internal_state st( NULL, tpath.str(), false );
    _status.push_back( st );
  }

  if (textures.size() == 0) {
    string tname = "unknown.rgb";
    SGPath tpath( fg_root );
    tpath.append("Textures");
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
    refcount = 0;
    shininess = 1.0;
    for (int i = 0; i < 4; i++) {
        ambient[i]  = (i < 3) ? 0.2 : 1.0;
        specular[i] = (i < 3) ? 0.0 : 1.0;
        diffuse[i]  = (i < 3) ? 0.8 : 1.0;
        emission[i] = (i < 3) ? 0.0 : 1.0;
    }
}

bool
SGMaterial::load_texture ( int n )
{
    int i   = (n >= 0) ? n   : 0 ;
    int end = (n >= 0) ? n+1 : _status.size();

    for (; i < end; i++)
    {
        if ( !_status[i].texture_loaded ) {
            SG_LOG( SG_GENERAL, SG_INFO, "Loading deferred texture "
                                          << _status[i].texture_path );
            _status[i].state->setTexture(
                   (char *)_status[i].texture_path.c_str(),
                   wrapu, wrapv, mipmap );
            _status[i].texture_loaded = true;
       }
    }
    return true;
}

ssgSimpleState *
SGMaterial::get_state (int n)
{
    if (_status.size() == 0) {
        SG_LOG( SG_GENERAL, SG_WARN, "No state available.");
        return NULL;
    }

    if ( _current_ptr >= _status.size())
        _current_ptr = 0;

    return (n >= 0) ? _status[n].state : _status[_current_ptr++].state;
}


void 
SGMaterial::build_ssg_state( bool defer_tex_load )
{
    GLenum shade_model = GL_SMOOTH;
    
    for (int i = 0; i < _status.size(); i++)
    {
        ssgSimpleState *state = new ssgSimpleState();
        state->ref();

        // Set up the textured state
        state->setShadeModel( shade_model );
        state->enable( GL_LIGHTING );
        state->enable ( GL_CULL_FACE ) ;
        state->enable( GL_TEXTURE_2D );
        state->disable( GL_BLEND );
        state->disable( GL_ALPHA_TEST );

        if ( !defer_tex_load ) {
            SG_LOG(SG_INPUT, SG_INFO, "    " << _status[i].texture_path );
	    state->setTexture( (char *)_status[i].texture_path.c_str(),
                                wrapu, wrapv );
            _status[i].texture_loaded = true;
        } else {
            _status[i].texture_loaded = false;
        }

        state->enable( GL_COLOR_MATERIAL );
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

        _status[i].state = state;
    }
}


void SGMaterial::set_ssg_state( ssgSimpleState *s )
{
    _internal_state st( s, "", true );
    st.state->ref();
    _status.push_back( st );
}

// end of mat.cxx
