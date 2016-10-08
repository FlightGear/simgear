#ifndef __SG_DEM_ROOT_HXX__
#define __SG_DEM_ROOT_HXX__

#include <simgear/scene/dem/SGDemLevel.hxx>
#include <simgear/scene/dem/SGDemTile.hxx>

class SGDemRoot
{
public:
    SGDemRoot( SGPath p ) {
        demRoot = p;
    }

    void addLevel( const SGDemLevel& level ) {
        levels.push_back( level );
        caches.push_back( SGDemCache() );
    }

    // create a new level
    int createLevel( int w, int h, int x, int y, int overlap, const std::string& ext );
    SGDemTileRef createTile( int lvl, int lon, int lat, int overlap, SGDemSession& s );
    void closeLevel( int lvl );

    unsigned int numLevels( void ) const {
        return levels.size();
    }

    SGDemTileRef getTile( int lvlIndex, unsigned long key );

    void flushCaches( int lvl );

    bool isValid( int lvl, unsigned wo, unsigned so, unsigned eo, unsigned no ) const;

    unsigned int getWidth( unsigned int level ) {
        if ( level < levels.size() ) {
            return levels[level].getWidth() * 8;
        } else {
            return 0;
        }
    }

    unsigned int getHeight( unsigned int level ) {
        if ( level < levels.size() ) {
            return levels[level].getHeight() * 8;
        } else {
            return 0;
        }
    }

    unsigned int getResX( unsigned int level ) {
        if ( level < levels.size() ) {
            return levels[level].getResX();
        } else {
            return 0;
        }
    }

    unsigned int getResY( unsigned int level ) {
        if ( level < levels.size() ) {
            return levels[level].getResY();
        } else {
            return 0;
        }
    }

    unsigned int getOverlap( unsigned int level ) {
        if ( level < levels.size() ) {
            return levels[level].getOverlap();
        } else {
            return 0;
        }
    }

    SGDemTile* getOrCreateTile( unsigned wo, unsigned so,
                                unsigned w, unsigned h, unsigned x, unsigned y,
                                unsigned o, int level, bool cache )
    {
        unsigned long key = wo << 16 | so;

        // is this tile already loaded?
        SGDemCache::const_iterator it = caches[level].find(key);
        if ( it != caches[level].end() ) {
            // yes - add the reference to this session
            //printf( "********************** adding ref to tile at %d,%d with key %lx\n", lon, lat, key );
            return it->second;
        } else {
            // no load the tile, now
            SGDemTile* pTile = new SGDemTile( levels[level].getLevelDir(),
                                              wo, so, w, h,
                                              x, y, o, cache );
            caches[level][key] = pTile;
            return pTile;
        }
    }

private:
    SGPath                  demRoot;
    std::vector<SGDemLevel> levels;
    std::vector<SGDemCache> caches;
};

#endif /* __SG_DEM_ROOT_HXX__ */
