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

#include <simgear/structure/SGSharedPtr.hxx>

#include <memory>
#include <string>		// Standard C++ string library
#include <map>			// STL associative "array"
#include <vector>		// STL "array"

class SGMaterial;
class SGPropertyNode;

namespace simgear { class Effect; }
namespace osg { class Geode; }

// Material management class
class SGMaterialLib {

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
    
    typedef std::map < std::string, SGSharedPtr<SGMaterial> > active_material_cache;
    active_material_cache active_cache;
    
public:

    // Constructor
    SGMaterialLib ( void );

    // Load a library of material properties
    bool load( const std::string &fg_root, const std::string& mpath,
            SGPropertyNode *prop_root );
    // find a material record by material name
    SGMaterial *find( const std::string& material ) const;

    /**
     * Material lookup involves evaluation of SGConditions to determine which
     * possible material (by season, region, etc) is valid. This involves
     * vproperty tree queries, so repeated calls to find() can cause
     * race conditions when called from the osgDB pager thread. (especially
     * during startup)
     *
     * To fix this, and also avoid repeated re-evaluation of the material
     * conditions, we provide a version which uses a cached, threadsafe table
     * of the currently valid materials. The main thread calls the refresh
     * method below to evaluate the valid materials, and findCached can be
     * safely called from other threads with no access to unprotected state.
     */
    SGMaterial *findCached( const std::string& material ) const;
    void refreshActiveMaterials();
    
    material_map_iterator begin() { return matlib.begin(); }
    const_material_map_iterator begin() const { return matlib.begin(); }

    material_map_iterator end() { return matlib.end(); }
    const_material_map_iterator end() const { return matlib.end(); }

    static const SGMaterial *findMaterial(const osg::Geode* geode);

    // Destructor
    ~SGMaterialLib ( void );
};


#endif // _MATLIB_HXX 
