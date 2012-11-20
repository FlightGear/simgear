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
    SGMaterial *find( const std::string& material );

    material_map_iterator begin() { return matlib.begin(); }
    const_material_map_iterator begin() const { return matlib.begin(); }

    material_map_iterator end() { return matlib.end(); }
    const_material_map_iterator end() const { return matlib.end(); }

    static const SGMaterial *findMaterial(const osg::Geode* geode);

    // Destructor
    ~SGMaterialLib ( void );
};


#endif // _MATLIB_HXX 
