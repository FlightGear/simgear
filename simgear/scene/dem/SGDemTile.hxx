#ifndef __SG_DEM_TILE_HXX__
#define __SG_DEM_TILE_HXX__

#include <gdal.h>
#include <gdal_priv.h>

#include <map>

#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGGeod.hxx>

class SGDemSession;

class SGDemTile : public SGReferenced
{
public:
    // TODO - simple constructor, so writing a tile to disk not done in constructor.
    // then - reading / writing done with tile API, not constructor.
    //SGDemTile( const SGPath& path, int lon, int lat, int w, int h, int x, int y, int overlap );

    // constructor for reading a tile
    SGDemTile( const SGPath& path, unsigned wo, unsigned so, int w, int h, int x, int y, int overlap, bool cache );
    // constructor for writing a tile
    SGDemTile( const SGPath& path, unsigned wo, unsigned so, int w, int h, int x, int y, int overlap, const SGDemSession& s, bool& bWritten );

    ~SGDemTile();

    // read / write tile from / to disk
    //int read( bool cache );
    //int write( const SGDemSession& s );

    SGPath getPath( void ) const { return path; }
    unsigned short getAlt(const SGGeod& loc) const;
    void getGeods(unsigned wo, unsigned so, unsigned eo, unsigned no, int grid_width, int grid_height, unsigned subx, unsigned suby, int incw, int inch, ::std::vector<SGGeod>& geods, bool Debug1, bool Debug2);

private:
    std::string     getTileName( int lon, int lat );
    void            dbgDumpDataset( GDALDataset* poDataset ) const;
    void            dbgDumpBand( GDALRasterBand* poBand ) const;

    unsigned short* cacheTile( const SGPath& path );

    GDALDatasetH    createTile( char **papszSrcFiles, 
                        const char *pszFilename, 
                        int nForceLines, int nForcePixels,
                        double dfMinX, double dfMinY,
                        double dfMaxX, double dfMaxY,
                        char **papszTO ); 

    void            doWarp( int iSrc, char* pszSrcFile, GDALDatasetH hDstDS, char **papszTO, char** papszWarpOptions ); 

    SGPath          path;
    unsigned short* raster;

    int             ref_lon, ref_lat;
    int             width, height;
    int             resx, resy;
    int             overlap;
    double          pixResX, pixResY;
};

typedef SGSharedPtr<SGDemTile> SGDemTileRef;
typedef std::map<unsigned long, SGDemTileRef> SGDemCache;

#endif /* #define __SG_DEM_TILE_HXX__ */
