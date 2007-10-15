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

#if defined ( __CYGWIN__ )
#include <ieeefp.h>
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

// Constructor
SGMaterialLib::SGMaterialLib ( void ) {
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

    int nMaterials = materials.nChildren();
    for (int i = 0; i < nMaterials; i++) {
        const SGPropertyNode * node = materials.getChild(i);
        if (!strcmp(node->getName(), "material")) {
            SGSharedPtr<SGMaterial> m = new SGMaterial(fg_root, node, season);

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

const SGMaterial*
SGMaterialLib::findMaterial(const osg::StateSet* stateSet) const
{
  if (!stateSet)
    return 0;
  
  const osg::Referenced* base = stateSet->getUserData();
  if (!base)
    return 0;

  const SGMaterialUserData* matUserData
    = dynamic_cast<const SGMaterialUserData*>(base);
  if (!matUserData)
    return 0;

  return matUserData->getMaterial();
}
