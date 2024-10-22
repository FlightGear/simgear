// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#include "BVHStaticGeometry.hxx"

#include "BVHBoundingBoxVisitor.hxx"

namespace simgear {

BVHStaticGeometry::BVHStaticGeometry(const BVHStaticNode* staticNode,
                                     const BVHStaticData* staticData) :
    _staticNode(staticNode),
    _staticData(staticData)
{
}

BVHStaticGeometry::~BVHStaticGeometry()
{
}

void
BVHStaticGeometry::accept(BVHVisitor& visitor)
{
    visitor.apply(*this);
}

SGSphered
BVHStaticGeometry::computeBoundingSphere() const
{
    BVHBoundingBoxVisitor bbv;
    _staticNode->accept(bbv, *_staticData);
    SGSphered sphere;
    sphere.expandBy(SGBoxd(bbv.getBox()));
    return sphere;
}

}
