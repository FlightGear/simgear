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

#ifndef SHADER_GEOMETRY_HXX
#define SHADER_GEOMETRY_HXX 1

#include <vector>

#include <osg/BoundingBox>
#include <osg/CopyOp>
#include <osg/Drawable>
#include <osg/Geometry>
#include <osg/RenderInfo>
#include <osg/Vec3>
#include <osg/Vec4>

#include "TreeBin.hxx"

namespace simgear
{

class ShaderGeometry : public osg::Geometry
{
    public:
        ShaderGeometry() :
          varieties(1)
        { 
        }

        ShaderGeometry(int v) :
          varieties(v)
        { 
        }
        
        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        ShaderGeometry(const ShaderGeometry& ShaderGeometry,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
            osg::Geometry(ShaderGeometry,copyop) {}

        META_Object(flightgear, ShaderGeometry);

        virtual void drawImplementation(osg::RenderInfo& renderInfo) const;
        virtual osg::BoundingBox computeBound() const;
        
        void setGeometry(osg::Geometry* geometry)
        {
            _geometry = geometry;
        }
        
        void addTree(const TreeBin::Tree& tree);

        osg::ref_ptr<osg::Geometry> _geometry;

        int varieties;

    protected:
    
        virtual ~ShaderGeometry() {}
        
};

}
#endif
