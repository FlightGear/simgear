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
