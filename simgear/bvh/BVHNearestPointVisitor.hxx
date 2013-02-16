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

#ifndef BVHNearestPointVisitor_hxx
#define BVHNearestPointVisitor_hxx

#include <simgear/math/SGGeometry.hxx>

#include "BVHVisitor.hxx"

#include "BVHNode.hxx"
#include "BVHGroup.hxx"
#include "BVHPageNode.hxx"
#include "BVHTransform.hxx"
#include "BVHLineGeometry.hxx"
#include "BVHStaticGeometry.hxx"

#include "BVHStaticData.hxx"

#include "BVHStaticNode.hxx"
#include "BVHStaticTriangle.hxx"
#include "BVHStaticBinary.hxx"

namespace simgear {

class BVHNearestPointVisitor : public BVHVisitor {
public:
    BVHNearestPointVisitor(const SGSphered& sphere, const double& t) :
        _sphere(sphere),
        _time(t),
        _material(0),
        _id(0),
        _havePoint(false)
    { }
    
    virtual void apply(BVHGroup& leaf)
    {
        if (!intersects(_sphere, leaf.getBoundingSphere()))
            return;
        leaf.traverse(*this);
    }
    virtual void apply(BVHPageNode& leaf)
    {
        if (!intersects(_sphere, leaf.getBoundingSphere()))
            return;
        leaf.traverse(*this);
    }
    virtual void apply(BVHTransform& transform)
    {
        if (!intersects(_sphere, transform.getBoundingSphere()))
            return;
        
        SGSphered sphere = _sphere;
        _sphere = transform.sphereToLocal(sphere);
        bool havePoint = _havePoint;
        _havePoint = false;
        
        transform.traverse(*this);
        
        if (_havePoint) {
            _point = transform.ptToWorld(_point);
            _linearVelocity = transform.vecToWorld(_linearVelocity);
            _angularVelocity = transform.vecToWorld(_angularVelocity);
        }
        _havePoint |= havePoint;
        _sphere.setCenter(sphere.getCenter());
    }
    virtual void apply(BVHMotionTransform& transform)
    {
        if (!intersects(_sphere, transform.getBoundingSphere()))
            return;
        
        SGSphered sphere = _sphere;
        _sphere = transform.sphereToLocal(sphere, _time);
        bool havePoint = _havePoint;
        _havePoint = false;
        
        transform.traverse(*this);
        
        if (_havePoint) {
            SGMatrixd toWorld = transform.getToWorldTransform(_time);
            SGVec3d localCenter = _sphere.getCenter();
            _linearVelocity += transform.getLinearVelocityAt(localCenter);
            _angularVelocity += transform.getAngularVelocity();
            _linearVelocity = toWorld.xformVec(_linearVelocity);
            _angularVelocity = toWorld.xformVec(_angularVelocity);
            _point = toWorld.xformPt(_point);
            if (!_id)
                _id = transform.getId();
        }
        _havePoint |= havePoint;
        _sphere.setCenter(sphere.getCenter());
    }
    virtual void apply(BVHLineGeometry& node)
    { }
    virtual void apply(BVHStaticGeometry& node)
    {
        if (!intersects(_sphere, node.getBoundingSphere()))
            return;
        node.traverse(*this);
    }
    
    virtual void apply(const BVHStaticBinary& node, const BVHStaticData& data)
    {
        if (!intersects(_sphere, node.getBoundingBox()))
            return;
        node.traverse(*this, data, _sphere.getCenter());
    }
    virtual void apply(const BVHStaticTriangle& node, const BVHStaticData& data)
    {
        SGVec3f center(_sphere.getCenter());
        SGVec3d closest(closestPoint(node.getTriangle(data), center));
        if (!intersects(_sphere, closest))
            return;
        _point = closest;
        _linearVelocity = SGVec3d::zeros();
        _angularVelocity = SGVec3d::zeros();
        _material = data.getMaterial(node.getMaterialIndex());
        // The trick is to decrease the radius of the search sphere.
        _sphere.setRadius(length(closest - _sphere.getCenter()));
        _havePoint = true;
        _id = 0;
    }
    
    void setSphere(const SGSphered& sphere)
    { _sphere = sphere; }
    const SGSphered& getSphere() const
    { return _sphere; }
    
    const SGVec3d& getPoint() const
    { return _point; }
    const SGVec3d& getLinearVelocity() const
    { return _linearVelocity; }
    const SGVec3d& getAngularVelocity() const
    { return _angularVelocity; }
    const BVHMaterial* getMaterial() const
    { return _material; }
    BVHNode::Id getId() const
    { return _id; }
    
    bool empty() const
    { return !_havePoint; }
    
private:
    SGSphered _sphere;
    double _time;

    SGVec3d _point;
    SGVec3d _linearVelocity;
    SGVec3d _angularVelocity;
    const BVHMaterial* _material;
    BVHNode::Id _id;

    bool _havePoint;
};

}

#endif
