// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

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
