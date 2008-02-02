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

#include <osg/BoundingBox>
#include <osg/Math>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include "QuadTreeBuilder.hxx"

using namespace std;
using namespace osg;

namespace simgear
{
QuadTreeBuilder::QuadTreeBuilder(const Vec2& min, const Vec2& max) :
    _root(new osg::Group), _min(min), _max(max)
{
    for (int i = 0; i < QUAD_TREE_LEAVES; ++i) {
        Group* interior = new osg::Group;
        _root->addChild(interior);
        for (int j = 0; j < QUAD_TREE_LEAVES; ++j) {
            LOD* lod  = new osg::LOD;
            interior->addChild(lod);
            _leaves[i][j] = lod;
        }
    }
}

void QuadTreeBuilder::addNode(Node* node, int lod, const Matrix& transform)
{
    Vec3 center = node->getBound().center() * transform;

    int x = (int)(QUAD_TREE_LEAVES * (center.x() - _min.x()) / (_max.x() - _min.x()));
    x = clampTo(x, 0, (QUAD_TREE_LEAVES - 1));
    int y = (int)(QUAD_TREE_LEAVES * (center.y() - _min.y()) / (_max.y() - _min.y()));
    y = clampTo(y, 0, (QUAD_TREE_LEAVES -1));
    
    _leaves[y][x]->addChild(node, 0, lod);
}

osg::Group* QuadTreeBuilder::makeQuadTree(LodMap& models,
                                          const Matrix& transform)
{
    typedef vector<ref_ptr<Node> > NodeList;
    BoundingBox extents;
    for (LodMap::iterator iter = models.begin(); iter != models.end(); ++iter) {
        const Vec3 center = (*iter).first->getBound().center() * transform;
        extents.expandBy(center);
    }
    const Vec2 quadMin(extents.xMin(), extents.yMin());
    const Vec2 quadMax(extents.xMax(), extents.yMax());
    ref_ptr<Group> result;
    {
        QuadTreeBuilder quadTree(quadMin, quadMax);
        for (LodMap::iterator iter = models.begin();
             iter != models.end();
             ++iter) {
            quadTree.addNode(iter->first.get(), iter->second, transform);
        }
        result = quadTree.getRoot();
    }
    return result.release();
}

}
