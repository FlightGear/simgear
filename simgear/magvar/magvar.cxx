// magvar.cxx -- compute local magnetic variation given position,
//               altitude, and date
//
// This is an implementation of the NIMA (formerly DMA) WMM2000
//
//    http://www.nima.mil/GandG/ngdc-wmm2000.html
//
// Copyright (C) 2000  Edward A Williams <Ed_Williams@compuserve.com>
//
// Adapted from Excel 3.0 version 3/27/94 EAW
// Recoded in C++ by Starry Chan
// WMM95 added and rearranged in ANSI-C EAW 7/9/95
// Put shell around program and made Borland & GCC compatible EAW 11/22/95
// IGRF95 added 2/96 EAW
// WMM2000 IGR2000 added 2/00 EAW
// Released under GPL 3/26/00 EAW
// Adaptions and modifications for the SimGear project  3/27/2000 CLO
//
// Removed all pow() calls and made static roots[][] arrays to
// save many sqrt() calls on subsequent invocations
// left old code as SGMagVarOrig() for testing purposes
// 3/28/2000  Norman Vine -- nhv@yahoo.com
//
// Put in some bullet-proofing to handle magnetic and geographic poles.
// 3/28/2000 EAW

//  The routine uses a spherical harmonic expansion of the magnetic
// potential up to twelfth order, together with its time variation, as
// described in Chapter 4 of "Geomagnetism, Vol 1, Ed. J.A.Jacobs,
// Academic Press (London 1987)". The program first converts geodetic
// coordinates (lat/long on elliptic earth and altitude) to spherical
// geocentric (spherical lat/long and radius) coordinates. Using this,
// the spherical (B_r, B_theta, B_phi) magnetic field components are
// computed from the model. These are finally referred to surface (X, Y,
// Z) coordinates.
//
//   Fields are accurate to better than 200nT, variation and dip to
// better than 0.5 degrees, with the exception of the declination near
// the magnetic poles (where it is ill-defined) where the error may reach
// 4 degrees or more.
//
//   Variation is undefined at both the geographic and  
// magnetic poles, even though the field itself is well-behaved. To
// avoid the routine blowing up, latitude entries corresponding to
// the geographic poles are slightly offset. At the magnetic poles,
// the routine returns zero variation.


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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "magvar.hxx"


#define	max(a,b)	(((a) > (b)) ? (a) : (b))

static const double pi = 3.14159265358979;
static const double a = 6378.16;	/* major radius (km) IAU66 ellipsoid */
static const double f = 1.0 / 298.25;	/* inverse flattening IAU66 ellipsoid */
static const double b = 6378.16 * (1.0 -1.0 / 298.25 );
/* minor radius b=a*(1-f) */
static const double r_0 = 6371.2;	/* "mean radius" for spherical harmonic expansion */

static double gnm_wmm2000[13][13] =
{
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {-29616.0, -1722.7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {-2266.7, 3070.2, 1677.6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {1322.4, -2291.5, 1255.9, 724.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {932.1, 786.3, 250.6, -401.5, 106.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {-211.9, 351.6, 220.8, -134.5, -168.8, -13.3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {73.8, 68.2, 74.1, -163.5, -3.8, 17.1, -85.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {77.4, -73.9, 2.2, 35.7, 7.3, 5.2, 8.4, -1.5, 0.0, 0.0, 0.0, 0.0, 0.0},
    {23.3, 7.3, -8.5, -6.6, -16.9, 8.6, 4.9, -7.8, -7.6, 0.0, 0.0, 0.0, 0.0},
    {5.7, 8.5, 2.0, -9.8, 7.6, -7.0, -2.0, 9.2, -2.2, -6.6, 0.0, 0.0, 0.0},
    {-2.2, -5.7, 1.6, -3.7, -0.6, 4.1, 2.2, 2.2, 4.6, 2.3, 0.1, 0.0, 0.0},
    {3.3, -1.1, -2.4, 2.6, -1.3, -1.7, -0.6, 0.4, 0.7, -0.3, 2.3, 4.2, 0.0},
    {-1.5, -0.2, -0.3, 0.5, 0.2, 0.9, -1.4, 0.6, -0.6, -1.0, -0.3, 0.3, 0.4},
};

static double hnm_wmm2000[13][13]=
{
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 5194.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, -2484.8, -467.9, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, -224.7, 293.0, -486.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 273.3, -227.9, 120.9, -302.7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 42.0, 173.8, -135.0, -38.6, 105.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, -17.4, 61.2, 63.2, -62.9, 0.2, 43.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, -62.3, -24.5, 8.9, 23.4, 15.0, -27.6, -7.8, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 12.4, -20.8, 8.4, -21.2, 15.5, 9.1, -15.5, -5.4, 0.0, 0.0, 0.0, 0.0},
    {0.0, -20.4, 13.9, 12.0, -6.2, -8.6, 9.4, 5.0, -8.4, 3.2, 0.0, 0.0, 0.0},
    {0.0, 0.9, -0.7, 3.9, 4.8, -5.3, -1.0, -2.4, 1.3, -2.3, -6.4, 0.0, 0.0},
    {0.0, -1.5, 0.7, -1.1, -2.3, 1.3, -0.6, -2.8, -1.6, -0.1, -1.9, 1.4, 0.0},
    {0.0, -1.0, 0.7, 2.2, -2.5, -0.2, 0.0, -0.2, 0.0, 0.2, -0.9, -0.2, 1.0},
};

static double gtnm_wmm2000[13][13]=
{
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {14.7, 11.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {-13.6, -0.7, -1.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.3, -4.3, 0.9, -8.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {-1.6, 0.9, -7.6, 2.2, -3.2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {-0.9, -0.2, -2.5, -2.7, -0.9, 1.7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {1.2, 0.2, 1.7, 1.6, -0.1, -0.3, 0.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {-0.4, -0.8, -0.2, 1.1, 0.4, 0.0, -0.2, -0.2, 0.0, 0.0, 0.0, 0.0, 0.0},
    {-0.3, 0.6, -0.8, 0.3, -0.2, 0.5, 0.0, -0.6, 0.1, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
};

static double htnm_wmm2000[13][13]=
{
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, -20.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, -21.5, -9.6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 6.4, -1.3, -13.3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 2.3, 0.7, 3.7, -0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 2.1, 2.3, 3.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, -0.3, -1.7, -0.9, -1.0, -0.1, 1.9, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 1.4, 0.2, 0.7, 0.4, -0.3, -0.8, -0.1, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, -0.5, 0.1, -0.2, 0.0, 0.1, -0.1, 0.3, 0.2, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
};

static const int nmax = 12;

static double P[13][13];
static double DP[13][13];
static double gnm[13][13];
static double hnm[13][13];
static double sm[13];
static double cm[13];

static double root[13];
static double roots[13][13][2];

/* Convert date to Julian day    1950-2049 */
unsigned long int yymmdd_to_julian_days( int yy, int mm, int dd )
{
    unsigned long jd;
 
    yy = (yy < 50) ? (2000 + yy) : (1900 + yy);
    jd = dd - 32075L + 1461L * (yy + 4800L + (mm - 14) / 12 ) / 4;
    jd = jd + 367L * (mm - 2 - (mm - 14) / 12*12) / 12;
    jd = jd - 3 * ((yy + 4900L + (mm - 14) / 12) / 100) / 4;

    /* printf("julian date = %d\n", jd ); */
    return jd;
} 


/* Convert degrees to radians */
double deg_to_rad( double deg )
{
    return deg*pi/180.;
}


/* Convert radians to degrees */
double rad_to_deg( double rad )
{
    return rad*180./pi;
}
	     

/*
 * return variation (in radians) given geodetic latitude (radians),
 * longitude(radians), height (km) and (Julian) date
 * N and E lat and long are positive, S and W negative
*/

double SGMagVar( double lat, double lon, double h, long dat, double* field )
{
    /* output field B_r,B_th,B_phi,B_x,B_y,B_z */
    int n,m;
    /* reference dates */
    long date0_wmm2000 = yymmdd_to_julian_days(0,1,1);

    double yearfrac,sr,r,theta,c,s,psi,fn,fn_0,B_r,B_theta,B_phi,X,Y,Z;
    double sinpsi, cospsi, inv_s;

    static int been_here = 0;

    double sinlat = sin(lat);
    double coslat = cos(lat);

    /* convert to geocentric coords: */
    // sr = sqrt(pow(a*coslat,2.0)+pow(b*sinlat,2.0));
    sr = sqrt(a*a*coslat*coslat + b*b*sinlat*sinlat);
    /* sr is effective radius */
    theta = atan2(coslat * (h*sr + a*a),
		  sinlat * (h*sr + b*b));
    /* theta is geocentric co-latitude */

    r = h*h + 2.0*h * sr +
	(a*a*a*a - ( a*a*a*a - b*b*b*b ) * sinlat*sinlat ) / 
	(a*a - (a*a - b*b) * sinlat*sinlat );

    r = sqrt(r);

    /* r is geocentric radial distance */
    c = cos(theta);
    s = sin(theta);
    /* protect against zero divide at geographic poles */
    inv_s =  1.0 / (s + (s == 0.)*1.0e-8); 

    /* zero out arrays */
    for ( n = 0; n <= nmax; n++ ) {
	for ( m = 0; m <= n; m++ ) {
	    P[n][m] = 0;
	    DP[n][m] = 0;
	}
    }

    /* diagonal elements */
    P[0][0] = 1;
    P[1][1] = s;
    DP[0][0] = 0;
    DP[1][1] = c;
    P[1][0] = c ;
    DP[1][0] = -s;

    // these values will not change for subsequent function calls
    if( !been_here ) {
	for ( n = 2; n <= nmax; n++ ) {
	    root[n] = sqrt((2.0*n-1) / (2.0*n));
	}

	for ( m = 0; m <= nmax; m++ ) {
	    double mm = m*m;
	    for ( n = max(m + 1, 2); n <= nmax; n++ ) {
		roots[m][n][0] = sqrt((n-1)*(n-1) - mm);
		roots[m][n][1] = 1.0 / sqrt( n*n - mm);
	    }
	}
	been_here = 1;
    }

    for ( n=2; n <= nmax; n++ ) {
	// double root = sqrt((2.0*n-1) / (2.0*n));
	P[n][n] = P[n-1][n-1] * s * root[n];
	DP[n][n] = (DP[n-1][n-1] * s + P[n-1][n-1] * c) *
	    root[n];
    }

    /* lower triangle */
    for ( m = 0; m <= nmax; m++ ) {
	// double mm = m*m;
	for ( n = max(m + 1, 2); n <= nmax; n++ ) {
	    // double root1 = sqrt((n-1)*(n-1) - mm);
	    // double root2 = 1.0 / sqrt( n*n - mm);
	    P[n][m] = (P[n-1][m] * c * (2.0*n-1) -
		       P[n-2][m] * roots[m][n][0]) *
		roots[m][n][1];

	    DP[n][m] = ((DP[n-1][m] * c - P[n-1][m] * s) *
			(2.0*n-1) - DP[n-2][m] * roots[m][n][0]) *
		roots[m][n][1];
	}
    }

    /* compute gnm, hnm at dat */
    /* WMM2000 */
    yearfrac = (dat - date0_wmm2000) / 365.25;
    for ( n = 1; n <= nmax; n++ ) {
	for ( m = 0; m <= nmax; m++ ) {
	    gnm[n][m] = gnm_wmm2000[n][m] + yearfrac * gtnm_wmm2000[n][m];
	    hnm[n][m] = hnm_wmm2000[n][m] + yearfrac * htnm_wmm2000[n][m];
	}
    }

    /* compute sm (sin(m lon) and cm (cos(m lon)) */
    for ( m = 0; m <= nmax; m++ ) {
	sm[m] = sin(m * lon);
	cm[m] = cos(m * lon);
    }

    /* compute B fields */
    B_r = 0.0;
    B_theta = 0.0;
    B_phi = 0.0;
    fn_0 = r_0/r;
    fn = fn_0 * fn_0;

    for ( n = 1; n <= nmax; n++ ) {
	double c1_n=0;
	double c2_n=0;
	double c3_n=0;
	for ( m = 0; m <= n; m++ ) {
	    double tmp = (gnm[n][m] * cm[m] + hnm[n][m] * sm[m]); 
	    c1_n=c1_n + tmp * P[n][m];
	    c2_n=c2_n + tmp * DP[n][m];
	    c3_n=c3_n + m * (gnm[n][m] * sm[m] - hnm[n][m] * cm[m]) * P[n][m];
	}
	// fn=pow(r_0/r,n+2.0);
	fn *= fn_0;
	B_r = B_r + (n + 1) * c1_n * fn;
	B_theta = B_theta - c2_n * fn;
	B_phi = B_phi + c3_n * fn * inv_s;
    }

    /* Find geodetic field components: */
    psi = theta - ((pi / 2.0) - lat);
    sinpsi = sin(psi);
    cospsi = cos(psi);
    X = -B_theta * cospsi - B_r * sinpsi;
    Y = B_phi;
    Z = B_theta * sinpsi - B_r * cospsi;

    field[0]=B_r;
    field[1]=B_theta;
    field[2]=B_phi;
    field[3]=X;
    field[4]=Y;
    field[5]=Z;   /* output fields */

    /* find variation in radians */
    /* return zero variation at magnetic pole X=Y=0. */
    /* E is positive */
    return (X != 0. || Y != 0.) ? atan2(Y, X) : (double) 0.;  
}


#ifdef TEST_NHV_HACKS
double SGMagVarOrig( double lat, double lon, double h, long dat, double* field )
{
    /* output field B_r,B_th,B_phi,B_x,B_y,B_z */
    int n,m;
    /* reference dates */
    long date0_wmm2000 = yymmdd_to_julian_days(0,1,1);

    double yearfrac,sr,r,theta,c,s,psi,fn,B_r,B_theta,B_phi,X,Y,Z;

    /* convert to geocentric coords: */
    sr = sqrt(pow(a*cos(lat),2.0)+pow(b*sin(lat),2.0));
    /* sr is effective radius */
    theta = atan2(cos(lat) * (h * sr + a * a),
		  sin(lat) * (h * sr + b * b));
    /* theta is geocentric co-latitude */

    r = h * h + 2.0*h * sr +
	(pow(a,4.0) - (pow(a,4.0) - pow(b,4.0)) * pow(sin(lat),2.0)) / 
	(a * a - (a * a - b * b) * pow(sin(lat),2.0));

    r = sqrt(r);

    /* r is geocentric radial distance */
    c = cos(theta);
    s = sin(theta);

    /* zero out arrays */
    for ( n = 0; n <= nmax; n++ ) {
	for ( m = 0; m <= n; m++ ) {
	    P[n][m] = 0;
	    DP[n][m] = 0;
	}
    }

    /* diagonal elements */
    P[0][0] = 1;
    P[1][1] = s;
    DP[0][0] = 0;
    DP[1][1] = c;
    P[1][0] = c ;
    DP[1][0] = -s;

    for ( n = 2; n <= nmax; n++ ) {
	P[n][n] = P[n-1][n-1] * s * sqrt((2.0*n-1) / (2.0*n));
	DP[n][n] = (DP[n-1][n-1] * s + P[n-1][n-1] * c) *
	    sqrt((2.0*n-1) / (2.0*n));
    }

    /* lower triangle */
    for ( m = 0; m <= nmax; m++ ) {
	for ( n = max(m + 1, 2); n <= nmax; n++ ) {
	    P[n][m] = (P[n-1][m] * c * (2.0*n-1) - P[n-2][m] *
		       sqrt(1.0*(n-1)*(n-1) - m * m)) /
		sqrt(1.0* n * n - m * m);
	    DP[n][m] = ((DP[n-1][m] * c - P[n-1][m] * s) *
			(2.0*n-1) - DP[n-2][m] *
			sqrt(1.0*(n-1) * (n-1) - m * m)) /
		sqrt(1.0* n * n - m * m);
	}
    }

    /* compute gnm, hnm at dat */
    /* WMM2000 */
    yearfrac = (dat - date0_wmm2000) / 365.25;
    for ( n = 1; n <= nmax; n++ ) {
	for ( m = 0; m <= nmax; m++ ) {
	    gnm[n][m] = gnm_wmm2000[n][m] + yearfrac * gtnm_wmm2000[n][m];
	    hnm[n][m] = hnm_wmm2000[n][m] + yearfrac * htnm_wmm2000[n][m];
	}
    }

    /* compute sm (sin(m lon) and cm (cos(m lon)) */
    for ( m = 0; m <= nmax; m++ ) {
	sm[m] = sin(m * lon);
	cm[m] = cos(m * lon);
    }

    /* compute B fields */
    B_r = 0.0;
    B_theta = 0.0;
    B_phi = 0.0;

    for ( n = 1; n <= nmax; n++ ) {
	double c1_n=0;
	double c2_n=0;
	double c3_n=0;
	for ( m = 0; m <= n; m++ ) {
	    c1_n=c1_n + (gnm[n][m] * cm[m] + hnm[n][m] * sm[m]) * P[n][m];
	    c2_n=c2_n + (gnm[n][m] * cm[m] + hnm[n][m] * sm[m]) * DP[n][m];
	    c3_n=c3_n + m * (gnm[n][m] * sm[m] - hnm[n][m] * cm[m]) * P[n][m];
	}
	fn=pow(r_0/r,n+2.0);
	B_r = B_r + (n + 1) * c1_n * fn;
	B_theta = B_theta - c2_n * fn;
	B_phi = B_phi + c3_n * fn / s;
    }

    /* Find geodetic field components: */
    psi = theta - (pi / 2.0 - lat);
    X = -B_theta * cos(psi) - B_r * sin(psi);
    Y = B_phi;
    Z = B_theta * sin(psi) - B_r * cos(psi);

    field[0]=B_r;
    field[1]=B_theta;
    field[2]=B_phi;
    field[3]=X;
    field[4]=Y;
    field[5]=Z;   /* output fields */

    /* find variation, leave in radians! */
    return atan2(Y, X);  /* E is positive */
}
#endif // TEST_NHV_HACKS
