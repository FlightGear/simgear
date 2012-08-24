// Copyright (C) 2008 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifndef BVHPageNode_hxx
#define BVHPageNode_hxx

#include <list>

#include <simgear/structure/SGSharedPtr.hxx>

#include "BVHGroup.hxx"
#include "BVHVisitor.hxx"

namespace simgear {

class BVHPager;
class BVHPageRequest;

class BVHPageNode : public BVHGroup {
public:
    BVHPageNode();
    virtual ~BVHPageNode();
    
    virtual void accept(BVHVisitor& visitor);

    /// Return the usage stamp of the last access
    unsigned getUseStamp() const
    { return _useStamp; }

    virtual SGSphered computeBoundingSphere() const = 0;

    virtual BVHPageRequest* newRequest() = 0;

protected:
    virtual void invalidateBound() = 0;

    bool getRequested() const
    { return _requested; }
    void setRequested(bool requested)
    { _requested = requested; }

private:
    friend class BVHPager;

    std::list<SGSharedPtr<BVHPageNode> >::iterator _iterator;
    unsigned _useStamp;
    bool _requested;
};

}

#endif
