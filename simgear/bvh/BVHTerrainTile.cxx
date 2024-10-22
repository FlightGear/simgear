// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023 Stuart Buchanan <stuart13@gmail.com>

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "BVHTerrainTile.hxx"
#include "BVHLineSegmentVisitor.hxx"
#include "BVHSubTreeCollector.hxx"
#include "simgear/scene/tgdb/VPBTechnique.hxx"

#include <algorithm>

namespace simgear {

BVHTerrainTile::BVHTerrainTile(osgTerrain::TerrainTile *tile)
{
    _tile = tile;
}

BVHTerrainTile::~BVHTerrainTile()
{
    _tile = 0;
}

BVHMaterial* BVHTerrainTile::getMaterial(simgear::BVHLineSegmentVisitor* lsv) {
    BVHMaterial* material = 0;
    if (! lsv->empty()) {
        // LSV contains the uv coordinates of the intersection u*(v1-v0) + v*(v2-v0) and the origin indices on the drawable they refer to.
        // However it does not have any information on the actual material as this is part of the TerrainTile texture.
        simgear::VPBTechnique* technique = dynamic_cast<simgear::VPBTechnique*>(_tile->getTerrainTechnique());
        if (technique) {
            material = technique->getMaterial(toOsg(lsv->getPoint()));
        } else {
            //  This is possible when the BVH has been created outside of a full FG run, e.g. from fgelev
            SG_LOG(SG_TERRAIN, SG_DEBUG, "BVHTerrainTile::getMaterial unable to get technique");
        }
    } else {
        SG_LOG(SG_TERRAIN, SG_ALERT, "BVHTerrainTile::getMaterial but no LSV hit");
    }

    return material;
}

void BVHTerrainTile::accept(BVHVisitor& visitor)
{
    visitor.apply(*this);
}

SGSphered BVHTerrainTile::computeBoundingSphere() const {
    simgear::VPBTechnique* technique = dynamic_cast<simgear::VPBTechnique*>(_tile->getTerrainTechnique());
    if (technique) {
        return technique->computeBoundingSphere();
    } else {
        return BVHGroup::computeBoundingSphere();
    }
}


}
