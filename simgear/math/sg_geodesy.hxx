// sg_geodesy.hxx -- routines to convert between geodetic and geocentric 
//                   coordinate systems.
//
// Copied and adapted directly from LaRCsim/ls_geodesy.c
//
// See below for the complete original LaRCsim comments.
//
// $Id$


#ifndef _SG_GEODESY_HXX
#define _SG_GEODESY_HXX


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   


#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>


// sgGeocToGeod(lat_geoc, radius, *lat_geod, *alt, *sea_level_r)
//     INPUTS:	
//         lat_geoc	Geocentric latitude, radians, + = North
//         radius	C.G. radius to earth center (meters)
//
//     OUTPUTS:
//         lat_geod	Geodetic latitude, radians, + = North
//         alt		C.G. altitude above mean sea level (meters)
//         sea_level_r	radius from earth center to sea level at
//                      local vertical (surface normal) of C.G. (meters)

void sgGeocToGeod( double lat_geoc, double radius, double
		   *lat_geod, double *alt, double *sea_level_r );


// sgGeodToGeoc( lat_geod, alt, *sl_radius, *lat_geoc )
//     INPUTS:	
//         lat_geod	Geodetic latitude, radians, + = North
//         alt		C.G. altitude above mean sea level (meters)
//
//     OUTPUTS:
//         sl_radius	SEA LEVEL radius to earth center (meters)
//                      (add Altitude to get true distance from earth center.
//         lat_geoc	Geocentric latitude, radians, + = North
//

void sgGeodToGeoc( double lat_geod, double alt, double *sl_radius,
		      double *lat_geoc );


// convert a geodetic point lon(radians), lat(radians), elev(meter) to
// a cartesian point

inline Point3D sgGeodToCart(const Point3D& geod) {
    double gc_lon, gc_lat, sl_radius;

    // printf("A geodetic point is (%.2f, %.2f, %.2f)\n", 
    //        geod[0], geod[1], geod[2]);

    gc_lon = geod.lon();
    sgGeodToGeoc(geod.lat(), geod.radius(), &sl_radius, &gc_lat);

    // printf("A geocentric point is (%.2f, %.2f, %.2f)\n", gc_lon, 
    //        gc_lat, sl_radius+geod[2]);

    Point3D pp = Point3D( gc_lon, gc_lat, sl_radius + geod.radius());
    return sgPolarToCart3d(pp);
}


// given, alt, lat1, lon1, az1 and distance (s), calculate lat2, lon2
// and az2.  Lat, lon, and azimuth are in degrees.  distance in meters
int geo_direct_wgs_84 ( double alt, double lat1, double lon1, double az1, 
			double s, double *lat2, double *lon2,  double *az2 );


// given alt, lat1, lon1, lat2, lon2, calculate starting and ending
// az1, az2 and distance (s).  Lat, lon, and azimuth are in degrees.
// distance in meters
int geo_inverse_wgs_84( double alt, double lat1, double lon1, double lat2,
			double lon2, double *az1, double *az2, double *s );


/***************************************************************************

	TITLE:	ls_geodesy
	
----------------------------------------------------------------------------

	FUNCTION:	Converts geocentric coordinates to geodetic positions

----------------------------------------------------------------------------

	MODULE STATUS:	developmental

----------------------------------------------------------------------------

	GENEALOGY:	Written as part of LaRCSim project by E. B. Jackson

----------------------------------------------------------------------------

	DESIGNED BY:	E. B. Jackson
	
	CODED BY:	E. B. Jackson
	
	MAINTAINED BY:	E. B. Jackson

----------------------------------------------------------------------------

	MODIFICATION HISTORY:
	
	DATE	PURPOSE						BY
	
	930208	Modified to avoid singularity near polar region.	EBJ
	930602	Moved backwards calcs here from ls_step.		EBJ
	931214	Changed erroneous Latitude and Altitude variables to 
		*lat_geod and *alt in routine ls_geoc_to_geod.		EBJ
	940111	Changed header files from old ls_eom.h style to ls_types, 
		and ls_constants.  Also replaced old DATA type with new
		SCALAR type.						EBJ

	CURRENT RCS HEADER:

$Header$

 * Revision 1.5  1994/01/11  18:47:05  bjax
 * Changed include files to use types and constants, not ls_eom.h
 * Also changed DATA type to SCALAR type.
 *
 * Revision 1.4  1993/12/14  21:06:47  bjax
 * Removed global variable references Altitude and Latitude.   EBJ
 *
 * Revision 1.3  1993/06/02  15:03:40  bjax
 * Made new subroutine for calculating geodetic to geocentric; changed name
 * of forward conversion routine from ls_geodesy to ls_geoc_to_geod.
 *

----------------------------------------------------------------------------

	REFERENCES:

		[ 1]	Stevens, Brian L.; and Lewis, Frank L.: "Aircraft 
			Control and Simulation", Wiley and Sons, 1992.
			ISBN 0-471-61397-5		      


----------------------------------------------------------------------------

	CALLED BY:	ls_aux

----------------------------------------------------------------------------

	CALLS TO:

----------------------------------------------------------------------------

	INPUTS:	
		lat_geoc	Geocentric latitude, radians, + = North
		radius		C.G. radius to earth center, ft

----------------------------------------------------------------------------

	OUTPUTS:
		lat_geod	Geodetic latitude, radians, + = North
		alt		C.G. altitude above mean sea level, ft
		sea_level_r	radius from earth center to sea level at
				local vertical (surface normal) of C.G.

--------------------------------------------------------------------------*/


#endif // _SG_GEODESY_HXX
