// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023 Stuart Buchanan <stuart13@gmail.com>

#ifndef BVHTerrainTile_hxx
#define BVHTerrainTile_hxx

#include <simgear/structure/SGSharedPtr.hxx>
#include "BVHGroup.hxx"
#include "BVHLineSegmentVisitor.hxx"
#include "BVHMaterial.hxx"
#include "BVHVisitor.hxx"
#include <osgTerrain/TerrainTile>

namespace simgear {

class BVHTerrainTile : public BVHGroup {
public:
    BVHTerrainTile(osgTerrain::TerrainTile *tile);
    virtual ~BVHTerrainTile();
    virtual void accept(BVHVisitor& visitor);
    virtual SGSphered computeBoundingSphere() const;
    BVHMaterial* getMaterial(BVHLineSegmentVisitor* lsv);
private:
    osg::ref_ptr<osgTerrain::TerrainTile> _tile;
};

}

#endif
