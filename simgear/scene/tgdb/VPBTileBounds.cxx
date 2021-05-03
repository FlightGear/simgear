// VPBTileBounds.cxx -- VirtualPlanetBuilder Tile bounds for clipping
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

#include "VPBTileBounds.hxx"

#include <simgear/debug/logstream.hxx>


TileBounds::TileBounds(Locator *locator, osg::Vec3d up) {
    // Determine the corners of the tile;
    locator->convertLocalToModel(osg::Vec3d(0.0, 0.0, 0.0), v00);
    locator->convertLocalToModel(osg::Vec3d(1.0, 0.0, 0.0), v10);
    locator->convertLocalToModel(osg::Vec3d(0.0, 1.0, 0.0), v01);
    locator->convertLocalToModel(osg::Vec3d(1.0, 1.0, 0.0), v11);

    corner[0] = v10;
    corner[1] = v11;
    corner[2] = v01;
    corner[3] = v00;

    // Determine the normals of the planes defining the vertical edges of the tiles.
    // This can be found using the cross product of the horizontal line and an appropriate "up"
    // vector.  We will approximate the last using the center rather than working out different
    // "up" vectors for each edge individually.  We assume that (0,0) in local coords is bottom left.
    south = (v10 - v00) ^ up;
    east  = (v11 - v10) ^ up;
    north = (v01 - v11) ^ up;
    west  = (v00 - v01) ^ up;
}

std::list<osg::Vec3d> TileBounds::clipToTile(std::list<osg::Vec3d> points) {

    std::list<osg::Vec3d> lreturn;

    bool last_in = false;
    auto last_pt = points.begin();

    for (auto p = points.begin(); p != points.end(); p++) {
        if (insideTile(*p)) {

/*
            if ((last_in == false) && (last_pt != p)) {
                // Last point was outside the border of the tile, so we need to 
                // add the actual intersection as an additional point if required.
                lreturn.push_back(getTileIntersection(*p, *last_pt));
            }
*/            

            // Last point wasn't in, but we need to include it to ensure we get an
            // intersection with the edge of the tile.
            if (last_in == false) lreturn.push_back(*last_pt);

            // Point is within bounds, so add it.
            lreturn.push_back(*p);

            last_in = true;
        } else if (last_in) {
            // Point is outside bounds, but last point was inside.  So add it to ensure
            // we catch the intersection with the mesh edge.
            lreturn.push_back(*p);
            last_in = false;
        }

        last_pt = p;
    }

    return lreturn;
}

bool TileBounds::insideTile(osg::Vec3d pt) {

    float s = south * (pt - v00);
    float e = east  * (pt - v10);
    float n = north * (pt - v11);
    float w = west  * (pt - v01);

    return ((s<0) && (e<0) && (n<0) && (w<0));
}

// Get the Tile intersection, and also the corner of the tile to the right of the intersection.
// This corner can be used to define the seaward edge of some coastline.
osg::Vec3d TileBounds::getTileIntersection(osg::Vec3d inside, osg::Vec3d outside) {

    if (! insideTile(inside)) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Invalid VPB Tile intersection - \"inside\" point not inside!");
        return inside;
    }

    if (insideTile(outside)) {
        SG_LOG(SG_TERRAIN, SG_ALERT, "Invalid VPB Tile intersection - \"outside\" point not outside!");
        return outside;
    }

    // Simply clip against each of the planes in turn.
    osg::Vec3d intersect = outside;
    intersect = getPlaneIntersection(inside, intersect, south, v00);
    intersect = getPlaneIntersection(inside, intersect, east,  v10);
    intersect = getPlaneIntersection(inside, intersect, north, v11);
    intersect = getPlaneIntersection(inside, intersect, west,  v01);

    return intersect;
}

osg::Vec3d TileBounds::getPlaneIntersection(osg::Vec3d inside, osg::Vec3d outside, osg::Vec3d normal, osg::Vec3d plane) {
    // Check for any intersection at all first
    osg::Vec3d n  = normal;
    osg::Vec3d p0 = plane;
    osg::Vec3d l0 = inside;
    osg::Vec3d l  = outside-inside;

    if (fabs(l * n) < 0.01) return outside;

    float d = ((p0 - l0) * n) / (l * n);
    return l0 + l * d;


/*
    float i = normal * (outside - plane);

    if (i < 0) {
        // There's an intersection, so calculate it
        double d = ((plane - inside) * normal) / ((outside - inside) * normal);
        return inside + (outside - inside) * d;
    } else {
        // No intersection
        return outside;
    }
*/    
}


