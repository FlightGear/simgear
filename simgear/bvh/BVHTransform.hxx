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
