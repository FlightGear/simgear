// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2012 Mathias Froehlich <mathias.froehlich@web.de>

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
