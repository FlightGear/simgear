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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "BVHGroup.hxx"

#include <algorithm>

namespace simgear {

BVHGroup::BVHGroup()
{
}

BVHGroup::~BVHGroup()
{
    ChildList::iterator i;
    for (i = _children.begin(); i != _children.end(); ++i) {
        (*i)->removeParent(this);
        *i = 0;
    }
}

void
BVHGroup::accept(BVHVisitor& visitor)
{
    visitor.apply(*this);
}

void
BVHGroup::clear()
{
    _children.clear();
    invalidateBound();
}

void
BVHGroup::addChild(BVHNode* child)
{
    if (!child)
        return;
    ChildList::iterator i;
    i = std::find(_children.begin(), _children.end(), child);
    if (i != _children.end())
        return;
    invalidateBound();
    child->addParent(this);
    _children.push_back(child);
}

void
BVHGroup::removeChild(BVHNode* child)
{
    if (!child)
        return;
    ChildList::iterator i;
    i = std::find(_children.begin(), _children.end(), child);
    if (i == _children.end())
        return;
    invalidateBound();
    child->removeParent(this);
    _children.erase(i);
}

SGSphered
BVHGroup::computeBoundingSphere() const
{
    SGSphered sphere;
    ChildList::const_iterator i;
    for (i = _children.begin(); i != _children.end(); ++i)
        sphere.expandBy((*i)->getBoundingSphere());
    return sphere;
}

}
