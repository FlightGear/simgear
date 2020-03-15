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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <osg/AlphaFunc>
#include <osg/Depth>
#include <osg/Program>
#include <osg/Uniform>
#include <osg/ref_ptr>
#include <osg/Texture2D>
#include <osg/NodeVisitor>
#include <osg/PositionAttitudeTransform>
#include <osg/Material>
#include <osgUtil/UpdateVisitor>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>


#include <simgear/compiler.h>

#include <simgear/misc/sg_path.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/model/model.hxx>
#include <simgear/scene/util/SGReaderWriterOptions.hxx>
#include <simgear/scene/util/StateAttributeFactory.hxx>
#include <simgear/scene/util/SGUpdateVisitor.hxx>
#include <simgear/scene/util/RenderConstants.hxx>

#include <algorithm>
#include <osg/BlendFunc>
#include <osg/GLU>
#include <osg/ShadeModel>

#include "cloudfield.hxx"
#include "newcloud.hxx"
#include "CloudShaderGeometry.hxx"

using namespace simgear;
using namespace osg;
using namespace std;


namespace
{
typedef std::map<std::string, osg::observer_ptr<Effect> > EffectMap;
EffectMap effectMap;
}

float SGNewCloud::sprite_density = 1.0;

SGNewCloud::SGNewCloud(const SGPath &texture_root, const SGPropertyNode *cld_def, mt* s)
{
    // Set up the RNG with the passed in seed. This allows us to make the RNG repeatable
    // if required.
    seed = s;
  
    min_width = cld_def->getFloatValue("min-cloud-width-m", 500.0);
    max_width = cld_def->getFloatValue("max-cloud-width-m", min_width*2);
    min_height = cld_def->getFloatValue("min-cloud-height-m", 400.0);
    max_height = cld_def->getFloatValue("max-cloud-height-m", min_height*2);
    min_sprite_width = cld_def->getFloatValue("min-sprite-width-m", 200.0);
    max_sprite_width = cld_def->getFloatValue("max-sprite-width-m", min_sprite_width*1.5);
    min_sprite_height = cld_def->getFloatValue("min-sprite-height-m", 150);
    max_sprite_height = cld_def->getFloatValue("max-sprite-height-m", min_sprite_height*1.5);
    num_sprites = cld_def->getIntValue("num-sprites", 20);
    num_textures_x = cld_def->getIntValue("num-textures-x", 4);
    num_textures_y = cld_def->getIntValue("num-textures-y", 4);
    height_map_texture = cld_def->getBoolValue("height-map-texture", false);
    
    min_bottom_lighting_factor = cld_def->getFloatValue("min-bottom-lighting-factor", 1.0);
    max_bottom_lighting_factor = cld_def->getFloatValue("max-bottom-lighting-factor", min(min_bottom_lighting_factor  + 0.1, 1.0));
    
    min_middle_lighting_factor = cld_def->getFloatValue("min-middle-lighting-factor", 1.0);
    max_middle_lighting_factor = cld_def->getFloatValue("max-middle-lighting-factor", min(min_middle_lighting_factor  + 0.1, 1.0));

    min_top_lighting_factor = cld_def->getFloatValue("min-top-lighting-factor", 1.0);
    max_top_lighting_factor = cld_def->getFloatValue("max-top-lighting-factor", min(min_top_lighting_factor  + 0.1, 1.0));

    min_shade_lighting_factor = cld_def->getFloatValue("min-shade-lighting-factor", 0.5);
    max_shade_lighting_factor = cld_def->getFloatValue("max-shade-lighting-factor", min(min_shade_lighting_factor  + 0.1, 1.0));
    
    zscale = cld_def->getFloatValue("z-scale", 1.0);
    alpha_factor = cld_def->getFloatValue("alpha-factor",1.0);
    texture = cld_def->getStringValue("texture", "cl_cumulus.png");

    // Create a new Effect for the texture, if required.
    EffectMap::iterator iter = effectMap.find(texture);

    if ((iter == effectMap.end())||
        (!iter->second.lock(effect)))
    {
        SGPropertyNode_ptr pcloudEffect = new SGPropertyNode;
        makeChild(pcloudEffect, "inherits-from")->setValue("Effects/cloud");
        setValue(makeChild(makeChild(makeChild(pcloudEffect, "parameters"),
                                     "texture"),
                           "image"),
                 texture);
        ref_ptr<SGReaderWriterOptions> options;
        options = SGReaderWriterOptions::fromPath(texture_root);
        effect = makeEffect(pcloudEffect, true, options.get());
        if (effect.valid())
        {
            if (iter == effectMap.end())
                effectMap.insert(EffectMap::value_type(texture, effect));
            else
                iter->second = effect; // update existing, but empty observer
        }
    }
}

SGNewCloud::~SGNewCloud() {
}

#if 0
// return a random number between -n/2 and n/2, tending to 0
static float Rnd(float n) {
    return n * (-0.5f + (mt_rand(seed) + mt_rand(seed)) / 2.0f);
}
#endif

osg::ref_ptr<EffectGeode> SGNewCloud::genCloud() {
    
    osg::ref_ptr<EffectGeode> geode = new EffectGeode;
        
    // Determine how big this specific cloud instance is. Note that we subtract
    // the sprite size because the width/height is used to define the limits of
    // the center of the sprites, not their edges.
    float width = min_width + mt_rand(seed) * (max_width - min_width) - min_sprite_width;
    float height = min_height + mt_rand(seed) * (max_height - min_height) - min_sprite_height;
    
    if (width  < 0.0) { width  = 0.0; }
    
    // Protect against divide by 0 issues later when assigning index_y
    if (height <= 0.0) { height = 0.01; }

    // Determine appropriate shading factors
    float top_factor = min_top_lighting_factor + mt_rand(seed) * (max_top_lighting_factor - min_top_lighting_factor);
    float middle_factor = min_middle_lighting_factor + mt_rand(seed) * (max_middle_lighting_factor - min_middle_lighting_factor);
    float bottom_factor = min_bottom_lighting_factor + mt_rand(seed) * (max_bottom_lighting_factor - min_bottom_lighting_factor);
    float shade_factor = min_shade_lighting_factor + mt_rand(seed) * (max_shade_lighting_factor - min_shade_lighting_factor);
    
    //printf("Cloud: %2f, %2f, %2f, %2f\n", top_factor, middle_factor, bottom_factor, shade_factor); 
    
    CloudShaderGeometry* sg = new CloudShaderGeometry(num_textures_x, 
                                                      num_textures_y, 
                                                      max_width + max_sprite_width, 
                                                      max_height + max_sprite_height, 
                                                      top_factor,
                                                      middle_factor,
                                                      bottom_factor,
                                                      shade_factor,
                                                      height,
                                                      zscale,
                                                      alpha_factor);
        
    // Determine the cull distance. This is used to remove sprites that are too close together.
    // The value is squared as we use vector calculations.
    float cull_distance_squared = min_sprite_height * min_sprite_height * 0.1f;
    
    // The number of sprites we actually use is a function of the (user-controlled) density
    int n_sprites = num_sprites * sprite_density * (0.5f + mt_rand(seed));
    
    for (int i = 0; i < n_sprites; i++)
    {
        // Determine the position of the sprite. Rather than being completely random,
        // we place them on the surface of a distorted sphere. However, we place
        // the first sprite in the center of the sphere (and at maximum size) to
	      // ensure good coverage and reduce the chance of there being "holes" in the
	      // middle of our cloud. Also note that (0,0,0) defines the _bottom_ of the
	      // cloud, not the middle. 

        float x, y, z;

        if (i == 0) {
            x = 0;
            y = 0;
            z = height * 0.5;
        } else {
            float theta = mt_rand(seed) * SGD_2PI;
            float elev  = mt_rand(seed) * SGD_PI;
            x = width * cos(theta) * 0.5f * sin(elev);
            y = width * sin(theta) * 0.5f * sin(elev);
            z = height * cos(elev) * 0.5f + height * 0.5f;
        }
        
        // Determine the height and width
        float sprite_width = min_sprite_width + mt_rand(seed) * (max_sprite_width - min_sprite_width);
        float sprite_height = min_sprite_height + mt_rand(seed) * (max_sprite_height - min_sprite_height);
        
        // Sprites are never taller than square.
        if (sprite_height > sprite_width )
        {
            sprite_height = sprite_width;
        }

        if (i == 0) {
            // The center sprite is always maximum size to fill up any holes.
            sprite_width = max_sprite_width;
            sprite_height = max_sprite_height;
        }
        
        // If the center of the sprite is less than half the sprite heightthe sprite will extend 
        // below the bottom of the cloud and must be shifted upwards. This is particularly important 
        // for cumulus clouds which have a very well defined base.
        if (z < 0.5f * sprite_height)
        {
            z = 0.5f * sprite_height;
        }        

        // Determine the sprite texture indexes.
        int index_x = (int) floor(mt_rand(seed) * num_textures_x);
        if (index_x >= num_textures_x) { index_x = num_textures_x - 1; }
                
        int index_y = (int) floor(mt_rand(seed) * num_textures_y);
        
        if (height_map_texture) {
          // The y index depends on the position of the sprite within the cloud.
          // This allows cloud designers to have particular sprites for the base
          // and tops of the cloud.
          index_y = (int) floor((z / height) * num_textures_y);
        }
        
        if (index_y >= num_textures_y) { index_y = num_textures_y - 1; }
        
        sg->addSprite(SGVec3f(x, y, z), 
                      index_x, 
                      index_y, 
                      sprite_width, 
                      sprite_height, 
                      cull_distance_squared);
    }
    
    sg->generateGeometry();
    geode->addDrawable(sg);
    geode->setName("3D cloud");
    geode->setEffect(effect.get());
    geode->setNodeMask( ~(simgear::CASTSHADOW_BIT | simgear::MODELLIGHT_BIT) );
    
    return geode;
}

