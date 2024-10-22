// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2012 Mathias Froehlich <mathias.froehlich@web.de>

#ifndef BVHPager_hxx
#define BVHPager_hxx

#include <simgear/structure/SGSharedPtr.hxx>

namespace simgear {

class BVHPageNode;
class BVHPageRequest;

class BVHPager {
public:
    BVHPager();
    virtual ~BVHPager();

    /// Starts the pager thread
    bool start();

    /// Stops the pager thread
    void stop();

    /// Use this page node, if loaded make it as used, if not loaded schedule
    void use(BVHPageNode& pageNode);

    /// Call this from the main thread to incorporate the processed page
    /// requests into the bounding volume tree
    void update(unsigned expiry);

    /// The usage stamp to mark usage of BVHPageNodes
    void setUseStamp(unsigned stamp);
    unsigned getUseStamp() const;

private:
    BVHPager(const BVHPager&);
    BVHPager& operator=(const BVHPager&);

    struct _PrivateData;
    _PrivateData* _privateData;
};

}

#endif
