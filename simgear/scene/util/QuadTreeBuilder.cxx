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

#include "QuadTreeBuilder.hxx"

using namespace std;
using namespace osg;

namespace simgear
{
QuadTreeBuilder::QuadTreeBuilder(const Vec2& min, const Vec2& max) :
    _root(new osg::Group), _min(min), _max(max)
{
    for (int i = 0; i < 4; ++i) {
        Group* interior = new osg::Group;
        _root->addChild(interior);
        for (int j = 0; j < 4; ++j) {
            Group* leaf = new osg::Group;
            interior->addChild(leaf);
            _leaves[i][j] = leaf;
        }
    }
}

void QuadTreeBuilder::addNode(Node* node, const Matrix& transform)
{
    Vec3 center = node->getBound().center() * transform;

    int x = (int)(4.0 * (center.x() - _min.x()) / (_max.x() - _min.x()));
    x = clampTo(x, 0, 3);
    int y = (int)(4.0 * (center.y() - _min.y()) / (_max.y() - _min.y()));
    y = clampTo(y, 0, 3);
    _leaves[y][x]->addChild(node);
}

osg::Group* QuadTreeBuilder::makeQuadTree(vector<ref_ptr<Node> >& nodes,
                                          const Matrix& transform)
{
    typedef vector<ref_ptr<Node> > NodeList;
    BoundingBox extents;
    for (NodeList::iterator iter = nodes.begin(); iter != nodes.end(); ++iter) {
        const Vec3 center = (*iter)->getBound().center() * transform;
        extents.expandBy(center);
    }
    const Vec2 quadMin(extents.xMin(), extents.yMin());
    const Vec2 quadMax(extents.xMax(), extents.yMax());
    ref_ptr<Group> result;
    {
        QuadTreeBuilder quadTree(quadMin, quadMax);
        for (NodeList::iterator iter = nodes.begin();
             iter != nodes.end();
             ++iter) {
            quadTree.addNode(iter->get(), transform);
        }
        result = quadTree.getRoot();
    }
    return result.release();
}

}
