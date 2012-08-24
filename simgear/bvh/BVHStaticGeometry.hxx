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
