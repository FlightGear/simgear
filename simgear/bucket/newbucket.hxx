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


#ifndef _NEWBUCKET_HXX
#define _NEWBUCKET_HXX

#include <simgear/compiler.h>
#include <simgear/constants.h>

#ifdef FG_HAVE_STD_INCLUDES
#  include <cmath>
#  include <cstdio> // sprintf()
#  include <iostream>
#else
#  include <math.h>
#  include <stdio.h> // sprintf()
#  include <iostream.h>
#endif

// I don't understand ... <math.h> or <cmath> should be included
// already depending on how you defined FG_HAVE_STD_INCLUDES, but I
// can go ahead and add this -- CLO
#ifdef __MWERKS__
FG_USING_STD(sprintf);
FG_USING_STD(fabs);
#endif

#include STL_STRING

FG_USING_STD(string);

#if ! defined( FG_HAVE_NATIVE_SGI_COMPILERS )
FG_USING_STD(ostream);
#endif



#define SG_BUCKET_SPAN      0.125   // 1/8 of a degree
#define SG_HALF_BUCKET_SPAN 0.0625  // 1/2 of 1/8 of a degree = 1/16 = 0.0625

class SGBucket;
ostream& operator<< ( ostream&, const SGBucket& );
bool operator== ( const SGBucket&, const SGBucket& );


// return the horizontal tile span factor based on latitude
inline double sg_bucket_span( double l ) {
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


class SGBucket {

private:
    double cx, cy;  // centerpoint (lon, lat) in degrees of bucket
    int lon;        // longitude index (-180 to 179)
    int lat;        // latitude index (-90 to 89)
    int x;          // x subdivision (0 to 7)
    int y;          // y subdivision (0 to 7)

public:

    // default constructor
    SGBucket();

    // constructor for specified location
    SGBucket(const double dlon, const double dlat);

    // create an impossible bucket if false
    SGBucket(const bool is_good);

    // Parse a unique scenery tile index and find the lon, lat, x, and y
    SGBucket(const long int bindex);

    // default destructor
    ~SGBucket();

    // Set the bucket params for the specified lat and lon
    void set_bucket( double dlon, double dlat );
    void set_bucket( double *lonlat );

    // create an impossible bucket
    inline void make_bad( void ) {
	set_bucket(0.0, 0.0);
	lon = -1000;
    }

    // Generate the unique scenery tile index for this bucket
    // 
    // The index is constructed as follows:
    // 
    // 9 bits - to represent 360 degrees of longitude (-180 to 179)
    // 8 bits - to represent 180 degrees of latitude (-90 to 89)
    //
    // Each 1 degree by 1 degree tile is further broken down into an 8x8
    // grid.  So we also need:
    //
    // 3 bits - to represent x (0 to 7)
    // 3 bits - to represent y (0 to 7)

    inline long int gen_index() const {
	return ((lon + 180) << 14) + ((lat + 90) << 6) + (y << 3) + x;
    }

    inline string gen_index_str() const {
	char tmp[20];
	sprintf(tmp, "%ld", 
		(((long)lon + 180) << 14) + ((lat + 90) << 6) + (y << 3) + x);
	return (string)tmp;
    }

    // Build the path name for this bucket
    string gen_base_path() const;

    // return the center lon of a tile
    inline double get_center_lon() const {
	double span = sg_bucket_span( lat + y / 8.0 + SG_HALF_BUCKET_SPAN );

	if ( span >= 1.0 ) {
	    return lon + span / 2.0;
	} else {
	    return lon + x * span + span / 2.0;
	}
    }

    // return the center lat of a tile
    inline double get_center_lat() const {
	return lat + y / 8.0 + SG_HALF_BUCKET_SPAN;
    }

    // return width of the tile in degrees
    double get_width() const;
    // return height of the tile in degrees
    double get_height() const;

    // return width of the tile in meters
    double get_width_m() const; 
    // return height of the tile in meters
    double get_height_m() const;
 
    // Informational methods
    inline int get_lon() const { return lon; }
    inline int get_lat() const { return lat; }
    inline int get_x() const { return x; }
    inline int get_y() const { return y; }

    // friends
    friend ostream& operator<< ( ostream&, const SGBucket& );
    friend bool operator== ( const SGBucket&, const SGBucket& );
};


// offset a bucket specified by dlon, dlat by the specified tile units
// in the X & Y direction
SGBucket sgBucketOffset( double dlon, double dlat, int x, int y );


// calculate the offset between two buckets
void sgBucketDiff( const SGBucket& b1, const SGBucket& b2, int *dx, int *dy );


inline ostream&
operator<< ( ostream& out, const SGBucket& b )
{
    return out << b.lon << ":" << b.x << ", " << b.lat << ":" << b.y;
}


inline bool
operator== ( const SGBucket& b1, const SGBucket& b2 )
{
    return ( b1.lon == b2.lon &&
	     b1.lat == b2.lat &&
	     b1.x == b2.x &&
	     b1.y == b2.y );
}


#endif // _NEWBUCKET_HXX


