/**************************************************************************
 * newbucket.hxx -- new bucket routines for better world modeling
 *
 * Written by Curtis L. Olson, started February 1999.
 *
 * Copyright (C) 1999  Curtis L. Olson - curt@flightgear.org
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * $Id$
 **************************************************************************/

/** \file newbucket.hxx
 * A class and associated utiltity functions to manage world scenery tiling.
 */

#ifndef _NEWBUCKET_HXX
#define _NEWBUCKET_HXX

#include <simgear/compiler.h>
#include <simgear/constants.h>

#ifdef SG_HAVE_STD_INCLUDES
#  include <cmath>
#  include <cstdio> // sprintf()
#else
#  include <math.h>
#  include <stdio.h> // sprintf()
#endif

#include STL_IOSTREAM

// I don't understand ... <math.h> or <cmath> should be included
// already depending on how you defined SG_HAVE_STD_INCLUDES, but I
// can go ahead and add this -- CLO
#ifdef __MWERKS__
SG_USING_STD(sprintf);
SG_USING_STD(fabs);
#endif

#include STL_STRING

SG_USING_STD(string);
SG_USING_STD(ostream);


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
	return 360.0;
    } else if ( l >= 88.0 ) {
	return 8.0;
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
    } else if ( l >= -88.0 ) {
	return 4.0;
    } else if ( l >= -89.0 ) {
	return 8.0;
    } else {
	return 360.0;
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
    double cx, cy;  // centerpoint (lon, lat) in degrees of bucket
    int lon;        // longitude index (-180 to 179)
    int lat;        // latitude index (-90 to 89)
    int x;          // x subdivision (0 to 7)
    int y;          // y subdivision (0 to 7)

public:

    /**
     * Default constructor.
     */
    SGBucket();

    /**
     * Construct a bucket given a specific location.
     * @param dlon longitude specified in degrees
     * @param dlat latitude specified in degrees
     */
    SGBucket(const double dlon, const double dlat);

    /** Construct a bucket.
     *  @param is_good if false, create an invalid bucket.  This is
     *  useful * if you are comparing cur_bucket to last_bucket and
     *  you want to * make sure last_bucket starts out as something
     *  impossible.
     */
    SGBucket(const bool is_good);

    /** Construct a bucket given a unique bucket index number.
     * @param bindex unique bucket index
     */
    SGBucket(const long int bindex);

    /**
     * Default destructor.
     */
    ~SGBucket();

    /**
     * Reset a bucket to represent a new lat and lon
     * @param dlon longitude specified in degrees
     * @param dlat latitude specified in degrees
     */
    void set_bucket( double dlon, double dlat );

    /**
     * Reset a bucket to represent a new lat and lon
     * @param lonlat an array of double[2] holding lon and lat
     * (specified) in degrees
     */
    void set_bucket( double *lonlat );

    /**
     * Create an impossible bucket.
     * This is useful if you are comparing cur_bucket to last_bucket
     * and you want to make sure last_bucket starts out as something
     * impossible.
     */
    inline void make_bad() {
	set_bucket(0.0, 0.0);
	lon = -1000;
    }

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
    inline string gen_index_str() const {
	char tmp[20];
	sprintf(tmp, "%ld", 
                (((long)lon + 180) << 14) + ((lat + 90) << 6) + (y << 3) + x);
	return (string)tmp;
    }

    /**
     * Build the base path name for this bucket.
     * @return base path in string form
     */
    string gen_base_path() const;

    /**
     * @return the center lon of a tile.
     */
    inline double get_center_lon() const {
	double span = sg_bucket_span( lat + y / 8.0 + SG_HALF_BUCKET_SPAN );

	if ( span >= 1.0 ) {
	    return lon + span / 2.0;
	} else {
	    return lon + x * span + span / 2.0;
	}
    }

    /**
     * @return the center lat of a tile.
     */
    inline double get_center_lat() const {
	return lat + y / 8.0 + SG_HALF_BUCKET_SPAN;
    }

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
 
    // Informational methods.

    /**
     * @return the lon of the lower left corner of this tile.
     */
    inline int get_lon() const { return lon; }

    /**
     * @return the lat of the lower left corner of this tile.
     */
    inline int get_lat() const { return lat; }

    /**
     * @return the x coord within the 1x1 degree chunk this tile.
     */
    inline int get_x() const { return x; }

    /**
     * @return the y coord within the 1x1 degree chunk this tile.
     */
    inline int get_y() const { return y; }

    // friends

    friend ostream& operator<< ( ostream&, const SGBucket& );
    friend bool operator== ( const SGBucket&, const SGBucket& );
};


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
 * Write the bucket lon, lat, x, and y to the output stream.
 * @param out output stream
 * @param b bucket
 */
inline ostream&
operator<< ( ostream& out, const SGBucket& b )
{
    return out << b.lon << ":" << b.x << ", " << b.lat << ":" << b.y;
}


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


