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

#include <algorithm>
#include <functional>
#include <vector>
#include <osg/BoundingBox>
#include <osg/ref_ptr>
#include <osg/Node>
#include <osg/Group>
#include <osg/Matrix>
#include <osg/Vec2>
#include <osg/LOD>
#include "VectorArrayAdapter.hxx"

namespace simgear
{
typedef std::map<osg::ref_ptr<osg::Node>,int> LodMap;

// Create a quad tree based on x, y extents
template <typename LeafType, typename ObjectType, typename MakeLeaf,
          typename AddLeafObject, typename GetObjectLocalCoords>
class QuadTreeBuilder {
public:
    QuadTreeBuilder(const GetObjectLocalCoords& getLocalCoords,
                    const AddLeafObject& addLeafObject, int depth = 2,
                    const MakeLeaf& makeLeaf = MakeLeaf()) :
        _root(new osg::Group), _depth(depth),
        _dimension(1 << depth), _leafStorage(_dimension * _dimension),
        _leaves(_leafStorage, _dimension),
        _leafParents(_leafParentStorage, _dimension / 2),
        _getLocalCoords(getLocalCoords),
        _addLeafObject(addLeafObject), _makeLeaf(makeLeaf)
    {
        using namespace std;
        using namespace osg;
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
        // Save leaf parents for later when we add leaves
        _leafParentStorage = parentNodes;
    }
    osg::Vec2 getMin() { return _min; }
    void setMin(const osg::Vec2& min) { _min = min; }
    osg::Vec2 getMax() { return _max; }
    void setMax(const osg::Vec2& max) { _max = max; }
    ~QuadTreeBuilder() {}
    osg::Group* getRoot() { return _root.get(); }

    void addNode(ObjectType& obj)
    {
        using namespace osg;
        const Vec3 center(_getLocalCoords(obj));
        int x = 0;
        if (_max.x() != _min.x())
            x = (int)(_dimension * (center.x() - _min.x())
                      / (_max.x() - _min.x()));
        x = clampTo(x, 0, (_dimension - 1));
        int y = 0;
        if (_max.y() != _min.y())
            y = (int)(_dimension * (center.y() - _min.y())
                      / (_max.y() - _min.y()));
        y = clampTo(y, 0, (_dimension -1));
        if (!_leaves(y, x)) {
            _leaves(y, x) = _makeLeaf();
            _leafParents(y / 2, x / 2)->addChild(_leaves(y, x));
        }
        _addLeafObject(_leaves(y, x), obj);
    }
    // STL craziness
    struct AddNode
    {
        AddNode(QuadTreeBuilder* qt) : _qt(qt) {}
        AddNode(const AddNode& rhs) : _qt(rhs._qt) {}
        void operator() (ObjectType& obj) const { _qt->addNode(obj); }
        QuadTreeBuilder *_qt;
    };
    // Make a quadtree of nodes from a map of nodes and LOD values
    template <typename ForwardIterator>
    void buildQuadTree(const ForwardIterator& begin,
                       const ForwardIterator& end)
    {
        using namespace osg;
        BoundingBox extents;
        for (ForwardIterator iter = begin; iter != end; ++iter) {
            const Vec3 center = _getLocalCoords(*iter);
            extents.expandBy(center);
        }
        _min = Vec2(extents.xMin(), extents.yMin());
        _max = Vec2(extents.xMax(), extents.yMax());
        std::for_each(begin, end, AddNode(this));
    }

protected:
    typedef std::vector<LeafType> LeafVector;
    osg::ref_ptr<osg::Group> _root;
    osg::Vec2 _min;
    osg::Vec2 _max;
    int _depth;
    int _dimension;
    LeafVector _leafStorage;
    VectorArrayAdapter<LeafVector> _leaves;
    std::vector<osg::Group*> _leafParentStorage;
    VectorArrayAdapter<std::vector<osg::Group*> > _leafParents;
    const GetObjectLocalCoords _getLocalCoords;
    const AddLeafObject _addLeafObject;
    const MakeLeaf _makeLeaf;
};

}
#endif
