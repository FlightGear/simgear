// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "BVHTransform.hxx"

#include "BVHVisitor.hxx"
#include "BVHNode.hxx"
#include "BVHGroup.hxx"

namespace simgear {

BVHTransform::BVHTransform() :
    _toWorld(SGMatrixd::unit()),
    _toLocal(SGMatrixd::unit()),
    _toWorldAmplification(1),
    _toLocalAmplification(1)
{
}

BVHTransform::~BVHTransform()
{
}

void
BVHTransform::accept(BVHVisitor& visitor)
{
    visitor.apply(*this);
}
    
void
BVHTransform::setTransform(const BVHTransform& transform)
{
    _toWorld = transform._toWorld;
    _toLocal = transform._toLocal;
    _toWorldAmplification = transform._toWorldAmplification;
    _toLocalAmplification = transform._toLocalAmplification;
    invalidateParentBound();
}

void
BVHTransform::setToWorldTransform(const SGMatrixd& transform)
{
    _toWorld = transform;
    invert(_toLocal, transform);
    updateAmplificationFactors();
    invalidateParentBound();
}

void
BVHTransform::setToLocalTransform(const SGMatrixd& transform)
{
    _toLocal = transform;
    invert(_toWorld, transform);
    updateAmplificationFactors();
    invalidateParentBound();
}

SGSphered
BVHTransform::computeBoundingSphere() const
{
    return sphereToWorld(BVHGroup::computeBoundingSphere());
}

void
BVHTransform::updateAmplificationFactors()
{
    // Hmm, this is just a hint, true?
    // But anyway, almost all transforms in a scenegraph will
    // have them equal to 1 ...
    double r = norm(vecToWorld(SGVec3d(1, 0, 0)));
    r = std::max(r, norm(vecToWorld(SGVec3d(0, 1, 0))));
    r = std::max(r, norm(vecToWorld(SGVec3d(0, 0, 1))));
    _toWorldAmplification = r;
    
    r = norm(vecToLocal(SGVec3d(1, 0, 0)));
    r = std::max(r, norm(vecToLocal(SGVec3d(0, 1, 0))));
    r = std::max(r, norm(vecToLocal(SGVec3d(0, 0, 1))));
    _toLocalAmplification = r;
}

}
