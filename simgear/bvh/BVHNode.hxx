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

#ifndef BVHNode_hxx
#define BVHNode_hxx

#include <vector>
#include <simgear/math/SGGeometry.hxx>
#include <simgear/structure/SGReferenced.hxx>

namespace simgear {

class BVHGroup;
class BVHVisitor;
class BVHPageNode;

// Base for the tree nodes
class BVHNode : public SGReferenced {
public:
    BVHNode();
    virtual ~BVHNode();
    
    // visitors ...
    virtual void accept(BVHVisitor& visitor) = 0;
    
    const SGSphered& getBoundingSphere() const
    {
        if (_dirtyBoundingSphere) {
            _boundingSphere = computeBoundingSphere();
            _dirtyBoundingSphere = false;
        }
        return _boundingSphere;
    }
    virtual SGSphered computeBoundingSphere() const = 0;

    /// A unique id for some kind of BVHNodes.
    /// Currently only motions transforms.
    typedef unsigned Id;

    // Factory to get a new id
    static Id getNewId();
    
protected:
    friend class BVHGroup;
    friend class BVHPageNode;
    void addParent(BVHNode* parent);
    void removeParent(BVHNode* parent);
    
    void invalidateParentBound();
    virtual void invalidateBound();
    
private:
    mutable bool _dirtyBoundingSphere;
    mutable SGSphered _boundingSphere;
    
    typedef std::vector<BVHNode*> ParentList;
    ParentList _parents;
};

}

#endif
