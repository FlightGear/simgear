// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2021 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef BVHMaterial_hxx
#define BVHMaterial_hxx

#include <simgear/structure/SGReferenced.hxx>
#include <simgear/props/props.hxx>

#include <cstdio>

namespace simgear {

class BVHMaterial : public SGReferenced {
public:
    virtual ~BVHMaterial() = default;

    BVHMaterial(bool solid=true, double friction_factor=1.0, double rolling_friction=0.02, double bumpiness=0.0, double load_resistance=1e30) :
      _solid(solid), _friction_factor(friction_factor), _rolling_friction(rolling_friction), _bumpiness(bumpiness), _load_resistance(load_resistance)
    { }
    
    /**
     * Return if the surface material is solid, if it is not solid, a fluid
     * can be assumed, that is usually water.
     */
    bool get_solid () const {
      return _solid_is_prop ? _solid_property->getBoolValue() : _solid;
    }

    /**
     * Return whether the solid factor is a propery.
     */
    bool solid_is_prop () const { return _solid_is_prop; }
    
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
    bool _solid = true;
    
    // the friction factor of that surface material
    double _friction_factor = 1.0;
    
    // the rolling friction of that surface material
    double _rolling_friction = 0.02;
    
    // the bumpiness of that surface material
    double _bumpiness = 0.0;
    
    // the load resistance of that surface material
    double _load_resistance = 1e30;

    // Placeholder for the solid property, if defined
    SGPropertyNode_ptr _solid_property;
    bool _solid_is_prop = false;
};


}

#endif
