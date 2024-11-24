// VPBBufferData.hxx -- Data Structure for VPB
//
// Copyright (C) 2024 Stuart Buchanan
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

#ifndef VPBBUFFERDATA
#define VPBBUFFERDATA 1

#include <osg/MatrixTransform>
#include <osg/Geode>
#include <osg/Geometry>
#include <osgTerrain/TerrainTile>
#include <osgTerrain/Terrain>

#include <simgear/scene/material/EffectGeode.hxx>

namespace simgear {

class BufferData : public osg::Referenced
{
public:
    BufferData() : _transform(0), _landGeode(0), _landGeometry(0), _lineFeatures(0), _width(0.0), _height(0.0)            
    {}

    osg::ref_ptr<osg::MatrixTransform>  _transform;
    osg::ref_ptr<simgear::EffectGeode>  _landGeode;
    osg::ref_ptr<simgear::EffectGeode>  _seaGeode;
    osg::ref_ptr<osg::Geometry>         _landGeometry;
    osg::ref_ptr<osg::Geometry>         _seaGeometry;
    osg::ref_ptr<osg::Group>            _lineFeatures;
    float                               _width;
    float                               _height;
    Atlas::AtlasMap                     _BVHMaterialMap;
    osg::ref_ptr<osgTerrain::Locator>   _masterLocator;
    osg::ref_ptr<osg::Texture2D>        _waterRasterTexture;

protected:
    virtual ~BufferData() {}
};

};

#endif