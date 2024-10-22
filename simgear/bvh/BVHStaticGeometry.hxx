// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef BVHStaticGeometry_hxx
#define BVHStaticGeometry_hxx

#include <simgear/math/SGGeometry.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

#include "BVHVisitor.hxx"
#include "BVHNode.hxx"
#include "BVHStaticData.hxx"
#include "BVHStaticNode.hxx"

namespace simgear {

class BVHStaticGeometry : public BVHNode {
public:
    BVHStaticGeometry(const BVHStaticNode* staticNode,
                      const BVHStaticData* staticData);
    virtual ~BVHStaticGeometry();
    
    virtual void accept(BVHVisitor& visitor);
    
    void traverse(BVHVisitor& visitor) const
    { _staticNode->accept(visitor, *_staticData); }
    
    const BVHStaticData* getStaticData() const
    { return _staticData; }
    const BVHStaticNode* getStaticNode() const
    { return _staticNode; }
    
    virtual SGSphered computeBoundingSphere() const;
    
private:
    SGSharedPtr<const BVHStaticNode> _staticNode;
    SGSharedPtr<const BVHStaticData> _staticData;
};

}

#endif
