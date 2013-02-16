// texcoord.hxx -- routine(s) to handle texture coordinate generation
//
// Written by Curtis Olson, started March 1999.
//
// Copyright (C) 1999  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


/* The following is an explanation of our somewhat conveluted and
   tricky texture scaling/offset scheme:

MAX_TEX_COORD is a value I arrived at by trial and error for my
voodoo2/3 video card.  If you use texture coordinates that are too
big, you quickly start getting into round off problems and the texture
jumps and moves relative to the polygon.

The point of all of this code is that I wanted to be able to define
this size in meters of a texture and have it be applied seamlessly to
the terrain.  I wanted to be able to change the defined size (in
meters) of textures at run time.  In other words I want to be able to
scale the textures at run time and still have them seamlessly tile
together across fans.

The problem is that I have to pregenerate all the texture coordinates
when I create the scenery, and I didn't want to burn CPU doing this
again when I load the scenery at run time.

It ended up taking me a lot of thought, a lot of trial and error, and
a lot of fiddling around to come up with a scheme that worked.

----------

Ok, so think about what needs to be done to have the texture tile
across a series of triangles and fans ...

Basically you want to use some function of lon/lat mod your max
texture coordinate size to calculate the texture coordinate of each
vertex.  This should result in nice tiling across distinct triangles
and fans.

Pretend our MAX_TEX_COORD = 4.0 and half of this is 2.0

Imagine the following two adjacent polygons with the "X" component of
the initial texture coordinate based on longitude (Note they are drawn
spaced apart, but in reality the two polygons are adjacent):

    7.0   8.6   8.6    9.0
     *-----*     *------*
     |     |     |      |

Now, this exceeds our MAX_TEX_COORD of 4.0 so we have to scale these
texture coordinates by some integer value.  Let's say we always want
to minimize the tex coordinates to minimize rounding error so we will
offset the first polygon by 7.0 and the second by 8.0:

    0.0 --- 1.6 and 0.6 --- 1.0

Our tiling is maintianed becuase the coordinates are continous (mod
1.0) and we still get the double repeat across both polygons.

We want to be able to scale these values by an arbitrary constant and
still have proper tiling.

Let's try doubling the coordinates:

    0.0 --- 3.2 and 1.2 --- 2.0

Everything still tiles nicely (because the coordinates are continuous
mod 1.0) and the texture is now repeated 4x across the two polygons.
Before it was repeated 2x.

Let's try halving the coordinates:

    0.0 --- 0.8 and 0.3 --- 0.5

Ooop!  We lost continuity in texture coordinate space ... no we will
have a visual discontinuity in the texture tiling!

Ok, so we need some other scheme to keep our texture coordinates
smaller than MAX_TEX_COORD that preserves continuity in texture
space.  <Deep breath> let's try the scheme that I have coded up that
you are asking about ... <fingers crossed> :-)

Going way back to the top before we shifted the texture coordinates.
tmin for the first polygon is 7.0, this is then adjusted to:

    (int)(tmin.x() / HALF_MAX_TEX_COORD) ) * HALF_MAX_TEX_COORD

    = (int)(7.0/2.0) * 2.0 = 3.0 * 2.0 = 6.0

The two texture coordinates are offset by 6.0 which yields 1.0 -- 2.6

tmin for the second polygon is 8.6 which is adjusted to:

    (int)(tmin.x() / HALF_MAX_TEX_COORD) ) * HALF_MAX_TEX_COORD
    = (int)( 8.6 / 2.0 ) * 2.0 = 4.0 * 2.0 = 8.0

The offset for the second polygon is 8.0 which yields 0.6 --- 1.0

So now we have:

    1.0 --- 2.6 and 0.6 --- 1.0

This still tiles nicely and strethes our texture across completely, so
far we haven't done any damage.

Now let's double the coordinates:

     2.0 --- 5.2 and 1.2 --- 2.0

The texture is repeated 4x as it should be and is still continuous.

How about halfing the coordinates.  This is where the first scheme
broke down.  Halving the coordinates yields

    0.5 --- 1.3 and 0.3 --- 0.5

Woohoo, we still have texture space continuity (mod 1.0) and the
texture is repeated 1x.

Note, it took me almost as long to re-figure this out and write this
explanation as it did to figure out the scheme originally.  I better
enter this in the official comments in case I forget again. :-)

*/

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

// #include <iostream>

#include "texcoord.hxx"

// using std::cout;
// using std::endl;


#define FG_STANDARD_TEXTURE_DIMENSION 1000.0 // meters
#define MAX_TEX_COORD 8.0
#define HALF_MAX_TEX_COORD ( MAX_TEX_COORD * 0.5 )


// return the basic unshifted/unmoded texture coordinate for a lat/lon
static inline SGVec2f basic_tex_coord( const SGGeod& p, 
                                       double degree_width,
                                       double degree_height,
                                       double scale )
{
    return SGVec2f( p.getLongitudeDeg() * ( degree_width * scale / 
			      FG_STANDARD_TEXTURE_DIMENSION ),
		    p.getLatitudeDeg() * ( degree_height * scale /
			      FG_STANDARD_TEXTURE_DIMENSION )
		    );
}


// traverse the specified fan/strip/list of vertices and attempt to
// calculate "none stretching" texture coordinates
std::vector<SGVec2f> sgCalcTexCoords( const SGBucket& b, const std::vector<SGGeod>& geod_nodes,
			    const int_list& fan, double scale )
{
    return sgCalcTexCoords(b.get_center_lat(), geod_nodes, fan, scale);
}

std::vector<SGVec2f> sgCalcTexCoords( double centerLat, const std::vector<SGGeod>& geod_nodes,
			    const int_list& fan, double scale )
{
    // cout << "calculating texture coordinates for a specific fan of size = "
    //      << fan.size() << endl;

    // calculate perimeter based on center of this degree (not center
    // of bucket)
    double clat = (int)centerLat;
    if ( clat > 0 ) {
	clat = (int)clat + 0.5;
    } else {
	clat = (int)clat - 0.5;
    }

    double clat_rad = clat * SGD_DEGREES_TO_RADIANS;
    double cos_lat = cos( clat_rad );
    double local_radius = cos_lat * SG_EQUATORIAL_RADIUS_M;
    double local_perimeter = local_radius * SGD_2PI;
    double degree_width = local_perimeter / 360.0;

    // cout << "clat = " << clat << endl;
    // cout << "clat (radians) = " << clat_rad << endl;
    // cout << "cos(lat) = " << cos_lat << endl;
    // cout << "local_radius = " << local_radius << endl;
    // cout << "local_perimeter = " << local_perimeter << endl;
    // cout << "degree_width = " << degree_width << endl;

    double perimeter = SG_EQUATORIAL_RADIUS_M * SGD_2PI;
    double degree_height = perimeter / 360.0;
    // cout << "degree_height = " << degree_height << endl;

    // find min/max of fan
    SGVec2f tmin(0.0, 0.0);
    SGVec2f tmax(0.0, 0.0);
    bool first = true;

    int i;

    for ( i = 0; i < (int)fan.size(); ++i ) {
      SGGeod p = geod_nodes[ fan[i] ];
	// cout << "point p = " << p << endl;

	SGVec2f t = basic_tex_coord( p, degree_width, degree_height, scale );
	// cout << "basic_tex_coord = " << t << endl;

	if ( first ) {
	    tmin = tmax = t;
	    first = false;
	} else {
	    if ( t.x() < tmin.x() ) {
		tmin.x() = t.x();
	    }
	    if ( t.y() < tmin.y() ) {
		tmin.y() = t.y();
	    }
	    if ( t.x() > tmax.x() ) {
		tmax.x() = t.x();
	    }
	    if ( t.y() > tmax.y() ) {
		tmax.y() = t.y();
	    }
	}
    }

    double dx = fabs( tmax.x() - tmin.x() );
    double dy = fabs( tmax.y() - tmin.y() );
    // cout << "dx = " << dx << " dy = " << dy << endl;

    // Point3D mod_shift;
    if ( (dx > HALF_MAX_TEX_COORD) || (dy > HALF_MAX_TEX_COORD) ) {
	// structure is too big, we'll just have to shift it so that
	// tmin = (0,0).  This messes up subsequent texture scaling,
	// but is the best we can do.
	// cout << "SHIFTING" << endl;
	if ( tmin.x() < 0 ) {
	    tmin.x() = (double)( (int)tmin.x() - 1 ) ;
	} else {
	    tmin.x() = (int)tmin.x();
	}
  
	if ( tmin.y() < 0 ) {
	    tmin.y() = (double)( (int)tmin.y() - 1 );
	} else {
	    tmin.y() = (int)tmin.y();
	}
	// cout << "found tmin = " << tmin << endl;
    } else {
	if ( tmin.x() < 0 ) {
	    tmin.x() =  ( (int)(tmin.x() / HALF_MAX_TEX_COORD) - 1 )
		       * HALF_MAX_TEX_COORD ;
	} else {
	    tmin.x() =  ( (int)(tmin.x() / HALF_MAX_TEX_COORD) )
		       * HALF_MAX_TEX_COORD ;
	}
	if ( tmin.y() < 0 ) {
	    tmin.y() =  ( (int)(tmin.y() / HALF_MAX_TEX_COORD) - 1 )
		       * HALF_MAX_TEX_COORD ;
	} else {
	    tmin.y() =  ( (int)(tmin.y() / HALF_MAX_TEX_COORD) )
		       * HALF_MAX_TEX_COORD ;
	}
#if 0
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
	// starting tex coordinate for the tmax > 16 so we can shift
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
#endif
	// cout << "mod_shift = " << mod_shift << endl;
    }

    // generate tex_list
    SGVec2f adjusted_t;
    std::vector<SGVec2f> tex;
    tex.clear();
    for ( i = 0; i < (int)fan.size(); ++i ) {
	SGGeod p = geod_nodes[ fan[i] ];
	SGVec2f t = basic_tex_coord( p, degree_width, degree_height, scale );
	// cout << "second t = " << t << endl;

	adjusted_t = t - tmin;
#if 0
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
#endif
	if ( adjusted_t.x() < SG_EPSILON ) {
	    adjusted_t.x() = 0.0;
	}
	if ( adjusted_t.y() < SG_EPSILON ) {
	    adjusted_t.y() = 0.0;
	}

	// cout << "adjusted_t = " << adjusted_t << endl;
	
	tex.push_back( adjusted_t );
    }

    return tex;
}
