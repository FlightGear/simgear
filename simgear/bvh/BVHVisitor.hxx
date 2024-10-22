// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008-2012 Mathias Froehlich <mathias.froehlich@web.de>

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
class BVHTerrainTile;

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
    virtual void apply(BVHTerrainTile&) = 0;

    // Static tree nodes to handle
    virtual void apply(const BVHStaticBinary&, const BVHStaticData&) = 0;
    virtual void apply(const BVHStaticTriangle&, const BVHStaticData&) = 0;
};

}

#endif
