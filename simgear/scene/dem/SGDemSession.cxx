#include <simgear/scene/dem/SGDem.hxx>
#include <simgear/scene/dem/SGDemSession.hxx>

SGDemSession::SGDemSession(int mnLon, int mnLat, int mxLon, int mxLat, int idx, int lvlW, int lvlH, SGDemRoot* root)
{
    setOffsets(SGDem::longitudeDegToOffset((double)mnLon),
        SGDem::latitudeDegToOffset((double)mnLat),
        SGDem::longitudeDegToOffset((double)mxLon),
        SGDem::latitudeDegToOffset((double)mxLat));

    pDemRoot = root;
    lvlIndex = idx;
    lvlWidth = lvlW;
    lvlHeight = lvlH;
}

SGDemSession::SGDemSession(int mnLon, int mnLat, int mxLon, int mxLat, SGDemRoot* root)
{
    setOffsets(SGDem::longitudeDegToOffset((double)mnLon),
        SGDem::latitudeDegToOffset((double)mnLat),
        SGDem::longitudeDegToOffset((double)mxLon),
        SGDem::latitudeDegToOffset((double)mxLat));

    pDemRoot = root;
    lvlIndex = -1; // no level - session is raw input dir
}

void SGDemSession::close(void)
{
    if (tileRefs.size()) {
        tileRefs.clear();
        if (lvlIndex >= 0) {
            pDemRoot->flushCaches(lvlIndex);
        }
    }
}

void SGDemSession::getGeods(unsigned wo, unsigned so, unsigned eo, unsigned no, int resx, int resy, int incx, int incy, ::std::vector<SGGeod>& geods, bool Debug1, bool Debug2)
{
    // todo - store this info in deminfo
    unsigned span; // smallest tile width/height in level ( in offsets )
    switch (lvlIndex) {
    case 0:
        span = 1;
        break; // 1/8 deg
    case 1:
        span = 16;
        break; // 2   degrees
    case 2:
        span = 480;
        break; // 60  degrees
    default:
        fprintf(stderr, "invalid lvlIndex %d\n", lvlIndex);
        exit(0);
    }

    if (lvlIndex >= 0) {
        unsigned tileLon, tileLat;
        unsigned meshLon, meshLat;
        int subx, suby;

        meshLon = wo;
        tileLon = SGDem::roundDown(meshLon, lvlWidth);
        subx = (meshLon - tileLon) / span;
        // fprintf(stderr, "getGeods: lon is %lf : meshLon is %u, tileLon is %u, subx is %d\n", SGDem::offsetToLongitudeDeg(wo), meshLon, tileLon, subx );

        meshLat = so;
        tileLat = SGDem::roundDown(meshLat, lvlHeight);
        suby = (meshLat - tileLat) / span;
        // fprintf(stderr, "getGeods: lat is %lf : meshLat is %u, tileLat is %u, suby is %d\n", SGDem::offsetToLatitudeDeg(so), meshLat, tileLat, suby );

        // get the tle from the tile cache
        unsigned long key = tileLon << 16 | tileLat;
        SGDemTileRef tile = pDemRoot->getTile(lvlIndex, key);
        if (tile) {
            tile->getGeods(wo, so, eo, no, resx, resy, subx, suby, incx, incy, geods, Debug1, Debug2);
        } else {
            fprintf(stderr, " *** ERROR: tile %u,%u not in session @ (%lf,%lf) - (%lf,%lf)\n",
                tileLon, tileLat,
                SGDem::offsetToLongitudeDeg(west_off),
                SGDem::offsetToLatitudeDeg(south_off),
                SGDem::offsetToLongitudeDeg(east_off),
                SGDem::offsetToLatitudeDeg(north_off));
        }
    }
}
