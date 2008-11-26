/* -*-c++-*-
 *
 * Copyright (C) 2008 Stuart Buchanan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef CLOUD_SHADER_GEOMETRY_HXX
#define CLOUD_SHADER_GEOMETRY_HXX 1

#include <vector>

#include <osg/BoundingBox>
#include <osg/CopyOp>
#include <osg/Drawable>
#include <osg/Geometry>
#include <osg/RenderInfo>
#include <osg/Vec3>
#include <osg/Vec4>

#include <simgear/math/SGMath.hxx>


namespace simgear
{

class CloudShaderGeometry : public osg::Drawable
{
    public:
        
        const static unsigned int CLOUD_HEIGHT = 10;
        const static unsigned int TEXTURE_INDEX_X = 11;
        const static unsigned int TEXTURE_INDEX_Y = 12;
        const static unsigned int WIDTH = 13;
        const static unsigned int HEIGHT = 14;
        const static unsigned int SHADE = 15;
        
        CloudShaderGeometry()
        { 
            setUseDisplayList(false); 
            skip_info = new SkipInfo();
        }

        CloudShaderGeometry(int vx, int vy) :
            varieties_x(vx), varieties_y(vy)
        { 
            setUseDisplayList(false); 
            skip_info = new SkipInfo();
        }
        
        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        CloudShaderGeometry(const CloudShaderGeometry& CloudShaderGeometry,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
            osg::Drawable(CloudShaderGeometry,copyop) {}

        META_Object(flightgear, CloudShaderGeometry);
        
        struct SkipInfo {
            SkipInfo() : skip_count(0), skip_limit(1) {}
            int skip_count;
            int skip_limit;
        };
        
        SkipInfo* skip_info;
        
        struct CloudSprite {
            CloudSprite(SGVec3f& p, int tx, int ty, float w, float h, float s, float ch) :
                    position(p), texture_index_x(tx), texture_index_y(ty), width(w), height(h), shade(s), cloud_height(ch)
                    { }
        
                    SGVec3f position;
                    int texture_index_x;
                    int texture_index_y;
                    float width;
                    float height;
                    float shade;
                    float cloud_height;
        };
        
        typedef std::vector<CloudSprite*> CloudSpriteList;
        
        void insert(CloudSprite* t)
        { _cloudsprites.push_back(t); }
        void insert(SGVec3f& p, int tx, int ty, float w, float h, float s, float ch)
        { insert(new CloudSprite(p, tx, ty, w, h, s, ch)); }
        
        unsigned getNumCloudSprite() const
        { return _cloudsprites.size(); }
        CloudSprite* getCloudSprite(unsigned i) const
        { return _cloudsprites[i]; }
        CloudSpriteList _cloudsprites;
        
        typedef std::vector<osg::Vec4> PositionSizeList;
        
        virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
        virtual osg::BoundingBox computeBound() const;
    
        
        void setGeometry(osg::Drawable* geometry)
        {
            _geometry = geometry;
        }
        
        void addSprite(SGVec3f& p, int tx, int ty, float w, float h, float s, float cull, float cloud_height)
        {
            // Only add the sprite if it is further than the cull distance to all other sprites
            for (CloudShaderGeometry::CloudSpriteList::iterator iter = _cloudsprites.begin();
                 iter != _cloudsprites.end();
                 ++iter) 
            {
                if (distSqr((*iter)->position, p) < cull)
                {
                    // Too close - cull it
                    return;
                }
            }

            _cloudsprites.push_back(new CloudSprite(p, tx, ty, w, h, s, cloud_height));
        }
        
        osg::ref_ptr<osg::Drawable> _geometry;

        int varieties_x;
        int varieties_y;
        
    protected:
    
        virtual ~CloudShaderGeometry() {
            delete skip_info;
            for (int i = 0; i < _cloudsprites.size(); i++)
            {
                delete _cloudsprites[i];
            }
        }
};

}
#endif
