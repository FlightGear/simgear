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


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif


#include <math.h>

#include <simgear/misc/fgpath.hxx>

#include "newbucket.hxx"


// default constructor
SGBucket::SGBucket() {
}


// constructor for specified location
SGBucket::SGBucket(const double dlon, const double dlat) {
    set_bucket(dlon, dlat);
}


// create an impossible bucket if false
SGBucket::SGBucket(const bool is_good) {
    set_bucket(0.0, 0.0);
    if ( !is_good ) {
	lon = -1000;
    }
}


// Parse a unique scenery tile index and find the lon, lat, x, and y
SGBucket::SGBucket(const long int bindex) {
    long int index = bindex;
	
    lon = index >> 14;
    index -= lon << 14;
    lon -= 180;

    lat = index >> 6;
    index -= lat << 6;
    lat -= 90;

    y = index >> 3;
    index -= y << 3;

    x = index;
}


// default destructor
SGBucket::~SGBucket() {
}


// Set the bucket params for the specified lat and lon
void SGBucket::set_bucket( double *lonlat ) {
    set_bucket( lonlat[0], lonlat[1] );
}	


// Set the bucket params for the specified lat and lon
void SGBucket::set_bucket( double dlon, double dlat ) {
    //
    // latitude first
    //
    double span = sg_bucket_span( dlat );
    double diff = dlon - (double)(int)dlon;

    // cout << "diff = " << diff << "  span = " << span << endl;

    if ( (dlon >= 0) || (fabs(diff) < SG_EPSILON) ) {
	lon = (int)dlon;
    } else {
	lon = (int)dlon - 1;
    }

    // find subdivision or super lon if needed
    if ( span < SG_EPSILON ) {
	// polar cap
	lon = 0;
	x = 0;
    } else if ( span <= 1.0 ) {
	x = (int)((dlon - lon) / span);
    } else {
	if ( (dlon >= 0) || (fabs(diff) < SG_EPSILON) ) {
	    lon = (int)( (int)(lon / span) * span);
	} else {
	    // cout << " lon = " << lon 
	    //  << "  tmp = " << (int)((lon-1) / span) << endl;
	    lon = (int)( (int)((lon + 1) / span) * span - span);
	    if ( lon < -180 ) {
		lon = -180;
	    }
	}
	x = 0;
    }

    //
    // then latitude
    //
    diff = dlat - (double)(int)dlat;

    if ( (dlat >= 0) || (fabs(diff) < SG_EPSILON) ) {
	lat = (int)dlat;
    } else {
	lat = (int)dlat - 1;
    }
    y = (int)((dlat - lat) * 8);
}


// Build the path name for this bucket
string SGBucket::gen_base_path() const {
    // long int index;
    int top_lon, top_lat, main_lon, main_lat;
    char hem, pole;
    char raw_path[256];

    top_lon = lon / 10;
    main_lon = lon;
    if ( (lon < 0) && (top_lon * 10 != lon) ) {
	top_lon -= 1;
    }
    top_lon *= 10;
    if ( top_lon >= 0 ) {
	hem = 'e';
    } else {
	hem = 'w';
	top_lon *= -1;
    }
    if ( main_lon < 0 ) {
	main_lon *= -1;
    }
    
    top_lat = lat / 10;
    main_lat = lat;
    if ( (lat < 0) && (top_lat * 10 != lat) ) {
	top_lat -= 1;
    }
    top_lat *= 10;
    if ( top_lat >= 0 ) {
	pole = 'n';
    } else {
	pole = 's';
	top_lat *= -1;
    }
    if ( main_lat < 0 ) {
	main_lat *= -1;
    }

    sprintf(raw_path, "%c%03d%c%02d/%c%03d%c%02d", 
	    hem, top_lon, pole, top_lat, 
	    hem, main_lon, pole, main_lat);

    FGPath path( raw_path );

    return path.str();
}


// return width of the tile in degrees
double SGBucket::get_width() const {
    return sg_bucket_span( get_center_lat() );
}


// return height of the tile in degrees
double SGBucket::get_height() const {
    return SG_BUCKET_SPAN;
}


// return width of the tile in meters
double SGBucket::get_width_m() const {
    double clat = (int)get_center_lat();
    if ( clat > 0 ) {
	clat = (int)clat + 0.5;
    } else {
	clat = (int)clat - 0.5;
    }
    double clat_rad = clat * SGD_DEGREES_TO_RADIANS;
    double cos_lat = cos( clat_rad );
    double local_radius = cos_lat * SG_EQUATORIAL_RADIUS_M;
    double local_perimeter = 2.0 * local_radius * SGD_PI;
    double degree_width = local_perimeter / 360.0;

    return sg_bucket_span( get_center_lat() ) * degree_width;
}


// return height of the tile in meters
double SGBucket::get_height_m() const {
    double perimeter = 2.0 * SG_EQUATORIAL_RADIUS_M * SGD_PI;
    double degree_height = perimeter / 360.0;

    return SG_BUCKET_SPAN * degree_height;
}


// find the bucket which is offset by the specified tile units in the
// X & Y direction.  We need the current lon and lat to resolve
// ambiguities when going from a wider tile to a narrower one above or
// below.  This assumes that we are feeding in
SGBucket sgBucketOffset( double dlon, double dlat, int dx, int dy ) {
    SGBucket result( dlon, dlat );
    double clat = result.get_center_lat() + dy * SG_BUCKET_SPAN;

    // walk dy units in the lat direction
    result.set_bucket( dlon, clat );

    // find the lon span for the new latitude
    double span = sg_bucket_span( clat );

    // walk dx units in the lon direction
    double tmp = dlon + dx * span;
    while ( tmp < -180.0 ) {
	tmp += 360.0;
    }
    while ( tmp >= 180.0 ) {
	tmp -= 360.0;
    }
    result.set_bucket( tmp, clat );

    return result;
}


// calculate the offset between two buckets
void sgBucketDiff( const SGBucket& b1, const SGBucket& b2, int *dx, int *dy ) {

    // Latitude difference
    double c1_lat = b1.get_center_lat();
    double c2_lat = b2.get_center_lat();
    double diff_lat = c2_lat - c1_lat;

#ifdef HAVE_RINT
    *dy = (int)rint( diff_lat / SG_BUCKET_SPAN );
#else
    if ( diff_lat > 0 ) {
	*dy = (int)( diff_lat / SG_BUCKET_SPAN + 0.5 );
    } else {
	*dy = (int)( diff_lat / SG_BUCKET_SPAN - 0.5 );
    }
#endif

    // longitude difference
    double c1_lon = b1.get_center_lon();
    double c2_lon = b2.get_center_lon();
    double diff_lon = c2_lon - c1_lon;
    double span;
    if ( sg_bucket_span(c1_lat) <= sg_bucket_span(c2_lat) ) {
	span = sg_bucket_span(c1_lat);
    } else {
	span = sg_bucket_span(c2_lat);
    }

#ifdef HAVE_RINT
    *dx = (int)rint( diff_lon / span );
#else
    if ( diff_lon > 0 ) {
	*dx = (int)( diff_lon / span + 0.5 );
    } else {
	*dx = (int)( diff_lon / span - 0.5 );
    }
#endif
}


