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

#include "BVHLineSegmentVisitor.hxx"

#include <simgear/math/SGGeometry.hxx>

#include "BVHVisitor.hxx"

#include "BVHNode.hxx"
#include "BVHGroup.hxx"
#include "BVHPageNode.hxx"
#include "BVHTransform.hxx"
#include "BVHMotionTransform.hxx"
#include "BVHLineGeometry.hxx"
#include "BVHStaticGeometry.hxx"

#include "BVHStaticData.hxx"

#include "BVHStaticNode.hxx"
#include "BVHStaticTriangle.hxx"
#include "BVHStaticBinary.hxx"

namespace simgear {

void
BVHLineSegmentVisitor::apply(BVHGroup& group)
{
    if (!intersects(_lineSegment, group.getBoundingSphere()))
        return;
    group.traverse(*this);
}

void
BVHLineSegmentVisitor::apply(BVHPageNode& pageNode)
{
    if (!intersects(_lineSegment, pageNode.getBoundingSphere()))
        return;
    pageNode.traverse(*this);
}
    
void
BVHLineSegmentVisitor::apply(BVHTransform& transform)
{
    if (!intersects(_lineSegment, transform.getBoundingSphere()))
        return;
    
    bool haveHit = _haveHit;
    _haveHit = false;
    
    // Push the line segment
    SGLineSegmentd lineSegment = getLineSegment();
    _lineSegment = transform.lineSegmentToLocal(lineSegment);
    
    transform.traverse(*this);
    
    if (_haveHit) {
        _linearVelocity = transform.vecToWorld(_linearVelocity);
        _angularVelocity = transform.vecToWorld(_angularVelocity);
        SGVec3d point(transform.ptToWorld(_lineSegment.getEnd()));
        _lineSegment.set(lineSegment.getStart(), point);
        _normal = transform.vecToWorld(_normal);
    } else {
        _lineSegment = lineSegment;
        _haveHit = haveHit;
    }
}

void
BVHLineSegmentVisitor::apply(BVHMotionTransform& transform)
{
    if (!intersects(_lineSegment, transform.getBoundingSphere()))
        return;
    
    bool haveHit = _haveHit;
    _haveHit = false;

    // Push the line segment
    SGLineSegmentd lineSegment = getLineSegment();
    SGMatrixd toLocal = transform.getToLocalTransform(_time);
    _lineSegment = lineSegment.transform(toLocal);
    
    transform.traverse(*this);
    
    if (_haveHit) {
        SGMatrixd toWorld = transform.getToWorldTransform(_time);
        SGVec3d localStart = _lineSegment.getStart();
        _linearVelocity += transform.getLinearVelocityAt(localStart);
        _angularVelocity += transform.getAngularVelocity();
        _linearVelocity = toWorld.xformVec(_linearVelocity);
        _angularVelocity = toWorld.xformVec(_angularVelocity);
        SGVec3d localEnd = _lineSegment.getEnd();
        _lineSegment.set(lineSegment.getStart(), toWorld.xformPt(localEnd));
        _normal = toWorld.xformVec(_normal);
        if (!_id)
            _id = transform.getId();
    } else {
        _lineSegment = lineSegment;
        _haveHit = haveHit;
    }
}

void
BVHLineSegmentVisitor::apply(BVHLineGeometry&)
{
}
    
void
BVHLineSegmentVisitor::apply(BVHStaticGeometry& node)
{
    if (!intersects(_lineSegment, node.getBoundingSphere()))
        return;
    node.traverse(*this);
}

void
BVHLineSegmentVisitor::apply(const BVHStaticBinary& node,
                             const BVHStaticData& data)
{
    if (!intersects(SGLineSegmentf(_lineSegment), node.getBoundingBox()))
        return;
    
    // The first box to enter is the one the startpoint is in.
    // this increases the probability, that on exit of that box we do not
    // even need to walk the other one, since the line segment is
    // then already short enough to not intersect the other one anymore.
    node.traverse(*this, data, _lineSegment.getStart());
}

void
BVHLineSegmentVisitor::apply(const BVHStaticTriangle& triangle,
                             const BVHStaticData& data)
{
    SGTrianglef tri = triangle.getTriangle(data);
    SGVec3f point;
    if (!intersects(point, tri, SGLineSegmentf(_lineSegment), 1e-4f))
        return;
    setLineSegmentEnd(SGVec3d(point));
    _normal = SGVec3d(tri.getNormal());
    _linearVelocity = SGVec3d::zeros();
    _angularVelocity = SGVec3d::zeros();
    _material = data.getMaterial(triangle.getMaterialIndex());
    _id = 0;
    _haveHit = true;
}


}
