// sg_geodesy.cxx -- routines to convert between geodetic and geocentric 
//                   coordinate systems.
//
// Copied and adapted directly from LaRCsim/ls_geodesy.c
//
// See below for the complete original LaRCsim comments.
//
// $Id$

#include <simgear/compiler.h>

#ifdef SG_HAVE_STD_INCLUDES
# include <cmath>
# include <cerrno>
# include <cstdio>
#else
# include <math.h>
# include <errno.h>
# include <stdio.h>
#endif

#include <simgear/constants.h>
#include <simgear/debug/logstream.hxx>

#include "point3d.hxx"
#include "sg_geodesy.hxx"
#include "localconsts.hxx"


#ifndef SG_HAVE_NATIVE_SGI_COMPILERS
SG_USING_STD(cout);
#endif


#define DOMAIN_ERR_DEBUG 1


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
		   *lat_geod, double *alt, double *sea_level_r )
{
#ifdef DOMAIN_ERR_DEBUG
    errno = 0;			// start with error zero'd
#endif

    double t_lat, x_alpha, mu_alpha, delt_mu, r_alpha, l_point, rho_alpha;
    double sin_mu_a, denom,delt_lambda, lambda_sl, sin_lambda_sl;

    if( ( (SGD_PI_2 - lat_geoc) < SG_ONE_SECOND )        // near North pole
	|| ( (SGD_PI_2 + lat_geoc) < SG_ONE_SECOND ) )   // near South pole
    {
	*lat_geod = lat_geoc;
	*sea_level_r = SG_EQUATORIAL_RADIUS_M*E;
	*alt = radius - *sea_level_r;
    } else {
	// cout << "  lat_geoc = " << lat_geoc << endl;
	t_lat = tan(lat_geoc);
	// cout << "  tan(t_lat) = " << t_lat << endl;
	x_alpha = E*SG_EQUATORIAL_RADIUS_M/sqrt(t_lat*t_lat + E*E);
#ifdef DOMAIN_ERR_DEBUG
	if ( errno ) {
	    perror("fgGeocToGeod()");
	    FG_LOG( FG_GENERAL, FG_ALERT, "sqrt(" << t_lat*t_lat + E*E << ")" );
	}
#endif
	// cout << "  x_alpha = " << x_alpha << endl;
	double tmp = sqrt(SG_EQ_RAD_SQUARE_M - x_alpha * x_alpha);
	if ( tmp < 0.0 ) { tmp = 0.0; }
#ifdef DOMAIN_ERR_DEBUG
	if ( errno ) {
	    perror("fgGeocToGeod()");
	    FG_LOG( FG_GENERAL, FG_ALERT, "sqrt(" << SG_EQ_RAD_SQUARE_M - x_alpha * x_alpha
		    << ")" );
	}
#endif
	mu_alpha = atan2(tmp,E*x_alpha);
	if (lat_geoc < 0) mu_alpha = - mu_alpha;
	sin_mu_a = sin(mu_alpha);
	delt_lambda = mu_alpha - lat_geoc;
	r_alpha = x_alpha/cos(lat_geoc);
	l_point = radius - r_alpha;
	*alt = l_point*cos(delt_lambda);

	denom = sqrt(1-EPS*EPS*sin_mu_a*sin_mu_a);
#ifdef DOMAIN_ERR_DEBUG
	if ( errno ) {
	    perror("fgGeocToGeod()");
	    FG_LOG( FG_GENERAL, FG_ALERT, "sqrt(" <<
		    1-EPS*EPS*sin_mu_a*sin_mu_a << ")" );
	}
#endif
	rho_alpha = SG_EQUATORIAL_RADIUS_M*(1-EPS)/
	    (denom*denom*denom);
	delt_mu = atan2(l_point*sin(delt_lambda),rho_alpha + *alt);
	*lat_geod = mu_alpha - delt_mu;
	lambda_sl = atan( E*E * tan(*lat_geod) ); // SL geoc. latitude
	sin_lambda_sl = sin( lambda_sl );
	*sea_level_r = 
	    sqrt(SG_EQ_RAD_SQUARE_M / (1 + ((1/(E*E))-1)*sin_lambda_sl*sin_lambda_sl));
#ifdef DOMAIN_ERR_DEBUG
	if ( errno ) {
	    perror("fgGeocToGeod()");
	    FG_LOG( FG_GENERAL, FG_ALERT, "sqrt(" <<
		    SG_EQ_RAD_SQUARE_M / (1 + ((1/(E*E))-1)*sin_lambda_sl*sin_lambda_sl)
		    << ")" );
	}
#endif

    }

}


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
		      double *lat_geoc )
{
    double lambda_sl, sin_lambda_sl, cos_lambda_sl, sin_mu, cos_mu, px, py;
    
#ifdef DOMAIN_ERR_DEBUG
    errno = 0;
#endif

    lambda_sl = atan( E*E * tan(lat_geod) ); // sea level geocentric latitude
    sin_lambda_sl = sin( lambda_sl );
    cos_lambda_sl = cos( lambda_sl );
    sin_mu = sin(lat_geod);                  // Geodetic (map makers') latitude
    cos_mu = cos(lat_geod);
    *sl_radius = 
	sqrt(SG_EQ_RAD_SQUARE_M / (1 + ((1/(E*E))-1)*sin_lambda_sl*sin_lambda_sl));
#ifdef DOMAIN_ERR_DEBUG
	if ( errno ) {
	    perror("fgGeodToGeoc()");
	    FG_LOG( FG_GENERAL, FG_ALERT, "sqrt(" <<
		    SG_EQ_RAD_SQUARE_M / (1 + ((1/(E*E))-1)*sin_lambda_sl*sin_lambda_sl)
		    << ")" );
	}
#endif
    py = *sl_radius*sin_lambda_sl + alt*sin_mu;
    px = *sl_radius*cos_lambda_sl + alt*cos_mu;
    *lat_geoc = atan2( py, px );
}


// Direct and inverse distance functions 
//
// Proceedings of the 7th International Symposium on Geodetic
// Computations, 1985
//
// "The Nested Coefficient Method for Accurate Solutions of Direct and
// Inverse Geodetic Problems With Any Length"
//
// Zhang Xue-Lian
// pp 747-763
//
// modified for FlightGear to use WGS84 only -- Norman Vine

#define GEOD_INV_PI SGD_PI

// s == distance
// az = azimuth

// for WGS_84 a = 6378137.000, rf = 298.257223563;

static double M0( double e2 ) {
    //double e4 = e2*e2;
    return GEOD_INV_PI*(1.0 - e2*( 1.0/4.0 + e2*( 3.0/64.0 + 
						  e2*(5.0/256.0) )))/2.0;
}


// given, alt, lat1, lon1, az1 and distance (s), calculate lat2, lon2
// and az2.  Lat, lon, and azimuth are in degrees.  distance in meters
int geo_direct_wgs_84 ( double alt, double lat1, double lon1, double az1, 
			double s, double *lat2, double *lon2,  double *az2 )
{
    double a = 6378137.000, rf = 298.257223563;
    double RADDEG = (GEOD_INV_PI)/180.0, testv = 1.0E-10;
    double f = ( rf > 0.0 ? 1.0/rf : 0.0 );
    double b = a*(1.0-f);
    double e2 = f*(2.0-f);
    double phi1 = lat1*RADDEG, lam1 = lon1*RADDEG;
    double sinphi1 = sin(phi1), cosphi1 = cos(phi1);
    double azm1 = az1*RADDEG;
    double sinaz1 = sin(azm1), cosaz1 = cos(azm1);
	
	
    if( fabs(s) < 0.01 ) {	// distance < centimeter => congruency
	*lat2 = lat1;
	*lon2 = lon1;
	*az2 = 180.0 + az1;
	if( *az2 > 360.0 ) *az2 -= 360.0;
	return 0;
    } else if( cosphi1 ) {	// non-polar origin
	// u1 is reduced latitude
	double tanu1 = sqrt(1.0-e2)*sinphi1/cosphi1;
	double sig1 = atan2(tanu1,cosaz1);
	double cosu1 = 1.0/sqrt( 1.0 + tanu1*tanu1 ), sinu1 = tanu1*cosu1;
	double sinaz =  cosu1*sinaz1, cos2saz = 1.0-sinaz*sinaz;
	double us = cos2saz*e2/(1.0-e2);

	// Terms
	double	ta = 1.0+us*(4096.0+us*(-768.0+us*(320.0-175.0*us)))/16384.0,
	    tb = us*(256.0+us*(-128.0+us*(74.0-47.0*us)))/1024.0,
	    tc = 0;

	// FIRST ESTIMATE OF SIGMA (SIG)
	double first = s/(b*ta);  // !!
	double sig = first;
	double c2sigm, sinsig,cossig, temp,denom,rnumer, dlams, dlam;
	do {
	    c2sigm = cos(2.0*sig1+sig);
	    sinsig = sin(sig); cossig = cos(sig);
	    temp = sig;
	    sig = first + 
		tb*sinsig*(c2sigm+tb*(cossig*(-1.0+2.0*c2sigm*c2sigm) - 
				      tb*c2sigm*(-3.0+4.0*sinsig*sinsig)
				      *(-3.0+4.0*c2sigm*c2sigm)/6.0)
			   /4.0);
	} while( fabs(sig-temp) > testv);

	// LATITUDE OF POINT 2
	// DENOMINATOR IN 2 PARTS (TEMP ALSO USED LATER)
	temp = sinu1*sinsig-cosu1*cossig*cosaz1;
	denom = (1.0-f)*sqrt(sinaz*sinaz+temp*temp);

	// NUMERATOR
	rnumer = sinu1*cossig+cosu1*sinsig*cosaz1;
	*lat2 = atan2(rnumer,denom)/RADDEG;

	// DIFFERENCE IN LONGITUDE ON AUXILARY SPHERE (DLAMS )
	rnumer = sinsig*sinaz1;
	denom = cosu1*cossig-sinu1*sinsig*cosaz1;
	dlams = atan2(rnumer,denom);

	// TERM C
	tc = f*cos2saz*(4.0+f*(4.0-3.0*cos2saz))/16.0;

	// DIFFERENCE IN LONGITUDE
	dlam = dlams-(1.0-tc)*f*sinaz*(sig+tc*sinsig*
				       (c2sigm+
					tc*cossig*(-1.0+2.0*
						   c2sigm*c2sigm)));
	*lon2 = (lam1+dlam)/RADDEG;
	if (*lon2 > 180.0  ) *lon2 -= 360.0;
	if (*lon2 < -180.0 ) *lon2 += 360.0;

	// AZIMUTH - FROM NORTH
	*az2 = atan2(-sinaz,temp)/RADDEG;
	if ( fabs(*az2) < testv ) *az2 = 0.0;
	if( *az2 < 0.0) *az2 += 360.0;
	return 0;
    } else {			// phi1 == 90 degrees, polar origin
	double dM = a*M0(e2) - s;
	double paz = ( phi1 < 0.0 ? 180.0 : 0.0 );
	return geo_direct_wgs_84( alt, 0.0, lon1, paz, dM,lat2,lon2,az2 );
    } 
}


// given alt, lat1, lon1, lat2, lon2, calculate starting and ending
// az1, az2 and distance (s).  Lat, lon, and azimuth are in degrees.
// distance in meters
int geo_inverse_wgs_84( double alt, double lat1, double lon1, double lat2,
			double lon2, double *az1, double *az2, double *s )
{
    double a = 6378137.000, rf = 298.257223563;
    int iter=0;
    double RADDEG = (GEOD_INV_PI)/180.0, testv = 1.0E-10;
    double f = ( rf > 0.0 ? 1.0/rf : 0.0 );
    double b = a*(1.0-f);
    // double e2 = f*(2.0-f); // unused in this routine
    double phi1 = lat1*RADDEG, lam1 = lon1*RADDEG;
    double sinphi1 = sin(phi1), cosphi1 = cos(phi1);
    double phi2 = lat2*RADDEG, lam2 = lon2*RADDEG;
    double sinphi2 = sin(phi2), cosphi2 = cos(phi2);
	
    if( (fabs(lat1-lat2) < testv && 
	 ( fabs(lon1-lon2) < testv) || fabs(lat1-90.0) < testv ) )
    {	
	// TWO STATIONS ARE IDENTICAL : SET DISTANCE & AZIMUTHS TO ZERO */
	*az1 = 0.0; *az2 = 0.0; *s = 0.0;
	return 0;
    } else if(  fabs(cosphi1) < testv ) {
	// initial point is polar
	int k = geo_inverse_wgs_84( alt, lat2,lon2,lat1,lon1, az1,az2,s );
	k = k; // avoid compiler error since return result is unused
	b = *az1; *az1 = *az2; *az2 = b;
	return 0;
    } else if( fabs(cosphi2) < testv ) {
	// terminal point is polar
	int k = geo_inverse_wgs_84( alt, lat1,lon1,lat1,lon1+180.0, 
				    az1,az2,s );
	k = k; // avoid compiler error since return result is unused
	*s /= 2.0;
	*az2 = *az1 + 180.0;
	if( *az2 > 360.0 ) *az2 -= 360.0; 
	return 0;
    } else if( (fabs( fabs(lon1-lon2) - 180 ) < testv) && 
	       (fabs(lat1+lat2) < testv) ) 
    {
	// Geodesic passes through the pole (antipodal)
	double s1,s2;
	geo_inverse_wgs_84( alt, lat1,lon1, lat1,lon2, az1,az2, &s1 );
	geo_inverse_wgs_84( alt, lat2,lon2, lat1,lon2, az1,az2, &s2 );
	*az2 = *az1;
	*s = s1 + s2;
	return 0;
    } else {
	// antipodal and polar points don't get here
	double dlam = lam2 - lam1, dlams = dlam;
	double sdlams,cdlams, sig,sinsig,cossig, sinaz,
	    cos2saz, c2sigm;
	double tc,temp, us,rnumer,denom, ta,tb;
	double cosu1,sinu1, sinu2,cosu2;

	// Reduced latitudes
	temp = (1.0-f)*sinphi1/cosphi1;
	cosu1 = 1.0/sqrt(1.0+temp*temp);
	sinu1 = temp*cosu1;
	temp = (1.0-f)*sinphi2/cosphi2;
	cosu2 = 1.0/sqrt(1.0+temp*temp);
	sinu2 = temp*cosu2;
    
	do {
	    sdlams = sin(dlams), cdlams = cos(dlams);
	    sinsig = sqrt(cosu2*cosu2*sdlams*sdlams+
			  (cosu1*sinu2-sinu1*cosu2*cdlams)*
			  (cosu1*sinu2-sinu1*cosu2*cdlams));
	    cossig = sinu1*sinu2+cosu1*cosu2*cdlams;
	    
	    sig = atan2(sinsig,cossig);
	    sinaz = cosu1*cosu2*sdlams/sinsig;
	    cos2saz = 1.0-sinaz*sinaz;
	    c2sigm = (sinu1 == 0.0 || sinu2 == 0.0 ? cossig : 
		      cossig-2.0*sinu1*sinu2/cos2saz);
	    tc = f*cos2saz*(4.0+f*(4.0-3.0*cos2saz))/16.0;
	    temp = dlams;
	    dlams = dlam+(1.0-tc)*f*sinaz*
		(sig+tc*sinsig*
		 (c2sigm+tc*cossig*(-1.0+2.0*c2sigm*c2sigm)));
	    if (fabs(dlams) > GEOD_INV_PI && iter++ > 50) {
		return iter;
	    }
	} while ( fabs(temp-dlams) > testv);

	us = cos2saz*(a*a-b*b)/(b*b); // !!
	// BACK AZIMUTH FROM NORTH
	rnumer = -(cosu1*sdlams);
	denom = sinu1*cosu2-cosu1*sinu2*cdlams;
	*az2 = atan2(rnumer,denom)/RADDEG;
	if( fabs(*az2) < testv ) *az2 = 0.0;
	if(*az2 < 0.0) *az2 += 360.0;

	// FORWARD AZIMUTH FROM NORTH
	rnumer = cosu2*sdlams;
	denom = cosu1*sinu2-sinu1*cosu2*cdlams;
	*az1 = atan2(rnumer,denom)/RADDEG;
	if( fabs(*az1) < testv ) *az1 = 0.0;
	if(*az1 < 0.0) *az1 += 360.0;

	// Terms a & b
	ta = 1.0+us*(4096.0+us*(-768.0+us*(320.0-175.0*us)))/
	    16384.0;
	tb = us*(256.0+us*(-128.0+us*(74.0-47.0*us)))/1024.0;

	// GEODETIC DISTANCE
	*s = b*ta*(sig-tb*sinsig*
		   (c2sigm+tb*(cossig*(-1.0+2.0*c2sigm*c2sigm)-tb*
			       c2sigm*(-3.0+4.0*sinsig*sinsig)*
			       (-3.0+4.0*c2sigm*c2sigm)/6.0)/
		    4.0));
	return 0;
    }
}


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


