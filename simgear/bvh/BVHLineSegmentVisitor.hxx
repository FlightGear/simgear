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

#ifndef BVHLineSegmentVisitor_hxx
#define BVHLineSegmentVisitor_hxx

#include <simgear/math/SGGeometry.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include "BVHVisitor.hxx"
#include "BVHNode.hxx"

namespace simgear {

class BVHMaterial;

class BVHLineSegmentVisitor : public BVHVisitor {
public:
    BVHLineSegmentVisitor(const SGLineSegmentd& lineSegment,
                          const double& t = 0) :
        _lineSegment(lineSegment),
        _time(t),
        _material(0),
        _id(0),
        _haveHit(false)
    { }
    virtual ~BVHLineSegmentVisitor()
    { }
    
    bool empty() const
    { return !_haveHit; }
    
    const SGLineSegmentd& getLineSegment() const
    { return _lineSegment; }
    
    SGVec3d getPoint() const
    { return _lineSegment.getEnd(); }
    const SGVec3d& getNormal() const
    { return _normal; }
    const SGVec3d& getLinearVelocity() const
    { return _linearVelocity; }
    const SGVec3d& getAngularVelocity() const
    { return _angularVelocity; }
    const BVHMaterial* getMaterial() const
    { return _material; }
    BVHNode::Id getId() const
    { return _id; }

    virtual void apply(BVHGroup& group);
    virtual void apply(BVHPageNode& node);
    virtual void apply(BVHTransform& transform);
    virtual void apply(BVHMotionTransform& transform);
    virtual void apply(BVHLineGeometry&);
    virtual void apply(BVHStaticGeometry& node);
    
    virtual void apply(const BVHStaticBinary&, const BVHStaticData&);
    virtual void apply(const BVHStaticTriangle&, const BVHStaticData&);
    
protected:
    void setLineSegmentEnd(const SGVec3d& end)
    {
        // Ok, you need to make sure that the new end is in the previous
        // direction and that the line segment is not enlarged by that call.
#ifndef _NDEBUG
        // FIXME insert code to check this...
#endif
        _lineSegment.set(_lineSegment.getStart(), end);
    }
    
private:
    SGLineSegmentd _lineSegment;
    double _time;
    
    // belongs in a derived class
    SGVec3d _normal;
    SGVec3d _linearVelocity;
    SGVec3d _angularVelocity;
    const BVHMaterial* _material;
    BVHNode::Id _id;
    
    bool _haveHit;
};

}

#endif
