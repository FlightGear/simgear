// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

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
