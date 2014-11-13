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
#include <osg/buffered_value>
#include <osg/Version>

#include <simgear/math/SGMath.hxx>
#include <simgear/math/sg_random.h>


namespace simgear
{

class CloudShaderGeometry : public osg::Drawable
{
    public:
        
        const static unsigned int USR_ATTR_1 = 10;
        const static unsigned int USR_ATTR_2 = 11;
        const static unsigned int USR_ATTR_3 = 12;        
                
        CloudShaderGeometry()
        { 
            setUseDisplayList(false); 
        }

        CloudShaderGeometry(int vx, int vy, float width, float height, float ts, float ms, float bs, float shade, float ch, float zsc, float af) :
            varieties_x(vx), varieties_y(vy), top_factor(ts), middle_factor(ms),
            bottom_factor(bs), shade_factor(shade), cloud_height(ch), zscale(zsc),
            alpha_factor(af)
        { 
            setUseDisplayList(false); 
            float x = width/2.0f;
            float z = height/2.0f;
            _bbox.expandBy(-x, -x, -z);
            _bbox.expandBy(x, x, z);
        }
        
        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        CloudShaderGeometry(const CloudShaderGeometry& CloudShaderGeometry,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
            osg::Drawable(CloudShaderGeometry,copyop) {}

        META_Object(flightgear, CloudShaderGeometry);
        
        struct CloudSprite {
            CloudSprite(const SGVec3f& p, int tx, int ty, float w, float h) :
                    position(p), texture_index_x(tx), texture_index_y(ty), width(w), height(h)
                    { }
        
                    SGVec3f position;
                    int texture_index_x;
                    int texture_index_y;
                    float width;
                    float height;
        };
        
        typedef std::vector<CloudSprite> CloudSpriteList;
        CloudSpriteList _cloudsprites;
        
        void insert(const CloudSprite& t)
        { _cloudsprites.push_back(t); }
        void insert(SGVec3f& p, int tx, int ty, float w, float h)
        { insert(CloudSprite(p, tx, ty, w, h)); }
        
        unsigned getNumCloudSprite() const
        { return _cloudsprites.size(); }
        CloudSprite& getCloudSprite(unsigned i)
        { return _cloudsprites[i]; }
        
        virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
        virtual osg::BoundingBox
#if OSG_VERSION_LESS_THAN(3,3,2)
        computeBound()
#else
        computeBoundingBox()
#endif
        const
        {
            return _bbox;
        }
        
        void addSprite(const SGVec3f& p, int tx, int ty, float w, float h, float cull);
        void generateGeometry();
        void rebuildGeometry();
                
        osg::ref_ptr<osg::Drawable> _geometry;

        int varieties_x;
        int varieties_y;
        float top_factor;
        float middle_factor;
        float bottom_factor;
        float shade_factor;
        float cloud_height;
        float zscale;
        float alpha_factor;
                
        // Bounding box extents.
        osg::BoundingBox _bbox;
        
    struct SortData
    {
        struct SortItem
        {
            SortItem(size_t idx_, float depth_) : idx(idx_), depth(depth_) {}
            SortItem() : idx(0), depth(0.0f) {}
            size_t idx;
            float depth;
        };
        SortData() : frameSorted(0), skip_limit(1), spriteIdx(0) {}
        int frameSorted;
        int skip_limit;
        // This will be sorted by Z distance
        typedef std::vector<SortItem> SortItemList;
        SortItemList* spriteIdx;
    };
protected:
    mutable osg::buffered_object<SortData> _sortData;
    
    virtual ~CloudShaderGeometry()
    {
        for (unsigned int i = 0; i < _sortData.size(); ++i)
            delete _sortData[i].spriteIdx;
    }
};

}
#endif
