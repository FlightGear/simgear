// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2009 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef BVHGroup_hxx
#define BVHGroup_hxx

#include <simgear/structure/SGSharedPtr.hxx>
#include "BVHNode.hxx"
#include "BVHVisitor.hxx"

namespace simgear {

class BVHGroup : public BVHNode {
public:
    BVHGroup();
    virtual ~BVHGroup();
    
    virtual void accept(BVHVisitor& visitor);
    
    void traverse(BVHVisitor& visitor)
    {
        ChildList::const_iterator i;
        for (i = _children.begin(); i != _children.end(); ++i)
            (*i)->accept(visitor);
    }
    
    void clear();
    void addChild(BVHNode* child);
    void removeChild(BVHNode* child);
    
    unsigned getNumChildren() const
    { return static_cast<unsigned>(_children.size()); }
    const BVHNode* getChild(unsigned i) const
    { if (_children.size() <= i) return 0; return _children[i]; }
    BVHNode* getChild(unsigned i)
    { if (_children.size() <= i) return 0; return _children[i]; }
    
    virtual SGSphered computeBoundingSphere() const;
    
private:
    typedef std::vector<SGSharedPtr<BVHNode> > ChildList;
    ChildList _children;
};

}

#endif
