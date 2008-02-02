// Copyright (C) 2008  Tim Moore
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
#ifndef SIMGEAR_QUADTREEBUILDER_HXX
#define SIMGEAR_QUADTREEBUILDER_HXX 1

#include <vector>
#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Group>
#include <osg/Matrix>
#include <osg/Vec2>
#include <osg/LOD>

#define QUAD_TREE_LEAVES 4

namespace simgear
{
typedef std::map<osg::ref_ptr<osg::Node>,int> LodMap;

// Create a quad tree based on x, y extents
class QuadTreeBuilder {
public:
    QuadTreeBuilder(const osg::Vec2& min, const osg::Vec2& max);
    ~QuadTreeBuilder() {}
    osg::Group* getRoot() { return _root.get(); }
    // Add node to the quadtree using its x, y and LoD
    void addNode(osg::Node* node, int lod, const osg::Matrix& transform);
    // Make a quadtree of nodes from a map of nodes and LOD values
    static osg::Group* makeQuadTree(LodMap& nodes,
                                    const osg::Matrix& transform);
protected:
    osg::ref_ptr<osg::Group> _root;
    osg::LOD* _leaves[QUAD_TREE_LEAVES][QUAD_TREE_LEAVES];
    osg::Vec2 _min;
    osg::Vec2 _max;
};
}
#endif
