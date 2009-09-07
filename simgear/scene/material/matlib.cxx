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

#if defined ( __CYGWIN__ )
#include <ieeefp.h>
#endif

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/structure/exception.hxx>

#include <string.h>
#include <string>

#include <osgDB/Registry>

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/misc/sgstream.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/props/condition.hxx>
#include <simgear/scene/tgdb/userdata.hxx>

#include "mat.hxx"

#include "Effect.hxx"
#include "Technique.hxx"
#include "matlib.hxx"

using std::string;

// Constructor
SGMaterialLib::SGMaterialLib ( void ) {
}

// Load a library of material properties
bool SGMaterialLib::load( const string &fg_root, const string& mpath,
        SGPropertyNode *prop_root )
{
    SGPropertyNode materials;

    SG_LOG( SG_INPUT, SG_INFO, "Reading materials from " << mpath );
    try {
        readProperties( mpath, &materials );
    } catch (const sg_exception &ex) {
        SG_LOG( SG_INPUT, SG_ALERT, "Error reading materials: "
                << ex.getMessage() );
        throw;
    }
    osg::ref_ptr<osgDB::ReaderWriter::Options> options
        = new osgDB::ReaderWriter::Options;
    options->setObjectCacheHint(osgDB::ReaderWriter::Options::CACHE_ALL);
    options->setDatabasePath(fg_root);
    int nMaterials = materials.nChildren();
    for (int i = 0; i < nMaterials; i++) {
        const SGPropertyNode *node = materials.getChild(i);
        if (!strcmp(node->getName(), "material")) {
            const SGPropertyNode *conditionNode = node->getChild("condition");
            if (conditionNode) {
                SGSharedPtr<const SGCondition> condition = sgReadCondition(prop_root, conditionNode);
                if (!condition->test()) {
                    SG_LOG(SG_INPUT, SG_DEBUG, "Skipping material entry #"
                        << i << " (condition false)");
                    continue;
                }
            }

            SGSharedPtr<SGMaterial> m = new SGMaterial(options.get(), node);

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
    SG_LOG( SG_GENERAL, SG_INFO, "SGMaterialLib::~SGMaterialLib() size=" << matlib.size());
}

const SGMaterial*
SGMaterialLib::findMaterial(const osg::Geode* geode)
{
    if (!geode)
        return 0;
    const simgear::EffectGeode* effectGeode;
    effectGeode = dynamic_cast<const simgear::EffectGeode*>(geode);
    if (!effectGeode)
        return 0;
    const simgear::Effect* effect = effectGeode->getEffect();
    if (!effect)
        return 0;
    const SGMaterialUserData* userData;
    userData = dynamic_cast<const SGMaterialUserData*>(effect->getUserData());
    if (!userData)
        return 0;
    return userData->getMaterial();
}
