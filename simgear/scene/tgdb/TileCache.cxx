// newcache.cxx -- routines to handle scenery tile caching
//
// Written by Curtis Olson, started December 2000.
//
// Copyright (C) 2000  Curtis L. Olson  - http://www.flightgear.org/~curt
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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/bucket/newbucket.hxx>
#include <simgear/debug/logstream.hxx>
#include <simgear/misc/sg_path.hxx>

#include "TileEntry.hxx"
#include "TileCache.hxx"

using simgear::TileEntry;
using simgear::TileCache;

TileCache::TileCache( void ) :
    max_cache_size(100)
{
    tile_cache.clear();
}


TileCache::~TileCache( void ) {
    clear_cache();
}


// Free a tile cache entry
void TileCache::entry_free( long cache_index ) {
    SG_LOG( SG_TERRAIN, SG_DEBUG, "FREEING CACHE ENTRY = " << cache_index );
    TileEntry *tile = tile_cache[cache_index];
    tile->removeFromSceneGraph();

    tile->free_tile();
    delete tile;

    tile_cache.erase( cache_index );
}


// Initialize the tile cache subsystem
void TileCache::init( void ) {
    SG_LOG( SG_TERRAIN, SG_INFO, "Initializing the tile cache." );

    SG_LOG( SG_TERRAIN, SG_INFO, "  max cache size = "
            << max_cache_size );
    SG_LOG( SG_TERRAIN, SG_INFO, "  current cache size = "
            << tile_cache.size() );

#if 0 // don't clear the cache
    clear_cache();
#endif

    SG_LOG( SG_TERRAIN, SG_INFO, "  done with init()"  );
}


// Search for the specified "bucket" in the cache
bool TileCache::exists( const SGBucket& b ) const {
    long tile_index = b.gen_index();
    const_tile_map_iterator it = tile_cache.find( tile_index );

    return ( it != tile_cache.end() );
}


// Return the index of the oldest tile in the cache, return -1 if
// nothing available to be removed.
long TileCache::get_oldest_tile() {
    // we need to free the furthest entry
    long min_index = -1;
    double timestamp = 0.0;
    double min_time = DBL_MAX;

    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();

    for ( ; current != end; ++current ) {
        long index = current->first;
        TileEntry *e = current->second;
        if ( e->is_loaded() ) {
            timestamp = e->get_timestamp();
            if ( timestamp < min_time ) {
                min_time = timestamp;
                min_index = index;
            }
        } else {
            SG_LOG( SG_TERRAIN, SG_DEBUG, "loaded = " << e->is_loaded()
                    << " time stamp = " << e->get_timestamp() );
        }
    }

    SG_LOG( SG_TERRAIN, SG_DEBUG, "    index = " << min_index );
    SG_LOG( SG_TERRAIN, SG_DEBUG, "    min_time = " << min_time );

    return min_index;
}


// Clear the inner ring flag for all tiles in the cache so that the
// external tile scheduler can flag the inner ring correctly.
void TileCache::clear_inner_ring_flags() {
    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();

    for ( ; current != end; ++current ) {
        TileEntry *e = current->second;
        if ( e->is_loaded() ) {
            e->set_inner_ring( false );
        }
    }
}

// Clear a cache entry, note that the cache only holds pointers
// and this does not free the object which is pointed to.
void TileCache::clear_entry( long cache_index ) {
    tile_cache.erase( cache_index );
}


// Clear all completely loaded tiles (ignores partially loaded tiles)
void TileCache::clear_cache() {
    std::vector<long> indexList;
    tile_map_iterator current = tile_cache.begin();
    tile_map_iterator end = tile_cache.end();

    for ( ; current != end; ++current ) {
        long index = current->first;
        SG_LOG( SG_TERRAIN, SG_DEBUG, "clearing " << index );
        TileEntry *e = current->second;
        if ( e->is_loaded() ) {
            e->tile_bucket.make_bad();
            // entry_free modifies tile_cache, so store index and call entry_free() later;
            indexList.push_back( index);
        }
    }
    for (unsigned int it = 0; it < indexList.size(); it++) {
        entry_free( indexList[ it]);
    }
}

/**
 * Create a new tile and schedule it for loading.
 */
bool TileCache::insert_tile( TileEntry *e ) {
    // register tile in the cache
    long tile_index = e->get_tile_bucket().gen_index();
    tile_cache[tile_index] = e;

    return true;
}

