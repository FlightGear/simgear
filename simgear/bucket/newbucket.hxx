/**************************************************************************
 * newbucket.hxx -- new bucket routines for better world modeling
 *
 * Written by Curtis L. Olson, started February 1999.
 *
 * Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * $Id$
 **************************************************************************/

/** \file newbucket.hxx
 * A class and associated utiltity functions to manage world scenery tiling.
 *
 * Tile borders are aligned along circles of latitude and longitude.
 * All tiles are 1/8 degree of latitude high and their width in degrees
 * longitude depends on their latitude, adjusted in such a way that
 * all tiles cover about the same amount of area of the earth surface.
 */

#ifndef _NEWBUCKET_HXX
#define _NEWBUCKET_HXX

#include <simgear/compiler.h>
#include <simgear/constants.h>
#include <simgear/math/SGMath.hxx>

#include <cmath>
#include <string>
#include <iosfwd>
#include <vector>

// #define NO_DEPRECATED_API

/**
 * standard size of a bucket in degrees (1/8 of a degree)
 */
#define SG_BUCKET_SPAN      0.125

/**
 * half of a standard SG_BUCKET_SPAN
 */
#define SG_HALF_BUCKET_SPAN ( 0.5 * SG_BUCKET_SPAN )


// return the horizontal tile span factor based on latitude
static double sg_bucket_span( double l ) {
    if ( l >= 89.0 ) {
	return 12.0;
    } else if ( l >= 86.0 ) {
	return 4.0;
    } else if ( l >= 83.0 ) {
	return 2.0;
    } else if ( l >= 76.0 ) {
	return 1.0;
    } else if ( l >= 62.0 ) {
	return 0.5;
    } else if ( l >= 22.0 ) {
	return 0.25;
    } else if ( l >= -22.0 ) {
	return 0.125;
    } else if ( l >= -62.0 ) {
	return 0.25;
    } else if ( l >= -76.0 ) {
	return 0.5;
    } else if ( l >= -83.0 ) {
	return 1.0;
    } else if ( l >= -86.0 ) {
	return 2.0;
    } else if ( l >= -89.0 ) {
	return 4.0;
    } else {
	return 12.0;
    }
}


/**
 * A class to manage world scenery tiling.
 * This class encapsulates the world tiling scheme.  It provides ways
 * to calculate a unique tile index from a lat/lon, and it can provide
 * information such as the dimensions of a given tile.
 */

class SGBucket {

private:
    short lon;        // longitude index (-180 to 179)
    short lat;        // latitude index (-90 to 89)
    unsigned char x;          // x subdivision (0 to 7)
    unsigned char y;          // y subdivision (0 to 7)

    void innerSet( double dlon, double dlat );
public:

    /**
     * Default constructor, creates an invalid SGBucket
     */
    SGBucket();

    /**
     * Check if this bucket refers to a valid tile, or not.
     */
    bool isValid() const;
    
#ifndef NO_DEPRECATED_API
    /**
     * Construct a bucket given a specific location.
     * @param dlon longitude specified in degrees
     * @param dlat latitude specified in degrees
     */
    SGBucket(const double dlon, const double dlat);
#endif
    
    /**
     * Construct a bucket given a specific location.
     *
     * @param geod Geodetic location
     */
    SGBucket(const SGGeod& geod);

    /** Construct a bucket given a unique bucket index number.
     *
     * @param bindex unique bucket index
     */
    SGBucket(const long int bindex);

#ifndef NO_DEPRECATED_API
    /**
     * Reset a bucket to represent a new location.
     *
     * @param geod New geodetic location
     */
    void set_bucket(const SGGeod& geod);


    /**
     * Reset a bucket to represent a new lat and lon
     * @param dlon longitude specified in degrees
     * @param dlat latitude specified in degrees
     */
    void set_bucket( double dlon, double dlat );
#endif
    
    /**
     * Create an impossible bucket.
     * This is useful if you are comparing cur_bucket to last_bucket
     * and you want to make sure last_bucket starts out as something
     * impossible.
     */
    void make_bad();
    
    /**
     * Generate the unique scenery tile index for this bucket
     *
     * The index is constructed as follows:
     * 
     * 9 bits - to represent 360 degrees of longitude (-180 to 179)
     * 8 bits - to represent 180 degrees of latitude (-90 to 89)
     *
     * Each 1 degree by 1 degree tile is further broken down into an 8x8
     * grid.  So we also need:
     *
     * 3 bits - to represent x (0 to 7)
     * 3 bits - to represent y (0 to 7)
     * @return tile index
     */
    inline long int gen_index() const {
	return ((lon + 180) << 14) + ((lat + 90) << 6) + (y << 3) + x;
    }

    /**
     * Generate the unique scenery tile index for this bucket in ascii
     * string form.
     * @return tile index in string form
     */
    std::string gen_index_str() const;
    
    /**
     * Build the base path name for this bucket.
     * @return base path in string form
     */
    std::string gen_base_path() const;

    /**
     * @return the center lon of a tile.
     */
    inline double get_center_lon() const {
	double span = sg_bucket_span( lat + y / 8.0 + SG_HALF_BUCKET_SPAN );

	if ( span >= 1.0 ) {
	    return lon + get_width() / 2.0;
	} else {
	    return lon + x * span + get_width() / 2.0;
	}
    }

    /**
     * @return the center lat of a tile.
     */
    inline double get_center_lat() const {
	return lat + y / 8.0 + SG_HALF_BUCKET_SPAN;
    }

    /**
     * @return the highest (furthest from the equator) latitude of this
     * tile. This is the top edge for tiles north of the equator, and
     * the bottom edge for tiles south
     */
    double get_highest_lat() const;
    
    /**
     * @return the width of the tile in degrees.
     */
    double get_width() const;

    /**
     * @return the height of the tile in degrees.
     */
    double get_height() const;

    /**
     * @return the width of the tile in meters.
     */
    double get_width_m() const; 

    /**
     * @return the height of the tile in meters.
     */
    double get_height_m() const;

    /**
     * @return the center of the bucket in geodetic coordinates.
     */
    SGGeod get_center() const
    { return SGGeod::fromDeg(get_center_lon(), get_center_lat()); }

    /**
     * @return the center of the bucket in geodetic coordinates.
     */
    SGGeod get_corner(unsigned num) const
    {
        double lonFac = ((num + 1) & 2) ? 0.5 : -0.5;
        double latFac = ((num    ) & 2) ? 0.5 : -0.5;
        return SGGeod::fromDeg(get_center_lon() + lonFac*get_width(),
                               get_center_lat() + latFac*get_height());
    }

    // Informational methods.

    /**
     * @return the lon of the lower left corner of 
     * the 1x1 chunk containing this tile.
     */
    inline int get_chunk_lon() const { return lon; }

    /**
     * @return the lat of the lower left corner of 
     * the 1x1 chunk containing this tile.
     */
    inline int get_chunk_lat() const { return lat; }

    /**
     * @return the x coord within the 1x1 degree chunk this tile.
     */
    inline int get_x() const { return x; }

    /**
     * @return the y coord within the 1x1 degree chunk this tile.
     */
    inline int get_y() const { return y; }

    /**
     * @return bucket offset from this by dx,dy
     */
    SGBucket sibling(int dx, int dy) const;
    
    // friends

    friend std::ostream& operator<< ( std::ostream&, const SGBucket& );
    friend bool operator== ( const SGBucket&, const SGBucket& );
};

inline bool operator!= (const SGBucket& lhs, const SGBucket& rhs)
    {
        return !(lhs == rhs);
    }

#ifndef NO_DEPRECATED_API
/**
 * \relates SGBucket
 * Return the bucket which is offset from the specified dlon, dlat by
 * the specified tile units in the X & Y direction.
 * @param dlon starting lon in degrees
 * @param dlat starting lat in degrees
 * @param x number of bucket units to offset in x (lon) direction
 * @param y number of bucket units to offset in y (lat) direction
 * @return offset bucket
 */
SGBucket sgBucketOffset( double dlon, double dlat, int x, int y );
#endif


/**
 * \relates SGBucket
 * Calculate the offset between two buckets (in quantity of buckets).
 * @param b1 bucket 1
 * @param b2 bucket 2
 * @param dx offset distance (lon) in tile units
 * @param dy offset distance (lat) in tile units
 */
void sgBucketDiff( const SGBucket& b1, const SGBucket& b2, int *dx, int *dy );


/**
 * \relates SGBucket
 * retrieve a list of buckets in the given bounding box
 * @param min min lon,lat of bounding box in degrees
 * @param max max lon,lat of bounding box in degrees
 * @param list standard vector of buckets within the bounding box
 */
void sgGetBuckets( const SGGeod& min, const SGGeod& max, std::vector<SGBucket>& list );

/**
 * Write the bucket lon, lat, x, and y to the output stream.
 * @param out output stream
 * @param b bucket
 */
std::ostream& operator<< ( std::ostream& out, const SGBucket& b );

/**
 * Compare two bucket structures for equality.
 * @param b1 bucket 1
 * @param b2 bucket 2
 * @return comparison result
 */
inline bool
operator== ( const SGBucket& b1, const SGBucket& b2 )
{
    return ( b1.lon == b2.lon &&
	     b1.lat == b2.lat &&
	     b1.x == b2.x &&
	     b1.y == b2.y );
}


#endif // _NEWBUCKET_HXX


