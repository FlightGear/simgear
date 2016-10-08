// dem.hxx -- read, write DEM heiarchy
//
// Written by Peter Sadrozinski, started August 2016.
//
// Copyright (C) 2001 - 2003  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _SG_DEM_HXX
#define _SG_DEM_HXX

#include <map>

#include <gdal.h>
#include <gdalwarper.h>
#include <gdal_priv.h>

#include <simgear/compiler.h>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/math/SGGeod.hxx>
#include <simgear/misc/sg_path.hxx>

class SGDem;
class SGDemSession;

class SGDemTile : public SGReferenced
{
public:
    // constructor for reading a tile
    SGDemTile( const SGPath& path, int lon, int lat, int w, int h, int x, int y, bool cache );
    // constructor for writing a tile
    SGDemTile( const SGPath& path, int lon, int lat, int w, int h, int x, int y, const SGDemSession& s );
    ~SGDemTile();

    SGPath getPath( void ) const { return path; }
    unsigned short getAlt(const SGGeod& loc) const;

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
};

typedef SGSharedPtr<SGDemTile> SGDemTileRef;
typedef std::map<unsigned long, SGDemTileRef> SGDemCache;

class SGDemLevel
{
public:
    SGDemLevel( int l, const SGPath& p, int w, int h, int x, int y, std::string ext, bool r )
    {
        level = l;
        path = p;
        width = w;
        height = h;
        xres = x;
        yres = y;
        extension = ext;
        ready = r;
    };

    int getLevel( void ) const {
        return level;
    }
    const SGPath& getLevelDir( void ) const {
        return path;
    }
    bool isReady( void ) const {
        return ready;
    }
    int getWidth( void ) const {
        return width;
    }
    int getHeight( void ) const {
        return height;
    }
    int getResX( void ) const {
        return xres;
    }
    int getResY( void ) const {
        return yres;
    }

private:
    SGPath path;
    std::string extension;
    bool ready;

    int level;
    int width;
    int height;
    int xres;
    int yres;
};

class SGDemSession
{
public:
    SGDemSession( int mnLon, int mnLat, int mxLon, int mxLat, int idx, SGDem* p ) {
        minLon = mnLon;
        minLat = mnLat;
        maxLon = mxLon;
        maxLat = mxLat;
        pDem   = p;
        lvlIndex = idx;
    }

    ~SGDemSession() {
        close();
    }

    void addTile(SGDemTileRef pTile) {
        tileRefs.push_back( pTile );
    }

    const std::vector<SGDemTileRef>& getTiles( void ) const {
        return tileRefs;
    }

    unsigned int size( void ) const {
        return tileRefs.size();
    }

    void close( void );

    int getLvlIndex( void ) const {
        return lvlIndex; 
    };

    int getMinLon( void ) const {
        return minLon;
    }

    int getMinLat( void ) const {
        return minLat;
    }

    int getMaxLon( void ) const {
        return maxLon;
    }

    int getMaxLat( void ) const {
        return maxLat;
    }

private:
    int minLon, minLat;
    int maxLon, maxLat;
    SGDem* pDem;
    int    lvlIndex;

    std::vector<SGDemTileRef> tileRefs;
};

class SGDem : public SGReferenced
{
public:
    SGDem() {
        fprintf(stderr, "new DEM\n");
    };
    ~SGDem() {};

    int init( const SGPath& root );

    unsigned short getAlt( const SGDemSession& s, const SGGeod& loc ) const;

    // create a new level
    int createLevel( int w, int h, int x, int y, const std::string& ext );
    SGDemTileRef createTile( int lvl, int lon, int lat, SGDemSession& s );

    // open a dem level - tiles will be read and reference counted until closed
    SGDemSession openSession( const SGGeod& min, const SGGeod& max, bool cache );

    void flushCaches( int lvl );

    int getNumLevels() const {
        return levels.size();
    }
    
private:
    int getBestLevel( int reqArea );

    SGPath                  demRoot;
    std::vector<SGDemLevel> levels;
    std::vector<SGDemCache> caches;
};

typedef SGSharedPtr<SGDem> SGDemPtr;

#endif // _SG_DEM_HXX
