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

#ifndef BVHMotionTransform_hxx
#define BVHMotionTransform_hxx

#include "BVHVisitor.hxx"
#include "BVHNode.hxx"
#include "BVHGroup.hxx"

namespace simgear {

class BVHMotionTransform : public BVHGroup {
public:
    BVHMotionTransform();
    virtual ~BVHMotionTransform();

    virtual void accept(BVHVisitor& visitor);

    void setTransform(const BVHMotionTransform& transform);
    void setToWorldTransform(const SGMatrixd& transform);
    void setToLocalTransform(const SGMatrixd& transform);

    void setLinearVelocity(const SGVec3d& linearVelocity)
    { _linearVelocity = linearVelocity; }
    const SGVec3d& getLinearVelocity() const
    { return _linearVelocity; }

    void setAngularVelocity(const SGVec3d& angularVelocity)
    { _angularVelocity = angularVelocity; }
    const SGVec3d& getAngularVelocity() const
    { return _angularVelocity; }

    void setReferenceTime(const double& referenceTime)
    { _referenceTime = referenceTime; }
    const double& getReferenceTime() const
    { return _referenceTime; }

    void setStartTime(const double& startTime)
    { _startTime = startTime; }
    const double& getStartTime() const
    { return _startTime; }

    void setEndTime(const double& endTime)
    { _endTime = endTime; }
    const double& getEndTime() const
    { return _endTime; }
    
    SGMatrixd getToWorldTransform(const double& t) const
    {
        if (t == _referenceTime)
            return _toWorldReference;
        double dt = t - _referenceTime;
        SGMatrixd matrix(_toWorldReference);
        matrix.postMultRotate(SGQuatd::fromAngleAxis(dt*_angularVelocity));
        matrix.postMultTranslate(dt*_linearVelocity);
        return matrix;
    }
    SGMatrixd getToLocalTransform(const double& t) const
    {
        if (t == _referenceTime)
            return _toLocalReference;
        double dt = _referenceTime - t;
        SGMatrixd matrix(_toLocalReference);
        matrix.preMultRotate(SGQuatd::fromAngleAxis(dt*_angularVelocity));
        matrix.preMultTranslate(dt*_linearVelocity);
        return matrix;
    }

    const SGMatrixd& getToWorldReferenceTransform() const
    { return _toWorldReference; }
    const SGMatrixd& getToLocalReferenceTransform() const
    { return _toLocalReference; }

    SGVec3d getLinearVelocityAt(const SGVec3d& reference) const
    { return _linearVelocity + cross(_angularVelocity, reference); }

    SGSphered sphereToLocal(const SGSphered& sphere, const double& t) const
    {
        SGMatrixd matrix = getToLocalTransform(t);
        SGVec3d center = matrix.xformPt(sphere.getCenter());
        double radius = _toLocalAmplification*sphere.getRadius();
        return SGSphered(center, radius);
    }

    void setId(Id id)
    { _id = id; }
    Id getId() const
    { return _id; }

private:
    virtual SGSphered computeBoundingSphere() const;
    void updateAmplificationFactors();
    
    SGMatrixd _toWorldReference;
    SGMatrixd _toLocalReference;
    double _toWorldAmplification;
    double _toLocalAmplification;

    SGVec3d _linearVelocity;
    SGVec3d _angularVelocity;

    double _referenceTime;
    double _startTime;
    double _endTime;

    Id _id;
};

}

#endif
