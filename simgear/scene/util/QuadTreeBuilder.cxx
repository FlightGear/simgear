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
#if 0
QuadTreeBuilder::QuadTreeBuilder(const Vec2& min, const Vec2& max, int depth) :
    _root(new osg::Group), _min(min), _max(max), _depth(depth),
    _dimension(1 << depth), _leafStorage(_dimension * _dimension),
    _leaves(_leafStorage, _dimension)
{
    for (LeafVector::iterator iter = _leafStorage.begin();
         iter != _leafStorage.end();
         ++iter)
        *iter = new LOD;
    vector<Group*> parentNodes(1);
    parentNodes[0] = _root.get();
    unsigned leafDim = 2;
    for (int i = 0; i < depth - 1;  ++i, leafDim *= 2) {
        VectorArrayAdapter<vector<Group*> > parents(parentNodes, leafDim / 2);
        vector<Group*> interiorNodes(leafDim * leafDim);
        VectorArrayAdapter<vector<Group*> > interiors(interiorNodes, leafDim);
        for (unsigned j = 0; j < leafDim; ++j) {
            for (unsigned k = 0; k < leafDim; ++k) {
                interiors(j, k) = new Group;
                parents(j / 2, k / 2)->addChild(interiors(j, k));
            }
        }
        parentNodes.swap(interiorNodes);
    }
    VectorArrayAdapter<vector<Group*> > leafParents(parentNodes,
                                                    _dimension / 2);
    for (int j = 0; j < _dimension; ++j)
        for (int k =0; k < _dimension; ++k)
            leafParents(j / 2, k / 2)->addChild(_leaves(j, k));
}

void QuadTreeBuilder::addNode(Node* node, int lod, const Matrix& transform)
{
    Vec3 center = node->getBound().center() * transform;

    int x = (int)(_dimension * (center.x() - _min.x()) / (_max.x() - _min.x()));
    x = clampTo(x, 0, (_dimension - 1));
    int y = (int)(_dimension * (center.y() - _min.y()) / (_max.y() - _min.y()));
    y = clampTo(y, 0, (_dimension -1));
    
    _leaves(y, x)->addChild(node, 0, lod);
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
#endif
}
