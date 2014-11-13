// 3D cloud class
//
// Written by Harald JOHNSEN, started April 2005.
//
// Copyright (C) 2005  Harald JOHNSEN - hjohnsen@evc.net
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
//

#ifndef _NEWCLOUD_HXX
#define _NEWCLOUD_HXX

#include <simgear/compiler.h>
#include <string>
#include <vector>
#include <osg/Fog>

#include <simgear/math/sg_random.h>
#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>

using std::string;
using std::vector;

/**
 * 3D cloud class.
 */
class SGNewCloud {

public:
        SGNewCloud(const SGPath &texture_root, const SGPropertyNode *cld_def, mt* s);

        ~SGNewCloud();

        // Generate a Cloud
        osg::ref_ptr<simgear::EffectGeode> genCloud ();

        static float getDensity(void)
        {
            return sprite_density;
        }
    
        // Set the sprite density
        static void setDensity(double d)
        {
            sprite_density = d;
        }
        

private:

        float min_width;
        float max_width;
        float min_height;
        float max_height;
        float min_sprite_width;
        float max_sprite_width;
        float min_sprite_height;
        float max_sprite_height;
        
        // Minimum and maximum bottom, middle, top, sunny, shade lighting
        // factors. For individual clouds we choose a bottom/middle/top
        // shade from between each min/max value
        float min_bottom_lighting_factor;
        float max_bottom_lighting_factor;
        float min_middle_lighting_factor;
        float max_middle_lighting_factor;
        float min_top_lighting_factor;
        float max_top_lighting_factor;
        float min_shade_lighting_factor;
        float max_shade_lighting_factor;
        
        // The density of the cloud is the shading applied
        // to cloud sprites on the opposite side of the cloud
        // from the sun. For an individual cloud instance a value
        // between min_density and max_density is chosen.
        float min_density;
        float max_density;
        
        // zscale indicates how sprites should be scaled vertically
        // after billboarding. 
        float zscale;
        // alpha_factor is the transparency adjustment of the clouds
        float alpha_factor;
        bool height_map_texture;
        int num_sprites;
        int num_textures_x;
        int num_textures_y;
        string texture;
        osg::Geometry* quad;
        osg::ref_ptr<simgear::Effect> effect;
        static float sprite_density;
        
        // RNG seed for this cloud
        mt* seed;

        osg::Geometry* createOrthQuad(float w, float h, int varieties_x, int varieties_y);
};



#endif // _NEWCLOUD_HXX
