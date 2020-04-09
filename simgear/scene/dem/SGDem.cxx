// SGDem.cxx -- read, write DEM heiarchy
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
#include <fstream>
#include <limits>

#include <cpl_conv.h> // for CPLMalloc()
#include "ogr_spatialref.h"

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/stdint.hxx>
#include <simgear/misc/sg_dir.hxx>

#include <simgear/scene/dem/SGDem.hxx>
#include <simgear/scene/dem/SGDemSession.hxx>

using namespace simgear;

#define DEM_DEBUG   (0)

// where to add these...
int SGDem::floorWithEpsilon(double x)
{
    return static_cast<int>(floor(x + SG_EPSILON));
}

unsigned SGDem::normalizeLongitude(unsigned offset)
{
    return offset - (360*8)*(offset/(360*8));
}

unsigned SGDem::longitudeDegToOffset(double lon)
{
    unsigned offset = (unsigned)( floorWithEpsilon( 8.0 * (lon + 180.0) ) );
    return normalizeLongitude(offset);
}

double SGDem::offsetToLongitudeDeg(unsigned offset)
{
    return offset*0.125 - 180;
}

unsigned SGDem::latitudeDegToOffset(double lat)
{
    if (lat < -90)
        return 0;

    unsigned offset = (unsigned)( floorWithEpsilon( 8.0 * (lat + 90.0) ) );
    if (8*180 < offset)
        return 8*180;

    return offset;
}

double SGDem::offsetToLatitudeDeg(unsigned offset)
{
    return offset*0.125 - 90;
}

int SGDem::addRoot( const SGPath& root )
{
    GDALAllRegister();

    SGDemRoot demRoot( root );

    // collect subdir for each dem level - format is level_X
    if ( root.isDir() ) {
        Dir d(root);
        if (d.exists() ) {
            PathList levelPaths = d.children(Dir::TYPE_DIR);
            SG_LOG( SG_TERRAIN, SG_INFO, levelPaths.size() << " Directories in " << d.path() );

            for(const SGPath& p : levelPaths) {
                std::string prefix;
                int         level;

                std::istringstream iss( p.file() );
                getline(iss, prefix, '_');
                if ( ((iss.rdstate() & std::ifstream::failbit ) == 0 ) &&
                      (prefix == "level" ) ) {
                    iss >> level;

                    // read the deminfo.txt file
                    SGPath infoFile = p / "deminfo.txt";
                    SGPath extentsFile = p / "demextents.bin";

                    if ( infoFile.exists() ) {
                        std::fstream demInfo(infoFile.c_str(), std::ios_base::in);
                        int w, h, x, y, o;
                        std::string ext;
                        unsigned long extents[45][180];

                        demInfo >> w >> h >> x >> y >> o >> ext;

                        // search level for extents
                        memset( (unsigned char*)extents, 0, sizeof(extents) );
                        if ( extentsFile.exists() ) {
                            std::fstream demExtents(extentsFile.c_str(), std::ios_base::in | std::ios_base::binary );

                            for ( unsigned int i=0; i<45; i++ ) {
                                for ( unsigned int j=0; j<180; j++ ) {
                                    demExtents >> extents[i][j];
                                }
                            }
                        }

                        SG_LOG( SG_TERRAIN, SG_INFO, " found DEM level " << level << " in directory " << p << " width : " << w << " height : " << h << " xres : " << x << " yres : " << y << " overlap : " << o << " extension: " << ext );
                        demRoot.addLevel( SGDemLevel( level, p, w, h, x, y, o, &extents[0][0], ext, true ) );

                    } else {
                        SG_LOG( SG_TERRAIN, SG_INFO, " found DEM level " << level << " in directory " << p << " without info file " );
                    }

                } else {
                    SG_LOG( SG_TERRAIN, SG_INFO, " invalid dem level dir " << p << " got prefix " << prefix );
                }
            }
        }
    }

    if ( demRoot.numLevels() ) {
        demRoots.push_back( demRoot );
    }

    return demRoot.numLevels();
}

int SGDem::createRoot( const SGPath& root )
{
    // collect subdir for each dem level - format is level_X
    // create the directory
    SGPath newDir = root / "dummy";
    newDir.create_dir();

    demRoots.push_back( SGDemRoot(root) );

    return 0;
}

unsigned SGDem::roundDown( unsigned offset, unsigned roundTo )
{
    if ( roundTo == 0 ) {
        return offset;
    } else {
        return (offset / roundTo) * roundTo;
    }
}

unsigned SGDem::roundUp( unsigned offset, unsigned roundTo )
{
    if ( roundTo == 0 ) {
        return offset;
    } else {
        return ((offset+roundTo-1)  / roundTo) * roundTo;
    }
}

SGDemSession SGDem::openSession( unsigned wo, unsigned so, unsigned eo, unsigned no, int level, bool cache )
{
    SGDemSession s;

    // Create the session
    SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::OpenSession - level " << level );

    if ( level >= 0 ) {
        SGDemRoot* demRoot = findDem( wo, so, eo, no, level );

        if ( demRoot ) {
            // traverse the demRoots, to see if any have this level
            unsigned w = demRoot->getWidth( level );
            unsigned h = demRoot->getHeight( level );
            unsigned x = demRoot->getResX( level );
            unsigned y = demRoot->getResY( level );
            unsigned o = demRoot->getOverlap( level );

            s = SGDemSession(wo, so, eo, no, level, w, h, demRoot);

            SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::OpenSession - from offsets " << wo << ", " << so << " to offsets " << eo << ", " << no );

            // calc min offsets based on tile widths and height for this level
            unsigned min_lon = roundDown( wo, w );
            unsigned min_lat = roundDown( so, h );
            unsigned max_lon = roundUp( eo, w );
            unsigned max_lat = roundUp( no, h );

            SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::OpenSession - from pre level rounding offsets " << min_lon << ", " << min_lat << " to offsets " << max_lon << ", " << max_lat );

            for (unsigned lon = min_lon; lon < max_lon; lon += w) {
                for (unsigned lat = min_lat; lat < max_lat; lat += h) {
                    s.addTile( demRoot->getOrCreateTile( lon, lat, w, h, x, y, o, level, cache ) );
                }
            }
        } else {
	    SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::OpenSession - could not find DEM for " << wo << ", " << so << " - " << eo << ", " << no << " level " << level );
        }
    } else {
        SG_LOG( SG_TERRAIN, SG_ALERT, "SGDem::OpenSession - invalid level " << level );
    }

    return s;
}

SGDemSession SGDem::openSession( const SGGeod& min, const SGGeod& max, const SGPath& input )
{
#define FP_ROUNDOFF_OUTSIDE     (0.1)

    // create a new demRoot for creation
    SGDemRoot demRoot(input);

    // open all tiles between min and max
    int min_lon = (int)(floor(min.getLongitudeDeg()-FP_ROUNDOFF_OUTSIDE));
    int min_lat = (int)(floor(min.getLatitudeDeg()-FP_ROUNDOFF_OUTSIDE));
    int max_lon = (int)(ceil(max.getLongitudeDeg()+FP_ROUNDOFF_OUTSIDE));
    int max_lat = (int)(ceil(max.getLatitudeDeg()+FP_ROUNDOFF_OUTSIDE));

    // Create the session
    SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::OpenSession - create sesion obj - req from " <<
	min.getLongitudeDeg() << ", " << min.getLatitudeDeg() << " to " <<
	max.getLongitudeDeg() << ", " << max.getLatitudeDeg() << " - getting " <<
	min_lon << ", " << min_lat << " to " << max_lon << ", " << max_lat );

    SGDemSession s(min_lon, min_lat, max_lon, max_lat, &demRoot);

    // todo - read a tile from the input dir
    int w = 1;
    int h = 1;
    int x = 1201;
    int y = 1201;
    int o = 32;

    SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::OpenSession - Traverse tiles");
    for (int lon = min_lon; lon < max_lon; lon += w) {
        for (int lat = min_lat; lat < max_lat; lat += h) {
	    SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::OpenSession - Create tile " <<
		lon << ", " << lat << " from dir " << input );

            unsigned wo = (lon+180)*8;
            unsigned so = (lat+ 90)*8;

            SGDemTile* pTile = new SGDemTile( input, wo, so, w, h, x, y, o, false );
            s.addTile( pTile );
        }
    }

    return s;
}

SGDemRoot* SGDem::findDem( unsigned wo, unsigned so, unsigned eo, unsigned no, int lvl )
{
    SGDemRoot* dr = NULL;

    for ( unsigned int i=0; i<demRoots.size(); i++ )
    {
        if ( demRoots[i].isValid( lvl, wo, so, eo, no ) ) {
            dr = &demRoots[i];
            break;
        } else {
	    SG_LOG( SG_TERRAIN, SG_ALERT, "SGDem::findDem - dem " << i << " is not vald." );
        }
    }

    return dr;
}

#if 0 // todo - move to session
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
            // fprintf( stderr, " Could NOT find tile %d,%d in cache %d key is %08lx\n", intLon, intLat, lvlIndex, key );
            alt = 0;
        }
    }

    return alt;
}
#endif
