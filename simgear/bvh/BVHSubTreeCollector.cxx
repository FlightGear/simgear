// Copyright (C) 2008 - 2009  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "BVHSubTreeCollector.hxx"

#include <simgear/math/SGGeometry.hxx>
#include <cassert>

#include "BVHNode.hxx"
#include "BVHGroup.hxx"
#include "BVHTransform.hxx"

#include "BVHStaticData.hxx"

#include "BVHStaticNode.hxx"
#include "BVHStaticTriangle.hxx"
#include "BVHStaticBinary.hxx"
#include "BVHStaticGeometry.hxx"
#include "BVHBoundingBoxVisitor.hxx"

namespace simgear {

BVHSubTreeCollector::BVHSubTreeCollector(const SGSphered& sphere) :
    _sphere(sphere)
{
}

BVHSubTreeCollector::~BVHSubTreeCollector()
{
}

void
BVHSubTreeCollector::apply(BVHGroup& group)
{
    if (!intersects(_sphere, group.getBoundingSphere()))
        return;

    // The _nodeList content is somehow the 'return value' of the subtree.
    // Set it to zero to see if we have something to collect down there.
    NodeList parentNodeList;
    pushNodeList(parentNodeList);

    group.traverse(*this);
    
    popNodeList(parentNodeList);
}

void
BVHSubTreeCollector::apply(BVHPageNode& group)
{
    if (!intersects(_sphere, group.getBoundingSphere()))
        return;

    // The _nodeList content is somehow the 'return value' of the subtree.
    // Set it to zero to see if we have something to collect down there.
    NodeList parentNodeList;
    pushNodeList(parentNodeList);

    group.traverse(*this);
    
    popNodeList(parentNodeList);
}

void
BVHSubTreeCollector::apply(BVHTransform& transform)
{
    if (!intersects(_sphere, transform.getBoundingSphere()))
        return;
    
    SGSphered sphere = _sphere;
    _sphere = transform.sphereToLocal(sphere);
    
    NodeList parentNodeList;
    pushNodeList(parentNodeList);
    
    transform.traverse(*this);

    if (haveChildren()) {
        BVHTransform* currentBvTransform = new BVHTransform;
        currentBvTransform->setTransform(transform);
        popNodeList(parentNodeList, currentBvTransform);
    } else {
        popNodeList(parentNodeList);
    }

    _sphere = sphere;
}

void
BVHSubTreeCollector::apply(BVHMotionTransform& transform)
{
    if (!intersects(_sphere, transform.getBoundingSphere()))
        return;

    SGSphered sphere = _sphere;
    _sphere = transform.sphereToLocal(sphere, transform.getStartTime());
    _sphere.expandBy(transform.sphereToLocal(sphere, transform.getEndTime()));
    
    NodeList parentNodeList;
    pushNodeList(parentNodeList);

    transform.traverse(*this);

    if (haveChildren()) {
        BVHMotionTransform* currentBvTransform = new BVHMotionTransform;
        currentBvTransform->setTransform(transform);
        popNodeList(parentNodeList, currentBvTransform);
    } else {
        popNodeList(parentNodeList);
    }
    
    _sphere = sphere;
}

void
BVHSubTreeCollector::apply(BVHLineGeometry& lineSegment)
{
    if (!intersects(_sphere, lineSegment.getBoundingSphere()))
        return;
    addNode(&lineSegment);
}

void
BVHSubTreeCollector::apply(BVHStaticGeometry& node)
{
    if (!intersects(_sphere, node.getBoundingSphere()))
        return;

    assert(!_staticNode);
    node.traverse(*this);
    if (!_staticNode)
        return;
    
    BVHStaticGeometry* staticTree;
    staticTree = new BVHStaticGeometry(_staticNode, node.getStaticData());
    addNode(staticTree);
    _staticNode = 0;
}

void
BVHSubTreeCollector::apply(const BVHStaticBinary& node,
                           const BVHStaticData& data)
{
    assert(!_staticNode);
    
    if (!intersects(_sphere, node.getBoundingBox()))
        return;
    
    SGVec3d corner(node.getBoundingBox().getFarestCorner(_sphere.getCenter()));
    if (intersects(_sphere, corner)) {
        // If the box is totally contained in the sphere, just take it all
        _staticNode = &node;
        
    } else {
        // We have still a chance to seperate something out, try it.
        
        node.getLeftChild()->accept(*this, data);
        SGSharedPtr<const BVHStaticNode> leftStaticNode = _staticNode;
        _staticNode = 0;
        node.getRightChild()->accept(*this, data);
        SGSharedPtr<const BVHStaticNode> rightStaticNode = _staticNode;
        _staticNode = 0;
        
        if (leftStaticNode) {
            if (rightStaticNode) {
                BVHBoundingBoxVisitor bbv;
                leftStaticNode->accept(bbv, data);
                rightStaticNode->accept(bbv, data);
                _staticNode
                    = new BVHStaticBinary(node.getSplitAxis(), leftStaticNode,
                                          rightStaticNode, bbv.getBox());
            } else {
                _staticNode = leftStaticNode;
            }
        } else {
            if (rightStaticNode) {
                _staticNode = rightStaticNode;
            } else {
                // Nothing to report to parents ...
            }
        }
    }
}

void
BVHSubTreeCollector::apply(const BVHStaticTriangle& node,
                           const BVHStaticData& data)
{
    if (!intersects(_sphere, node.getTriangle(data)))
        return;
    _staticNode = &node;
}

void
BVHSubTreeCollector::addNode(BVHNode* node)
{
    if (!node)
        return;
    if (!_nodeList.capacity())
        _nodeList.reserve(64);
    _nodeList.push_back(node);
}
    
void
BVHSubTreeCollector::popNodeList(NodeList& parentNodeList, BVHGroup* transform)
{
    // Only do something if we really have children
    if (!_nodeList.empty()) {
        NodeList::const_iterator i;
        for (i = _nodeList.begin(); i != _nodeList.end(); ++i)
            transform->addChild(*i);
        parentNodeList.push_back(transform);
    }
    _nodeList.swap(parentNodeList);
}
    
void
BVHSubTreeCollector::popNodeList(NodeList& parentNodeList)
{
    // Only do something if we really have children
    if (!_nodeList.empty()) {
        if (_nodeList.size() == 1) {
            parentNodeList.push_back(_nodeList.front());
        } else {
            BVHGroup* group = new BVHGroup;
            NodeList::const_iterator i;
            for (i = _nodeList.begin(); i != _nodeList.end(); ++i)
                group->addChild(*i);
            parentNodeList.push_back(group);
        }
    }
    _nodeList.swap(parentNodeList);
}

SGSharedPtr<BVHNode>
BVHSubTreeCollector::getNode() const
{
    if (_nodeList.empty())
        return 0;
    
    if (_nodeList.size() == 1)
        return _nodeList.front();
    
    BVHGroup* group = new BVHGroup;
    NodeList::const_iterator i;
    for (i = _nodeList.begin(); i != _nodeList.end(); ++i)
        group->addChild(*i);
    return group;
}

}
