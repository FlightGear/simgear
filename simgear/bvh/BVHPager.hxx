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

#ifndef BVHPager_hxx
#define BVHPager_hxx

#include <simgear/structure/SGSharedPtr.hxx>

namespace simgear {

class BVHPageNode;
class BVHPageRequest;

class BVHPager {
public:
    BVHPager();
    ~BVHPager();

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
