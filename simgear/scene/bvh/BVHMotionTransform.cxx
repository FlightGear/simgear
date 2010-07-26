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

#include "BVHMotionTransform.hxx"

#include "BVHVisitor.hxx"
#include "BVHNode.hxx"
#include "BVHGroup.hxx"

namespace simgear {

BVHMotionTransform::BVHMotionTransform() :
    _toWorldReference(SGMatrixd::unit()),
    _toLocalReference(SGMatrixd::unit()),
    _toWorldAmplification(1),
    _toLocalAmplification(1),
    _linearVelocity(0, 0, 0),
    _angularVelocity(0, 0, 0),
    _referenceTime(0),
    _startTime(0),
    _endTime(0),
    _id(0)
{
}

BVHMotionTransform::~BVHMotionTransform()
{
}

void
BVHMotionTransform::accept(BVHVisitor& visitor)
{
    visitor.apply(*this);
}

void
BVHMotionTransform::setTransform(const BVHMotionTransform& transform)
{
    _toWorldReference = transform._toWorldReference;
    _toLocalReference = transform._toLocalReference;
    _toWorldAmplification = transform._toWorldAmplification;
    _toLocalAmplification = transform._toLocalAmplification;
    _linearVelocity = transform._linearVelocity;
    _angularVelocity = transform._angularVelocity;
    _referenceTime = transform._referenceTime;
    _startTime = transform._startTime;
    _endTime = transform._endTime;
    _id = transform._id;
    invalidateParentBound();
}

void
BVHMotionTransform::setToWorldTransform(const SGMatrixd& transform)
{
    _toWorldReference = transform;
    invert(_toLocalReference, transform);
    updateAmplificationFactors();
    invalidateParentBound();
}

void
BVHMotionTransform::setToLocalTransform(const SGMatrixd& transform)
{
    _toLocalReference = transform;
    invert(_toWorldReference, transform);
    updateAmplificationFactors();
    invalidateParentBound();
}

SGSphered
BVHMotionTransform::computeBoundingSphere() const
{
    SGSphered sphere(BVHGroup::computeBoundingSphere());
    if (sphere.empty())
        return sphere;
    SGMatrixd toWorldStart = getToWorldTransform(_startTime);
    SGVec3d centerStart = toWorldStart.xformPt(sphere.getCenter());
    SGMatrixd toWorldEnd = getToWorldTransform(_endTime);
    SGVec3d centerEnd = toWorldEnd.xformPt(sphere.getCenter());
    double rad = 0.5*length(centerStart - centerEnd) + sphere.getRadius();
    rad *= _toWorldAmplification;
    return SGSphered(0.5*(centerStart + centerEnd), rad);
}

void
BVHMotionTransform::updateAmplificationFactors()
{
    // Hmm, this is just a hint, true?
    // But anyway, almost all transforms in a scenegraph will
    // have them equal to 1 ...
    double r = norm(_toWorldReference.xformVec(SGVec3d(1, 0, 0)));
	r = SGMiscd::max(r, norm(_toWorldReference.xformVec(SGVec3d(0, 1, 0))));
    r = SGMiscd::max(r, norm(_toWorldReference.xformVec(SGVec3d(0, 0, 1))));
    _toWorldAmplification = r;
    
    r = norm(_toLocalReference.xformVec(SGVec3d(1, 0, 0)));
    r = SGMiscd::max(r, norm(_toLocalReference.xformVec(SGVec3d(0, 1, 0))));
    r = SGMiscd::max(r, norm(_toLocalReference.xformVec(SGVec3d(0, 0, 1))));
    _toLocalAmplification = r;
}

}
