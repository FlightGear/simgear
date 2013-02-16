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

#ifndef BVHVisitor_hxx
#define BVHVisitor_hxx

namespace simgear {

class BVHStaticData;

class BVHGroup;
class BVHPageNode;
class BVHTransform;
class BVHMotionTransform;
class BVHStaticGeometry;
class BVHLineGeometry;

class BVHStaticBinary;
class BVHStaticTriangle;

class BVHVisitor {
public:
    // The magnitudes of pure virtuals is because of the fact that this chaining
    // just takes needless runtime. This declaration should force the user of
    // this classes to implement a common functionality that should be called
    // from each apropriate apply method directly.
    virtual ~BVHVisitor() {}

    // High level nodes to handle
    virtual void apply(BVHGroup&) = 0;
    virtual void apply(BVHPageNode&) = 0;
    virtual void apply(BVHTransform&) = 0;
    virtual void apply(BVHMotionTransform&) = 0;
    virtual void apply(BVHLineGeometry&) = 0;
    virtual void apply(BVHStaticGeometry&) = 0;
    
    // Static tree nodes to handle
    virtual void apply(const BVHStaticBinary&, const BVHStaticData&) = 0;
    virtual void apply(const BVHStaticTriangle&, const BVHStaticData&) = 0;
};

}

#endif
