// texcoord.hxx -- routine(s) to handle texture coordinate generation
//
// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#include "texcoord.hxx"


#define FG_STANDARD_TEXTURE_DIMENSION 1000.0 // meters
#define MAX_TEX_COORD 8.0
#define HALF_MAX_TEX_COORD ( MAX_TEX_COORD / 2.0 )


// return the basic unshifted/unmoded texture coordinate for a lat/lon
inline Point3D basic_tex_coord( const Point3D& p, 
				double degree_width, double degree_height,
				double scale )
{
    return Point3D( p.x() * ( degree_width * scale / 
			      FG_STANDARD_TEXTURE_DIMENSION ),
		    p.y() * ( degree_width * scale /
			      FG_STANDARD_TEXTURE_DIMENSION ),
		    0.0 );
}


// traverse the specified fan/strip/list of vertices and attempt to
// calculate "none stretching" texture coordinates
point_list calc_tex_coords( const FGBucket& b, const point_list& geod_nodes,
			    const int_list& fan, double scale )
{
    // cout << "calculating texture coordinates for a specific fan of size = "
    //      << fan.size() << endl;

    // calculate perimeter based on center of this degree (not center
    // of bucket)
    double clat = (int)b.get_center_lat();
    if ( clat > 0 ) {
	clat = (int)clat + 0.5;
    } else {
	clat = (int)clat - 0.5;
    }

    double clat_rad = clat * DEG_TO_RAD;
    double cos_lat = cos( clat_rad );
    double local_radius = cos_lat * EQUATORIAL_RADIUS_M;
    double local_perimeter = 2.0 * local_radius * FG_PI;
    double degree_width = local_perimeter / 360.0;

    // cout << "clat = " << clat << endl;
    // cout << "clat (radians) = " << clat_rad << endl;
    // cout << "cos(lat) = " << cos_lat << endl;
    // cout << "local_radius = " << local_radius << endl;
    // cout << "local_perimeter = " << local_perimeter << endl;
    // cout << "degree_width = " << degree_width << endl;

    double perimeter = 2.0 * EQUATORIAL_RADIUS_M * FG_PI;
    double degree_height = perimeter / 360.0;
    // cout << "degree_height = " << degree_height << endl;

    // find min/max of fan
    Point3D tmin, tmax, p, t;
    bool first = true;

    int i;

    for ( i = 0; i < (int)fan.size(); ++i ) {
	p = geod_nodes[ fan[i] ];
	// cout << "point p = " << p << endl;

	t = basic_tex_coord( p, degree_width, degree_height, scale );
	// cout << "basic_tex_coord = " << t << endl;

	if ( first ) {
	    tmin = tmax = t;
	    first = false;
	} else {
	    if ( t.x() < tmin.x() ) {
		tmin.setx( t.x() );
	    }
	    if ( t.y() < tmin.y() ) {
		tmin.sety( t.y() );
	    }
	    if ( t.x() > tmax.x() ) {
		tmax.setx( t.x() );
	    }
	    if ( t.y() > tmax.y() ) {
		tmax.sety( t.y() );
	    }
	}
    }

    double dx = fabs( tmax.x() - tmin.x() );
    double dy = fabs( tmax.y() - tmin.y() );
    // cout << "dx = " << dx << " dy = " << dy << endl;

    bool do_shift = false;
    Point3D mod_shift;
    if ( (dx > HALF_MAX_TEX_COORD) || (dy > HALF_MAX_TEX_COORD) ) {
	// structure is too big, we'll just have to shift it so that
	// tmin = (0,0).  This messes up subsequent texture scaling,
	// but is the best we can do.
	// cout << "SHIFTING" << endl;
	do_shift = true;
	tmin.setx( (double)( (int)tmin.x() + 1 ) );
	tmin.sety( (double)( (int)tmin.y() + 1 ) );
	// cout << "found tmin = " << tmin << endl;
    } else {
	// structure is small enough ... we can mod it so we can
	// properly scale the texture coordinates later.
	// cout << "MODDING" << endl;
	double x1 = fmod(tmin.x(), MAX_TEX_COORD);
	while ( x1 < 0 ) { x1 += MAX_TEX_COORD; }

	double y1 = fmod(tmin.y(), MAX_TEX_COORD);
	while ( y1 < 0 ) { y1 += MAX_TEX_COORD; }

	double x2 = fmod(tmax.x(), MAX_TEX_COORD);
	while ( x2 < 0 ) { x2 += MAX_TEX_COORD; }

	double y2 = fmod(tmax.y(), MAX_TEX_COORD);
	while ( y2 < 0 ) { y2 += MAX_TEX_COORD; }
	
	// At this point we know that the object is < 16 wide in
	// texture coordinate space.  If the modulo of the tmin is >
	// the mod of the tmax at this point, then we know that the
	// starting tex coordinate for the tmin > 16 so we can shift
	// everything down by 16 and get it within the 0-32 range.

	if ( x1 > x2 ) {
	    mod_shift.setx( HALF_MAX_TEX_COORD );
	} else {
	    mod_shift.setx( 0.0 );
	}

	if ( y1 > y2 ) {
	    mod_shift.sety( HALF_MAX_TEX_COORD );
	} else {
	    mod_shift.sety( 0.0 );
	}

	// cout << "mod_shift = " << mod_shift << endl;
    }

    // generate tex_list
    Point3D adjusted_t;
    point_list tex;
    tex.clear();
    for ( i = 0; i < (int)fan.size(); ++i ) {
	p = geod_nodes[ fan[i] ];
	t = basic_tex_coord( p, degree_width, degree_height, scale );
	// cout << "second t = " << t << endl;

	if ( do_shift ) {
	    adjusted_t = t - tmin;
	} else {
	    adjusted_t.setx( fmod(t.x() + mod_shift.x(), MAX_TEX_COORD) );
	    while ( adjusted_t.x() < 0 ) { 
		adjusted_t.setx( adjusted_t.x() + MAX_TEX_COORD );
	    }
	    adjusted_t.sety( fmod(t.y() + mod_shift.y(), MAX_TEX_COORD) );
	    while ( adjusted_t.y() < 0 ) {
		adjusted_t.sety( adjusted_t.y() + MAX_TEX_COORD );
	    }
	    // cout << "adjusted_t " << adjusted_t << endl;
	}

	if ( adjusted_t.x() < FG_EPSILON ) {
	    adjusted_t.setx( 0.0 );
	}
	if ( adjusted_t.y() < FG_EPSILON ) {
	    adjusted_t.sety( 0.0 );
	}
	adjusted_t.setz( 0.0 );
	// cout << "adjusted_t = " << adjusted_t << endl;
	
	tex.push_back( adjusted_t );
    }

    return tex;
}
