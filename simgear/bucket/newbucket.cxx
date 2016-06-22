/**************************************************************************
 * newbucket.hxx -- new bucket routines for better world modeling
 *
 * Written by Curtis L. Olson, started February 1999.
 *
 * Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt/
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


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <cmath>
#include <cstdio> // some platforms need this for ::snprintf
#include <iostream>

#include <simgear/misc/sg_path.hxx>
#include <simgear/debug/logstream.hxx>

#include "newbucket.hxx"


// default constructor
SGBucket::SGBucket() :
    lon(-1000),
    lat(-1000),
    x(0),
    y(0)
{
}

bool SGBucket::isValid() const
{
    // The most northerly valid latitude is 89, not 90. There is no tile
    // whose *bottom* latitude is 90. Similar there is no tile whose left egde
    // is 180 longitude.
    return (lon >= -180) &&
            (lon < 180) &&
            (lat >= -90) &&
            (lat < 90) &&
            (x < 8) && (y < 8);
}

void SGBucket::make_bad()
{
    lon = -1000;
    lat = -1000;
}

#ifndef NO_DEPRECATED_API

// constructor for specified location
SGBucket::SGBucket(const double dlon, const double dlat) {
    set_bucket(dlon, dlat);
}
#endif

SGBucket::SGBucket(const SGGeod& geod) {
    innerSet(geod.getLongitudeDeg(),
             geod.getLatitudeDeg());
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

/* Calculate the greatest integral value less than
 * or equal to the given value (floor(x)),
 * but attribute coordinates close to the boundary to the next
 * (increasing) integral
 */
static int floorWithEpsilon(double x)
{
    return static_cast<int>(floor(x + SG_EPSILON));
}

#ifndef NO_DEPRECATED_API

void SGBucket::set_bucket(double dlon, double dlat)
{
    innerSet(dlon, dlat);
}


void SGBucket::set_bucket(const SGGeod& geod)
{
    innerSet(geod.getLongitudeDeg(), geod.getLatitudeDeg());
}

#endif

// Set the bucket params for the specified lat and lon
void SGBucket::innerSet( double dlon, double dlat )
{
    if ((dlon < -180.0) || (dlon >= 180.0)) {
        SG_LOG(SG_TERRAIN, SG_WARN, "SGBucket::set_bucket: passed longitude:" << dlon);
        dlon = SGMiscd::normalizePeriodic(-180.0, 180.0, dlon);
    }
    
    if ((dlat < -90.0) || (dlat > 90.0)) {
        SG_LOG(SG_TERRAIN, SG_WARN, "SGBucket::set_bucket: passed latitude" << dlat);
        dlat = SGMiscd::clip(dlat, -90.0, 90.0);
    }
    
    //
    // longitude first
    //
    double span = sg_bucket_span( dlat );
    // we do NOT need to special case lon=180 here, since
    // normalizePeriodic will never return 180; it will
    // return -180, which is what we want.
    lon = floorWithEpsilon(dlon);
    
    // find subdivision or super lon if needed
    if ( span <= 1.0 ) {
        /* We have more than one tile per degree of
         * longitude, so we need an x offset.
         */
        x = floorWithEpsilon((dlon - lon) / span);
    } else {
        /* We have one or more degrees per tile,
         * so we need to find the base longitude
         * of that tile.
         *
         * First we calculate the integral base longitude
         * (e.g. -85.5 => -86) and then find the greatest
         * multiple of span that is less than or equal to
         * that longitude.
         *
         * That way, the Greenwich Meridian is always
         * a tile border.
         */
        lon=static_cast<int>(floor(lon / span) * span);
        x = 0;
    }

    //
    // then latitude
    //
    lat = floorWithEpsilon(dlat);
    
    // special case when passing in the north pole point (possibly due to
    // clipping latitude above). Ensures we generate a valid bucket in this
    // scenario
    if (lat == 90) {
        lat = 89;
        y = 7;
    } else {
        /* Latitude base and offset are easier, as
         * tiles always are 1/8 degree of latitude wide.
         */
        y = floorWithEpsilon((dlat - lat) * 8);
    }
}

// Build the path name for this bucket
std::string SGBucket::gen_base_path() const {
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

    ::snprintf(raw_path, 256, "%c%03d%c%02d/%c%03d%c%02d",
	    hem, top_lon, pole, top_lat, 
	    hem, main_lon, pole, main_lat);

    return raw_path;
}


// return width of the tile in degrees
double SGBucket::get_width() const {
    return sg_bucket_span( get_center_lat() );
}


// return height of the tile in degrees
double SGBucket::get_height() const {
    return SG_BUCKET_SPAN;
}

double SGBucket::get_highest_lat() const
{
    unsigned char adjustedY = y;
    if (lat >= 0) {
        // tile is north of the equator, so we want the top edge. Add one
        // to y to achieve this.
        ++adjustedY;
    }
    
	return lat + (adjustedY / 8.0);
}


// return width of the tile in meters. This function is used by the
// tile-manager to estimate how many tiles are in the view distance, so
// we care about the smallest width, which occurs at the highest latitude.
double SGBucket::get_width_m() const
{
    double clat_rad = get_highest_lat() * SGD_DEGREES_TO_RADIANS;
    double cos_lat = cos( clat_rad );
    if (fabs(cos_lat) < SG_EPSILON) {
        // happens for polar tiles, since we pass in a latitude of 90
        // return an arbitrary small value so all tiles are loaded
        return 10.0;
    }
    
    double local_radius = cos_lat * SG_EQUATORIAL_RADIUS_M;
    double local_perimeter = local_radius * SGD_2PI;
    double degree_width = local_perimeter / 360.0;

    return get_width() * degree_width;
}


// return height of the tile in meters
double SGBucket::get_height_m() const {
    double perimeter = SG_EQUATORIAL_RADIUS_M * SGD_2PI;
    double degree_height = perimeter / 360.0;

    return SG_BUCKET_SPAN * degree_height;
}

unsigned int SGBucket::siblings( int dx, int dy, std::vector<SGBucket>& buckets ) const 
{
    if (!isValid()) {
        SG_LOG(SG_TERRAIN, SG_WARN, "SGBucket::sibling: requesting sibling of invalid bucket");
        return 0;
    }
    
    double src_span = sg_bucket_span( get_center_lat() );
   
    double clat = get_center_lat() + dy * SG_BUCKET_SPAN;
    // return invalid here instead of clipping, so callers can discard
    // invalid buckets without having to check if it's an existing one
    if ((clat < -90.0) || (clat > 90.0)) {
        return 0;
    }
    
    // find the lon span for the new latitude
    double trg_span = sg_bucket_span( clat );
    
    // if target span < src_span, return multiple buckets...
    if ( trg_span < src_span ) {
        // calc center longitude of westernmost sibling
        double start_lon = get_center_lat() - src_span/2 + trg_span/2;
        
        unsigned int num_buckets = src_span/trg_span;
        for ( unsigned int x = 0; x < num_buckets; x++ ) {
            double tmp = start_lon + x * trg_span;
            tmp = SGMiscd::normalizePeriodic(-180.0, 180.0, tmp);
            
            SGBucket b;
            b.innerSet(tmp, clat);
            
            buckets.push_back( b );
        }
    } else {
        // just return the single sibling
        double tmp = get_center_lon() + dx * trg_span;
        tmp = SGMiscd::normalizePeriodic(-180.0, 180.0, tmp);
    
        SGBucket b;
        b.innerSet(tmp, clat);
        
        buckets.push_back( b );
    }
    
    return buckets.size();
}

SGBucket SGBucket::sibling(int dx, int dy) const
{
    if (!isValid()) {
        SG_LOG(SG_TERRAIN, SG_WARN, "SGBucket::sibling: requesting sibling of invalid bucket");
        return SGBucket();
    }
    
    double clat = get_center_lat() + dy * SG_BUCKET_SPAN;
    // return invalid here instead of clipping, so callers can discard
    // invalid buckets without having to check if it's an existing one
    if ((clat < -90.0) || (clat > 90.0)) {
        return SGBucket();
    }
    
    // find the lon span for the new latitude
    double span = sg_bucket_span( clat );
    
    double tmp = get_center_lon() + dx * span;
    tmp = SGMiscd::normalizePeriodic(-180.0, 180.0, tmp);
    
    SGBucket b;
    b.innerSet(tmp, clat);
    return b;
}

std::string SGBucket::gen_index_str() const
{
	char tmp[20];
	::snprintf(tmp, 20, "%ld",
                 (((long)lon + 180) << 14) + ((lat + 90) << 6)
                 + (y << 3) + x);
	return (std::string)tmp;
}

#ifndef NO_DEPRECATED_API
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
#endif

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
    double diff_lon=0.0;
    double span=0.0;

    SGBucket tmp_bucket;
    // To handle crossing the bucket size boundary
    //  we need to account for different size buckets.

    if ( sg_bucket_span(c1_lat) <= sg_bucket_span(c2_lat) )
    {
	span = sg_bucket_span(c1_lat);
    } else {
	span = sg_bucket_span(c2_lat);
    }

    diff_lon = b2.get_center_lon() - b1.get_center_lon();

    if (diff_lon <0.0)
    {
       diff_lon -= b1.get_width()*0.5 + b2.get_width()*0.5 - span;
    } 
    else
    {
       diff_lon += b1.get_width()*0.5 + b2.get_width()*0.5 - span;
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

void sgGetBuckets( const SGGeod& min, const SGGeod& max, std::vector<SGBucket>& list ) {
    double lon, lat, span;

    for (lat = min.getLatitudeDeg(); lat < max.getLatitudeDeg()+SG_BUCKET_SPAN; lat += SG_BUCKET_SPAN) {
        span = sg_bucket_span( lat );
        for (lon = min.getLongitudeDeg(); lon <= max.getLongitudeDeg(); lon += span)
        {
            SGBucket b(SGGeod::fromDeg(lon, lat));
            if (!b.isValid()) {
                continue;
            }
            
            list.push_back(b);
        }
    }
}

std::ostream& operator<< ( std::ostream& out, const SGBucket& b )
{
    return out << b.lon << ":" << (int)b.x << ", " << b.lat << ":" << (int)b.y;
}

