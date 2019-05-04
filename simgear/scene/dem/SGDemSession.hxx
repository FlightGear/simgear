#ifndef __SG_DEM_SESSION_HXX__
#define __SG_DEM_SESSION_HXX__

#include <simgear/scene/dem/SGDemRoot.hxx>

class SGDemSession {
public:
    SGDemSession()
        : west_off(0)
        , south_off(0)
        , east_off(0)
        , north_off(0)
        , maxLon(0)
        , maxLat(0)
        , pDemRoot(NULL)
        , lvlIndex(-1)
        , lvlWidth(0)
        , lvlHeight(0)
    {
    }

    SGDemSession(int mnLon, int mnLat, int mxLon, int mxLat, int idx, int lvlW, int lvlH, SGDemRoot* root);
    SGDemSession(int mnLon, int mnLat, int mxLon, int mxLat, SGDemRoot* root);

    SGDemSession(unsigned wo, unsigned so, unsigned eo, unsigned no, int idx, unsigned lvlW, unsigned lvlH, SGDemRoot* root)
        : west_off(wo)
        , south_off(so)
        , east_off(eo)
        , north_off(no)
        , maxLon(0)
        , maxLat(0)
        , pDemRoot(root)
        , lvlIndex(idx)
        , lvlWidth(lvlW)
        , lvlHeight(lvlH)
    {
    }

    SGDemSession(unsigned wo, unsigned so, unsigned eo, unsigned no, SGDemRoot* root)
        : west_off(wo)
        , south_off(so)
        , east_off(eo)
        , north_off(no)
        , maxLon(0)
        , maxLat(0)
        , pDemRoot(root)
        , lvlIndex(-1) // no level - session is raw input dir
        , lvlWidth(0)
        , lvlHeight(0)
    {
    }

    ~SGDemSession()
    {
        close();
    }

    void addTile(SGDemTileRef pTile)
    {
        tileRefs.push_back(pTile);
    }

    const std::vector<SGDemTileRef>& getTiles(void) const
    {
        return tileRefs;
    }

    unsigned int size(void) const
    {
        return tileRefs.size();
    }

    void getGeods(unsigned wp, unsigned so, unsigned eo, unsigned no,
        int resx, int resy, int incx, int incy,
        ::std::vector<SGGeod>& geods,
        bool Debug1, bool Debug2);

    void close(void);

    int getLvlIndex(void) const
    {
        return lvlIndex;
    };

private:
    void setOffsets( unsigned wo, unsigned so, unsigned eo, unsigned no ) {
        west_off  = wo;
        south_off = so;
        east_off  = eo;
        north_off = no;
    }
    
    unsigned west_off, south_off;
    unsigned east_off, north_off;
    int maxLon, maxLat;
    SGDemRoot* pDemRoot;
    int lvlIndex;
    unsigned lvlWidth, lvlHeight;

    std::vector<SGDemTileRef> tileRefs;
};

#endif /* __SG_DEM_SESSION_HXX__ */
