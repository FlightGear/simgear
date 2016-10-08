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

#include <iostream>
#include <iomanip>
#include <fstream>
#include <limits>

#include <boost/foreach.hpp>

#include <cpl_conv.h> // for CPLMalloc()
#include "ogr_spatialref.h"

#include <simgear/scene/tgdb/dem.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/misc/sg_dir.hxx>

using namespace simgear;

#define DEM_DEBUG   (0)

// temp
extern int GDALExit( int nCode );

SGDemTile::SGDemTile( const SGPath& levelDir, int lon, int lat, int w, int h, int x, int y, bool cache)
{
    // add the tile name to the level path
    ref_lon = lon;
    ref_lat = lat;
    path    = levelDir / getTileName( lon, lat );
    width   = w;
    height  = h;
    resx    = x;
    resy    = y;

    if ( cache ) {
        raster = cacheTile( path );
    } else {
        raster = NULL;
    }
}

void SGDemTile::dbgDumpDataset( GDALDataset* poDataset ) const
{
    double adfGeoTransform[6];

    SG_LOG( SG_TERRAIN, SG_INFO, "Driver: " << poDataset->GetDriver()->GetDescription() << "/" << poDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );
    SG_LOG( SG_TERRAIN, SG_INFO, "Size is " << poDataset->GetRasterXSize() << " x " << poDataset->GetRasterYSize() << " x " << poDataset->GetRasterCount() );

    if( poDataset->GetProjectionRef() != NULL ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "Projection is " << poDataset->GetProjectionRef() );
    }

    if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "Origin = (" << adfGeoTransform[0] << ", " << adfGeoTransform[3] << ")" );
        SG_LOG( SG_TERRAIN, SG_INFO, "Pixel Size = (" << adfGeoTransform[1] << ", " << adfGeoTransform[5] << ")" );
    }
}

void SGDemTile::dbgDumpBand( GDALRasterBand* poBand ) const
{
    int             nBlockXSize, nBlockYSize;
    int             bGotMin, bGotMax;
    double          adfMinMax[2];

    poBand->GetBlockSize( &nBlockXSize, &nBlockYSize );
    SG_LOG( SG_TERRAIN, SG_INFO, "Block=" << nBlockXSize << " x " << nBlockYSize << " Type=" << GDALGetDataTypeName(poBand->GetRasterDataType()) << "ColorInterp=" << GDALGetColorInterpretationName( poBand->GetColorInterpretation()) );

    adfMinMax[0] = poBand->GetMinimum( &bGotMin );
    adfMinMax[1] = poBand->GetMaximum( &bGotMax );
    if( ! (bGotMin && bGotMax) ) {
        GDALComputeRasterMinMax((GDALRasterBandH)poBand, TRUE, adfMinMax);
    }
    SG_LOG( SG_TERRAIN, SG_INFO, "Min=" << adfMinMax[0] << ", Max=" << adfMinMax[1] );
    if( poBand->GetOverviewCount() > 0 ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "Band has " << poBand->GetOverviewCount() << " overviews." );
    }
    if( poBand->GetColorTable() != NULL ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "Band has a color table with " << poBand->GetColorTable()->GetColorEntryCount() << " entries." );
    }
}

unsigned short* SGDemTile::cacheTile( const SGPath& path )
{
    GDALDataset*    poDataset = (GDALDataset *)GDALOpen( path.c_str(), GA_ReadOnly );
    GDALRasterBand* poBand = NULL;
    unsigned short* r = NULL;

    if ( poDataset ) {

#if DEM_DEBUG
        dbgDumpDataset( poDataset );
#endif

        unsigned int numRasters = poDataset->GetRasterCount();
        //fprintf( stderr, "Created tile from file %s : %d by %d handle is %p. num raster bands is %d\n", path.c_str(), resx, resy, poDataset, numRasters );

        // read the bands into array
        for ( unsigned int rb=1; rb<=numRasters; rb++ ) {
            poBand = poDataset->GetRasterBand(rb);

#if DEM_DEBUG
            dbgDumpBand( poBand );
#endif

            // if this is the raster we're looking for
            if ( rb == 1 ) {
                int   nXSize = poBand->GetXSize();
                int   nYSize = poBand->GetYSize();
                // SG_LOG( SG_TERRAIN, SG_INFO, "Reading raster " << rb << " (" << nXSize << "x" << nYSize << ")" );

                r = (unsigned short *)CPLMalloc(sizeof(unsigned short)*nXSize*nYSize);
                //fprintf( stderr, "reading raster size %d x %d - buffer is %p\n", nXSize, nYSize, r );
                poBand->RasterIO( GF_Read, 0, 0, nXSize, nYSize, r, nXSize, nYSize, GDT_UInt16, 0, 0 );
            }
        }

        GDALClose( poDataset );
    } else {
        printf("SGDemTile::SGDemTile (read ) - could not open file %s\n", path.c_str() );
    }

    return r;
}

// Create a new DEM tile from multiple source tiles ( at an expected lower resolution )
// code based on gdalwarp, but with most options removed.
// example dgalwarp usage this function is based on:
// gdalwarp -r cubic -ts 1201 1201 -te -85.0 32.0 -83.0 34.0 -dstnodata 0 -co "COMPRESS=DEFLATE" temp/N32W085.hgt temp/merged_cubic.tiff
SGDemTile::SGDemTile( const SGPath& levelDir, int lon, int lat, int w, int h, int x, int y, const SGDemSession& s)
{
    // add the tile name to the level path
    ref_lon = lon;
    ref_lat = lat;
    path    = levelDir / getTileName( lon, lat );
    width   = w;
    height  = h;
    resx    = x;
    resy    = y;

    raster = NULL;

    // use gdal warp api to generate tile from session
    char** papszSrcFiles = NULL;
    char** papszWarpOptions = NULL;
    char** papszTO = NULL;

    double  dfMinX=0.0, dfMinY=0.0, dfMaxX=0.0, dfMaxY=0.0;  // -te
    int     nForcePixels = 1201, nForceLines = 1201;         // -ts

    // -dstnodata
    papszWarpOptions = CSLSetNameValue(papszWarpOptions, "INIT_DEST", "0");

    // target extents
    dfMinX = (double)ref_lon;
    dfMinY = (double)ref_lat;
    dfMaxX = dfMinX + (double)w;
    dfMaxY = dfMinY + (double)h;

    // generate list of source files in session
    const std::vector<SGDemTileRef>& tiles = s.getTiles();
    for ( unsigned int i=0; i<tiles.size(); i++ ) {
        // check if the file exists
        if ( tiles[i]->getPath().exists() ) {
            papszSrcFiles = CSLAddString( papszSrcFiles, tiles[i]->getPath().c_str() );
        }
    }

    // create output
    if ( papszSrcFiles ) {
        GDALDatasetH hDstDS = createTile( papszSrcFiles, path.c_str(), nForceLines, nForcePixels,
                                          dfMinX, dfMinY, dfMaxX, dfMaxY,
                                          papszTO );

        if( hDstDS == NULL ) {
            GDALExit( 1 );
        }

        /* -------------------------------------------------------------------- */
        /*      Loop over all source files, processing each in turn.            */
        /* -------------------------------------------------------------------- */
        int iSrc;
        for( iSrc = 0; papszSrcFiles[iSrc] != NULL; iSrc++ )
        {
            doWarp( iSrc, papszSrcFiles[iSrc], hDstDS, papszTO, papszWarpOptions );
        }

        /* -------------------------------------------------------------------- */
        /*      Final Cleanup.                                                  */
        /* -------------------------------------------------------------------- */
        CPLErrorReset();
        GDALFlushCache( hDstDS );
        GDALClose( hDstDS );

        CSLDestroy( papszSrcFiles );
        CSLDestroy( papszWarpOptions );
        CSLDestroy( papszTO );

        GDALDumpOpenDatasets( stderr );
    }
}

SGDemTile::~SGDemTile()
{
    // free the raster
    if ( raster ) {
        CPLFree( raster );
    }
}

unsigned short SGDemTile::getAlt( const SGGeod& loc ) const
{
    if ( raster ) {
        // get lon and lat reletive to sw corner
        double offLon = loc.getLongitudeDeg() - (double)ref_lon;
        double offLat = loc.getLatitudeDeg()  - (double)ref_lat;

        double fractLon = offLon / (double)width;
        double fractLat = offLat / (double)height;

        int l = (int)( (double)(resy-1) - ((double)(resy-1)*fractLat) );
        int p = (int)( (double)(resx-1) * fractLon );

        return raster[l*resx+p];
    } else {
        return 0;
    }
}

std::string SGDemTile::getTileName( int lon, int lat )
{
    std::stringstream ss;
    if ( lat >= 0 ) {
        ss << "N" << std::setw(2) << std::setfill('0') << lat;
    } else {
        ss << "S" << std::setw(2) << std::setfill('0') << -lat;
    }
    if ( lon >= 0 ) {
        ss << "E" << std::setw(3) << std::setfill('0') << lon;
    } else {
        ss << "W" << std::setw(3) << std::setfill('0') << -lon;
    }
    ss << ".hgt";

    return ss.str();
}


int SGDem::init( const SGPath& root )
{
    GDALAllRegister();
    levels.clear();
    caches.clear();

    // collect subdir for each dem level - format is level_X
    if ( root.isDir() ) {
        demRoot = root;

        Dir d(demRoot);
        if (d.exists() ) {
            PathList levelPaths = d.children(Dir::TYPE_DIR);
            SG_LOG( SG_TERRAIN, SG_INFO, levelPaths.size() << " Directories in " << d.path() );

            BOOST_FOREACH(const SGPath& p, levelPaths) {
                std::string prefix;
                int         level;

                std::istringstream iss( p.file() );
                getline(iss, prefix, '_');
                if ( ((iss.rdstate() & std::ifstream::failbit ) == 0 ) && 
                      (prefix == "level" ) ) {
                    iss >> level;

                    // read the deminfo.txt file
                    SGPath infoFile = p / "deminfo.txt";
                    if ( infoFile.exists() ) {
                        std::fstream demInfo(infoFile.c_str(), std::ios_base::in);
                        int w, h, x, y;
                        std::string ext;
                        demInfo >> w >> h >> x >> y >> ext;

                        SG_LOG( SG_TERRAIN, SG_INFO, " found DEM level " << level << " in directory " << p << " width : " << w << " height : " << h << " xres : " << x << " yres : " << y << " extension: " << ext );
                        levels.push_back( SGDemLevel( level, p, w, h, x, y, ext, true ) );
                        caches.push_back( SGDemCache() );
                    } else {
                        SG_LOG( SG_TERRAIN, SG_INFO, " found DEM level " << level << " in directory " << p << " without info file " );
                    }

                } else {
                    SG_LOG( SG_TERRAIN, SG_INFO, " invalid dem level dir " << p << " got prefix " << prefix );
                }
            }
        }
    }

    return levels.size();
}

// this function seeks to find the layer closest to the requested size
int SGDem::getBestLevel( int reqArea )
{
    int lowerArea  = std::numeric_limits<int>::min();
    int higherArea = std::numeric_limits<int>::max();
    int lowLvl = -1, highLvl = -1, bestLevel = -1;

    for ( unsigned int i=0; i<levels.size(); i++ ) {
        // only check areas that have been populated
        if ( levels[i].isReady() ) {
            int levelArea = levels[i].getWidth() * levels[i].getHeight();

            // if the level is equal to requested area - we're done
            if ( levelArea == reqArea ) {
                bestLevel = i;
                break;
            }

            if ( (levelArea < reqArea) && (levelArea > lowerArea) ) {
                lowerArea = levelArea;
                lowLvl    = i;
            }

            if ( (levelArea > reqArea) && (levelArea < higherArea) ) {
                higherArea = levelArea;
                highLvl    = i;
            }
        }
    }

    if ( bestLevel < 0 ) {
        // we want to take the closer of the low/high level
        double lowRatio  = (double)reqArea/(double)lowerArea;
        double highRatio = (double)higherArea/(double)reqArea;

        if ( lowRatio < highRatio ) {
            bestLevel = lowLvl;
        } else {
            bestLevel = highLvl;
        }
    } else {
        //fprintf(stderr, "Found perfect level for reqArea %d\n", reqArea );
    }

    return bestLevel;
}

SGDemSession SGDem::openSession( const SGGeod& min, const SGGeod& max, bool cache )
{
#define FP_ROUNDOFF     (0.0000000001)

    // open all tiles between min and max
    int min_lon = (int)(floor(min.getLongitudeDeg()+FP_ROUNDOFF));
    int min_lat = (int)(floor(min.getLatitudeDeg()+FP_ROUNDOFF));
    int max_lon = (int)(ceil(max.getLongitudeDeg()-FP_ROUNDOFF));
    int max_lat = (int)(ceil(max.getLatitudeDeg()-FP_ROUNDOFF));

    // get either requested, or 'best' level
    // get the size of the requested area in integer degrees
    int areaWidth  = max_lon - min_lon;
    int areaHeight = max_lat - min_lat;

    int level = getBestLevel( areaWidth * areaHeight );

    // Create the session 
    SGDemSession s(min_lon, min_lat, max_lon, max_lat, level, this);
    if ( level >= 0 ) {
        int w = levels[level].getWidth();
        int h = levels[level].getHeight();
        int x = levels[level].getResX();
        int y = levels[level].getResY();

        for (int lon = min_lon; lon < max_lon; lon += w) {
            for (int lat = min_lat; lat < max_lat; lat += h) {
                unsigned long key = (lon + 180) << 16 | (lat + 90);

                // is this tile already loaded?
                SGDemCache::const_iterator it = caches[level].find(key);
                if ( it != caches[level].end() ) {
                    // yes - add the reference to this session
                    //printf( "********************** adding ref to tile at %d,%d with key %lx\n", lon, lat, key );
                    s.addTile( it->second );
                } else {
                    // no load the tile, now
                    //printf( "********************** create tile at %d,%d with key %lx\n", lon, lat, key );
                    SGDemTile* pTile = new SGDemTile( levels[level].getLevelDir(), 
                                                    lon, lat, w, h, x, y, cache );
                    caches[level][key] = pTile;
                    s.addTile( pTile );
                }
            }
        }
    }

    return s;
}

int SGDem::createLevel( int w, int h, int x, int y, const std::string& ext )
{
    int lvlidx = -1;

    std::stringstream ss;
    ss << "level_" << std::setw(2) << std::setfill('0') << levels.size()+1;

    // see if dir already exists
    SGPath lvlPath = demRoot / ss.str();
    if ( lvlPath.isDir() ) {
        SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::createLevel: found existing level directory at " << lvlPath.c_str() );
    } else {
        SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::createLevel: creating level directory at " << lvlPath.c_str() );

        // create the dir - needs the filename to do this !?!?
        SGPath infoFile = lvlPath / "deminfo.txt";
        int res = infoFile.create_dir();
        if ( res == 0 ) {
            SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::createLevel: successfully created directory at " << infoFile.realpath().c_str() << " error " << res );

            lvlidx = levels.size();

            std::fstream demInfo(infoFile.c_str(), std::ios_base::out);
            demInfo << w << " " << h << " " << x << " " << y << " " << ext << std::endl;

            levels.push_back( SGDemLevel( lvlidx-1, lvlPath, w, h, x, y, ext, false ) );
            caches.push_back( SGDemCache() );
        } else {
            SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::createLevel: can't create level directory at " << lvlPath.c_str() << " error " << res );
        }
    }

    return lvlidx;
}

SGDemTileRef SGDem::createTile( int lvl, int lon, int lat, SGDemSession& s )
{
    SGDemTileRef rTile = NULL;

    if ( lvl >= 0 ) {
        int w = levels[lvl].getWidth();
        int h = levels[lvl].getHeight();
        int x = levels[lvl].getResX();
        int y = levels[lvl].getResY();

        rTile = new SGDemTile( levels[lvl].getLevelDir(), 
                               lon, lat,
                               w, h, x, y, s );
    }

    return rTile;
}

void SGDem::flushCaches( int lvl )
{
    // traverse the map - delete any tiles with ref count == 1 ( us )
    //fprintf( stderr, "flush caches in lvl %d - num tiles is %lu\n", lvl, caches[lvl].size() );
    SGDemCache::iterator it = caches[lvl].begin();
    while ( it != caches[lvl].end() ) {
        if ( it->second.getNumRefs() == 1 ) {
            caches[lvl].erase(it++);
        } else {
            //fprintf( stderr, "can't flush tile - numrefs is %u\n", it->second.getNumRefs() );
            ++it;
        }
    }
}

int roundDown(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = abs(numToRound) % multiple;
    if (remainder == 0)
        return numToRound;

    if (numToRound < 0)
        return -(abs(numToRound) - remainder + multiple);
    else
        return numToRound - remainder;
}

unsigned short SGDem::getAlt( const SGDemSession& s, const SGGeod& loc ) const
{
    int alt = 0;

    // which level index are we querying?
    int lvlIndex = s.getLvlIndex();

    if ( lvlIndex >= 0 ) {
        // be careful with coordinates that lie on session boundaries - 
        // on min, ok.
        // on max - make sure we select the tile in the session...
        int lvlWidth  = levels[lvlIndex].getWidth();
        int lvlHeight = levels[lvlIndex].getHeight();

        int intLon, intLat;

        // we need to find the correct tile.
        // shift by 180, 90 to use 0 based ints
        intLon = (int)(round(loc.getLongitudeDeg())) + 180;
        intLon = (intLon / lvlWidth) * lvlWidth;
        intLon = intLon - 180;
        if ( intLon == s.getMaxLon() ) {
            intLon -= levels[lvlIndex].getWidth();
        }

        intLat = (int)(round(loc.getLatitudeDeg())) + 90;
        intLat = (intLat / lvlHeight) * lvlHeight;
        intLat = intLat - 90;
        if ( intLat == s.getMaxLat() ) {
            intLat -= levels[lvlIndex].getHeight();
        }

        unsigned long key = (intLon + 180) << 16 | (intLat + 90);

        SGDemCache::const_iterator it = caches[lvlIndex].find(key);
        if ( it != caches[lvlIndex].end() ) {
            SGDemTileRef tile = it->second;
            alt = tile->getAlt( loc );
        } else {
            fprintf( stderr, " Could NOT find tile %d,%d in cache %d key is %08lx\n", intLon, intLat, lvlIndex, key );
            fprintf( stderr, " session min %d,%d, max %d,%d requested loc %lf,%lf\n", s.getMinLon(), s.getMinLat(), s.getMaxLon(), s.getMaxLat(), loc.getLongitudeDeg(), loc.getLatitudeDeg() );
            exit(0);
        }
    }

    return alt;
}

void SGDemSession::close( void ) 
{
    if ( tileRefs.size() ) {
        tileRefs.clear();
        pDem->flushCaches( lvlIndex );
    }
}
