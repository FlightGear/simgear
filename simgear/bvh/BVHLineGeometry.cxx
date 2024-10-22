// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#include "BVHLineGeometry.hxx"

#include "BVHVisitor.hxx"

namespace simgear {

BVHLineGeometry::BVHLineGeometry(const SGLineSegmentf& lineSegment, Type type) :
    _lineSegment(lineSegment),
    _type(type)
{
}

BVHLineGeometry::~BVHLineGeometry()
{
}

void
BVHLineGeometry::accept(BVHVisitor& visitor)
{
    visitor.apply(*this);
}

SGSphered
BVHLineGeometry::computeBoundingSphere() const
{
    SGSphered sphere;
    sphere.expandBy(SGVec3d(_lineSegment.getStart()));
    sphere.expandBy(SGVec3d(_lineSegment.getEnd()));
    return sphere;
}

}
