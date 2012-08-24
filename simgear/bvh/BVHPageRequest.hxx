// Copyright (C) 2008 - 2012 Mathias Froehlich - Mathias.Froehlich@web.de
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
