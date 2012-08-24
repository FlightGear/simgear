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
