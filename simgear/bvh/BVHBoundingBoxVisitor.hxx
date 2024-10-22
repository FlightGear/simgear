// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef BVHBoundingBoxVisitor_hxx
#define BVHBoundingBoxVisitor_hxx

#include <simgear/math/SGGeometry.hxx>

#include "BVHVisitor.hxx"
#include "BVHNode.hxx"
#include "BVHGroup.hxx"
#include "BVHPageNode.hxx"
#include "BVHTransform.hxx"
#include "BVHMotionTransform.hxx"
#include "BVHLineGeometry.hxx"
#include "BVHTerrainTile.hxx"

#include "BVHStaticData.hxx"

#include "BVHStaticNode.hxx"
#include "BVHStaticTriangle.hxx"
#include "BVHStaticBinary.hxx"
#include "BVHStaticGeometry.hxx"

namespace simgear {

class BVHBoundingBoxVisitor : public BVHVisitor {
public:
    virtual ~BVHBoundingBoxVisitor() {}
    
    void clear()
    { _box.clear(); }
    
    virtual void apply(BVHGroup& node)
    { expandBy(node.getBoundingSphere()); }
    virtual void apply(BVHPageNode& node)
    { expandBy(node.getBoundingSphere()); }
    virtual void apply(BVHTransform& node)
    { expandBy(node.getBoundingSphere()); }
    virtual void apply(BVHMotionTransform& node)
    { expandBy(node.getBoundingSphere()); }
    virtual void apply(BVHLineGeometry& node)
    { expandBy(node.getBoundingSphere()); }
    virtual void apply(BVHStaticGeometry& node)
    { expandBy(node.getBoundingSphere()); }
    virtual void apply(BVHTerrainTile& node)
    { expandBy(node.getBoundingSphere()); }
    
    virtual void apply(const BVHStaticBinary& node, const BVHStaticData& data)
    { expandBy(node.getBoundingBox()); }
    virtual void apply(const BVHStaticTriangle& node, const BVHStaticData& data)
    { expandBy(node.computeBoundingBox(data)); }
    
    const SGBoxf& getBox() const
    { return _box; }
    
private:
    void expandBy(const SGSphered& sphere)
    {
        SGVec3f v0(sphere.getCenter() - sphere.getRadius()*SGVec3d(1, 1, 1));
        SGVec3f v1(sphere.getCenter() + sphere.getRadius()*SGVec3d(1, 1, 1));
        _box.expandBy(SGBoxf(v0, v1));
    }
    void expandBy(const SGBoxf& box)
    { _box.expandBy(box); }
    
    SGBoxf _box;
};

}

#endif
