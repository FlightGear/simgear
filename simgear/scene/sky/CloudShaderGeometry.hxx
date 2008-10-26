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
        
        const static unsigned int TEXTURE_INDEX_X = 11;
        const static unsigned int TEXTURE_INDEX_Y = 12;
        const static unsigned int WIDTH = 13;
        const static unsigned int HEIGHT = 14;
        const static unsigned int SHADE = 15;
        
        CloudShaderGeometry()
        { 
            setUseDisplayList(false); 
        }

        CloudShaderGeometry(int vx, int vy) :
            varieties_x(vx), varieties_y(vy)
        { 
            setUseDisplayList(false); 
        }
        
        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        CloudShaderGeometry(const CloudShaderGeometry& CloudShaderGeometry,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
            osg::Drawable(CloudShaderGeometry,copyop) {}

        META_Object(flightgear, CloudShaderGeometry);
        
        struct CloudSprite {
            CloudSprite(const SGVec3f& p, int tx, int ty, float w, float h, float s) :
                    position(p), texture_index_x(tx), texture_index_y(ty), width(w), height(h), shade(s)
                    { }
        
                    SGVec3f position;
                    int texture_index_x;
                    int texture_index_y;
                    float width;
                    float height;
                    float shade;
        };
        
        typedef std::vector<CloudSprite> CloudSpriteList;
        
        void insert(const CloudSprite& t)
        { _cloudsprites.push_back(t); }
        void insert(const SGVec3f& p, int tx, int ty, float w, float h, float s)
        { insert(CloudSprite(p, tx, ty, w, h, s)); }
        
        unsigned getNumCloudSprite() const
        { return _cloudsprites.size(); }
        const CloudSprite& getCloudSprite(unsigned i) const
        { return _cloudsprites[i]; }
        CloudSpriteList _cloudsprites;
        
        typedef std::vector<osg::Vec4> PositionSizeList;
        
        virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
        virtual osg::BoundingBox computeBound() const;
    
        
        void setGeometry(osg::Drawable* geometry)
        {
            _geometry = geometry;
        }
        
        void addSprite(const SGVec3f& p, int tx, int ty, float w, float h, float s, float cull)
        {
            // Only add the sprite if it is further than the cull distance to all other sprites
            for (CloudShaderGeometry::CloudSpriteList::const_iterator iter = _cloudsprites.begin();
                 iter != _cloudsprites.end();
                 ++iter) 
            {
                if (distSqr(iter->position, p) < cull)
                {
                    // Too close - cull it
                    return;
                }
            }

            _cloudsprites.push_back(CloudSprite(p, tx, ty, w, h, s));
        }

        osg::ref_ptr<osg::Drawable> _geometry;

        int varieties_x;
        int varieties_y;

    protected:
    
        virtual ~CloudShaderGeometry() {}
        
};

}
#endif
