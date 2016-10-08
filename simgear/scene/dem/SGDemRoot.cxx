#include <iomanip>
#include <fstream>
#include <sstream>

#include <simgear/misc/sg_dir.hxx>
#include <simgear/scene/dem/SGDemRoot.hxx>
#include <simgear/debug/logstream.hxx>

bool SGDemRoot::isValid( int lvl, unsigned wo, unsigned so, unsigned eo, unsigned no ) const
{
    bool valid = false;

    // check if we have requested level
    if ( (unsigned int)lvl < levels.size() ) {
        // check if we fit in this levels extents
        valid = levels[lvl].isValid( wo, so, eo, no );
    } else {
        SG_LOG( SG_TERRAIN, SG_INFO, "SGDemRoot::isValid - lvl " << lvl << " is greater than #levels in root " << levels.size() );
    }

    return valid;
}

int SGDemRoot::createLevel( int w, int h, int x, int y, int overlap, const std::string& ext )
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
            demInfo << w << " " << h << " " << x << " " << y << " " << overlap << " " << ext << std::endl;

            levels.push_back( SGDemLevel( lvlidx, lvlPath, w, h, x, y, overlap, ext, false ) );
            caches.push_back( SGDemCache() );
        } else {
            SG_LOG( SG_TERRAIN, SG_INFO, "SGDem::createLevel: can't create level directory at " << lvlPath.c_str() << " error " << res );
        }
    }

    return lvlidx;
}

void SGDemRoot::closeLevel( int lvl )
{
    if ( (unsigned int)lvl < levels.size() ) {
        levels[lvl].close();
    }
}

SGDemTileRef SGDemRoot::createTile( int lvl, int lon, int lat, int overlap, SGDemSession& s )
{
    SGDemTileRef rTile = NULL;
    bool bWritten = false;

    if ( lvl >= 0 ) {
        int w = levels[lvl].getWidth();
        int h = levels[lvl].getHeight();
        int x = levels[lvl].getResX();
        int y = levels[lvl].getResY();

        unsigned wo = (unsigned)(lon+180)*8;
        unsigned so = (unsigned)(lat+ 90)*8;

        rTile = new SGDemTile( levels[lvl].getLevelDir(),
                               wo, so,
                               w, h, x, y, overlap, s, bWritten );

        if ( bWritten ) {
            fprintf( stderr, "CreateTile - add extent (lat%d+h%d+90)*8-1 is %u\n", lat, h, (lat+h+90)*8-1 );
            levels[lvl].addExtent( (lon+180)*8, (lat+90)*8, (lon+w+180)*8-1, (lat+h+90)*8-1 );
        }
    }

    return rTile;
}

void SGDemRoot::flushCaches( int lvl )
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

SGDemTileRef SGDemRoot::getTile( int lvlIndex, unsigned long key )
{
    SGDemTileRef tile = NULL;

    SGDemCache::const_iterator it = caches[lvlIndex].find(key);
    if ( it != caches[lvlIndex].end() ) {
        tile = it->second;
    }

    return tile;
}
