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
    { return _children.size(); }
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
