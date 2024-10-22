// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#include "BVHNode.hxx"

#include <algorithm>
#include <simgear/structure/SGAtomic.hxx>
#include <simgear/math/SGGeometry.hxx>

namespace simgear {

BVHNode::BVHNode() :
    _dirtyBoundingSphere(true)
{
}

BVHNode::~BVHNode()
{
}

BVHNode::Id
BVHNode::getNewId()
{
    static SGAtomic id(0);
    return ++id;
}

void
BVHNode::addParent(BVHNode* parent)
{
    // should not happen, but be paranoid ...
    ParentList::iterator i;
    i = std::find(_parents.begin(), _parents.end(), parent);
    if (i != _parents.end())
        return;
    // add to the parents list ...
    _parents.push_back(parent);
}

void
BVHNode::removeParent(BVHNode* parent)
{
    ParentList::iterator i;
    i = std::find(_parents.begin(), _parents.end(), parent);
    if (i == _parents.end())
        return;
    _parents.erase(i);
}

void
BVHNode::invalidateParentBound()
{
    for (ParentList::iterator i = _parents.begin(); i != _parents.end(); ++i)
        (*i)->invalidateBound();
}

void
BVHNode::invalidateBound()
{
    if (_dirtyBoundingSphere)
        return;
    invalidateParentBound();
    _dirtyBoundingSphere = true;
}

}
