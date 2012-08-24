// Copyright (C) 2008 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef BVHMaterial_hxx
#define BVHMaterial_hxx

#include <simgear/structure/SGReferenced.hxx>

namespace simgear {

class BVHMaterial : public SGReferenced {
public:
    BVHMaterial();
    virtual ~BVHMaterial();

    /**
     * Return if the surface material is solid, if it is not solid, a fluid
     * can be assumed, that is usually water.
     */
    bool get_solid () const { return _solid; }
    
    /**
     * Get the friction factor for that material
     */
    double get_friction_factor () const { return _friction_factor; }
    
    /**
     * Get the rolling friction for that material
     */
    double get_rolling_friction () const { return _rolling_friction; }
    
    /**
     * Get the bumpines for that material
     */
    double get_bumpiness () const { return _bumpiness; }
    
    /**
     * Get the load resistance
     */
    double get_load_resistance () const { return _load_resistance; }

protected:    
    // True if the material is solid, false if it is a fluid
    bool _solid;
    
    // the friction factor of that surface material
    double _friction_factor;
    
    // the rolling friction of that surface material
    double _rolling_friction;
    
    // the bumpiness of that surface material
    double _bumpiness;
    
    // the load resistance of that surface material
    double _load_resistance;
};

}

#endif
