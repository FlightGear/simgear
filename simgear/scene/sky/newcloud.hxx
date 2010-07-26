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

#include <simgear/scene/material/Effect.hxx>
#include <simgear/scene/material/EffectGeode.hxx>

using std::string;
using std::vector;

/**
 * 3D cloud class.
 */
class SGNewCloud {

public:
        SGNewCloud(string type,
                   const SGPath &tex_path, 
                    string tex,
                    double min_w,
                    double max_w,
                    double min_h,
                    double max_h,
                    double min_sprite_w,
                    double max_sprite_w,
                    double min_sprite_h,
                    double max_sprite_h,
                    double b,
                    int n,
                    int nt_x,
                    int nt_y);

        ~SGNewCloud();

        // Generate a Cloud
        osg::ref_ptr<simgear::EffectGeode> genCloud ();

        static double getDensity(void)
        {
            return sprite_density;
        }
    
        // Set the sprite density
        static void setDensity(double d)
        {
            sprite_density = d;
        }
        

private:

        double min_width;
        double max_width;
        double min_height;
        double max_height;
        double min_sprite_width;
        double max_sprite_width;
        double min_sprite_height;
        double max_sprite_height;
        double bottom_shade;
        int num_sprites;
        int num_textures_x;
        int num_textures_y;
        const string texture;
        const string name;
        osg::Geometry* quad;
        osg::ref_ptr<simgear::Effect> effect;
        static double sprite_density;

        osg::Geometry* createOrthQuad(float w, float h, int varieties_x, int varieties_y);

};



#endif // _NEWCLOUD_HXX
