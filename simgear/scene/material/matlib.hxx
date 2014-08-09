// matlib.hxx -- class to handle material properties
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


#ifndef _MATLIB_HXX
#define _MATLIB_HXX


#include <simgear/compiler.h>

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGMath.hxx>

#include <memory>
#include <string>		// Standard C++ string library
#include <map>			// STL associative "array"
#include <vector>		// STL "array"

class SGMaterial;
class SGPropertyNode;

namespace simgear { class Effect; }
namespace osg { class Geode; }

// Material cache class
class SGMaterialCache : public osg::Referenced
{
private:
    typedef std::map < std::string, SGSharedPtr<SGMaterial> > material_cache;
    material_cache cache;

public:
    // Constructor
    SGMaterialCache ( void );

    // Insertion
    void insert( const std::string& name, SGSharedPtr<SGMaterial> material );

    // Lookup
    SGMaterial *find( const std::string& material ) const;

    // Destructor
    ~SGMaterialCache ( void );
};

// Material management class
class SGMaterialLib : public SGReferenced
{

private:
    class MatLibPrivate;
    std::auto_ptr<MatLibPrivate> d;
    
    // associative array of materials
    typedef std::vector< SGSharedPtr<SGMaterial> > material_list;    
    typedef material_list::iterator material_list_iterator;
    
    typedef std::map < std::string,  material_list> material_map;
    typedef material_map::iterator material_map_iterator;
    typedef material_map::const_iterator const_material_map_iterator;

    material_map matlib;
    
public:

    // Constructor
    SGMaterialLib ( void );

    // Load a library of material properties
    bool load( const std::string &fg_root, const std::string& mpath,
            SGPropertyNode *prop_root );
    // find a material record by material name
    SGMaterial *find( const std::string& material, SGVec2f center ) const;
    SGMaterial *find( const std::string& material, const SGGeod& center ) const;

    /**
     * Material lookup involves evaluation of position and SGConditions to
     * determine which possible material (by season, region, etc) is valid.
     * This involves property tree queries, so repeated calls to find() can cause
     * race conditions when called from the osgDB pager thread. (especially
     * during startup)
     *
     * To fix this, and also avoid repeated re-evaluation of the material
     * conditions, we provide factory method to generate a material library
     * cache of the valid materials based on the current state and a given position.
     */

    SGMaterialCache *generateMatCache( SGVec2f center);
    SGMaterialCache *generateMatCache( SGGeod center);

    material_map_iterator begin() { return matlib.begin(); }
    const_material_map_iterator begin() const { return matlib.begin(); }

    material_map_iterator end() { return matlib.end(); }
    const_material_map_iterator end() const { return matlib.end(); }

    static const SGMaterial *findMaterial(const osg::Geode* geode);

    // Destructor
    ~SGMaterialLib ( void );

};

typedef SGSharedPtr<SGMaterialLib> SGMaterialLibPtr;

#endif // _MATLIB_HXX 
