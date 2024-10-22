// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef BVHTransform_hxx
#define BVHTransform_hxx

#include "BVHGroup.hxx"

namespace simgear {

class BVHTransform : public BVHGroup {
public:
    BVHTransform();
    virtual ~BVHTransform();

    virtual void accept(BVHVisitor& visitor);
    
    void setTransform(const BVHTransform& transform);

    void setToWorldTransform(const SGMatrixd& transform);
    const SGMatrixd& getToWorldTransform() const
    { return _toWorld; }

    void setToLocalTransform(const SGMatrixd& transform);
    const SGMatrixd& getToLocalTransform() const
    { return _toLocal; }
    
    SGVec3d ptToWorld(const SGVec3d& point) const
    { return _toWorld.xformPt(point); }
    SGVec3d ptToLocal(const SGVec3d& point) const
    { return _toLocal.xformPt(point); }
    
    SGVec3d vecToWorld(const SGVec3d& vec) const
    { return _toWorld.xformVec(vec); }
    SGVec3d vecToLocal(const SGVec3d& vec) const
    { return _toLocal.xformVec(vec); }
    
    SGLineSegmentd lineSegmentToWorld(const SGLineSegmentd& lineSeg) const
    { return lineSeg.transform(_toWorld); }
    SGLineSegmentd lineSegmentToLocal(const SGLineSegmentd& lineSeg) const
    { return lineSeg.transform(_toLocal); }
    
    SGSphered sphereToWorld(const SGSphered& sphere) const
    {
        SGVec3d center = ptToWorld(sphere.getCenter());
        double radius = _toWorldAmplification*sphere.getRadius();
        return SGSphered(center, radius);
    }
    SGSphered sphereToLocal(const SGSphered& sphere) const
    {
        SGVec3d center = ptToLocal(sphere.getCenter());
        double radius = _toLocalAmplification*sphere.getRadius();
        return SGSphered(center, radius);
    }

private:
    virtual SGSphered computeBoundingSphere() const;
    void updateAmplificationFactors();
    
    SGMatrixd _toWorld;
    SGMatrixd _toLocal;
    double _toWorldAmplification;
    double _toLocalAmplification;
};

}

#endif
