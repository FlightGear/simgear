// VPBTileBounds.hxx -- VirtualPlanetBuilder Tile bounds for clipping
//
// Copyright (C) 2021 Stuart Buchanan
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

#ifndef VPBTILEBOUNDS
#define VPBTILEBOUNDS 1

#include <osg/Geometry>
#include <osgTerrain/TerrainTile>

using namespace osgTerrain;


class TileBounds {
    public:
        TileBounds(Locator *locator, osg::Vec3d up);
        virtual std::list<osg::Vec3d> clipToTile(std::list<osg::Vec3d> points);
        virtual bool insideTile(osg::Vec3d pt);
    
    protected:
        virtual osg::Vec3d getTileIntersection(osg::Vec3d inside, osg::Vec3d outside);
        virtual osg::Vec3d getPlaneIntersection(osg::Vec3d inside, osg::Vec3d outside, osg::Vec3d normal, osg::Vec3d plane);

        // Corners of the tile
        osg::Vec3d v00, v01, v10, v11;

        // Plane normals for the tile bounds
        osg::Vec3d north, east, south, west;
};

#endif
