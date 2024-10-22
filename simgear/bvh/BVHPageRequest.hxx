// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2012 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef BVHPageRequest_hxx
#define BVHPageRequest_hxx

#include <simgear/structure/SGReferenced.hxx>

namespace simgear {

class BVHPageNode;

class BVHPageRequest : public SGReferenced {
public:
    virtual ~BVHPageRequest();

    /// Happens in the pager thread, do not modify the calling bvh tree
    virtual void load() = 0;
    /// Happens in the bvh main thread where the bvh is actually used.
    /// So inside here it is safe to modify the paged node
    virtual void insert() = 0;
    /// The page node this request is for
    virtual BVHPageNode* getPageNode() = 0;
};

}

#endif
