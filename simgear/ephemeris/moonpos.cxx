/**************************************************************************
 * moonpos.cxx
 * Written by Durk Talsma. Originally started October 1997, for distribution
 * with the FlightGear project. Version 2 was written in August and
 * September 1998. This code is based upon algorithms and data kindly
 * provided by Mr. Paul Schlyter. (pausch@saaf.se).
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

#include <simgear_config.h>
#include <string.h>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/SGMath.hxx>

#include <math.h>

// #include <FDM/flight.hxx>

#include "moonpos.hxx"


/*************************************************************************
 * MoonPos::MoonPos(double mjd)
 * Public constructor for class MoonPos. Initializes the orbital elements and
 * sets up the moon texture.
 * Argument: The current time.
 * the hard coded orbital elements for MoonPos are passed to
 * CelestialBody::CelestialBody();
 ************************************************************************/
MoonPos::MoonPos(double mjd) :
  CelestialBody(125.1228, -0.0529538083,
		5.1454,    0.00000,
		318.0634,  0.1643573223,
		60.266600, 0.000000,
		0.054900,  0.000000,
		115.3654,  13.0649929509, mjd)
{
}

MoonPos::MoonPos() :
  CelestialBody(125.1228, -0.0529538083,
		5.1454,    0.00000,
		318.0634,  0.1643573223,
		60.266600, 0.000000,
		0.054900,  0.000000,
		115.3654,  13.0649929509)
{
}


MoonPos::~MoonPos()
{
}


/*****************************************************************************
 * void MoonPos::updatePositionTopo(double mjd, double lst, double
 * lat, Star *ourSun) this member function calculates the actual
 * topocentric position (i.e.)  the position of the moon as seen from
 * the current position on the surface of the earth. This include
 * parallax effects, the moon appears on different stars background
 * for different observers on the surface of the earth.
 ****************************************************************************/
void MoonPos::updatePositionTopo(double mjd, double lst, double lat, Star *ourSun)
{
  double
    eccAnom, ecl, actTime,
    xv, yv, v, r, xh, yh, zh, zg, xe,
    Ls, Lm, D, F, mpar, gclat, rho, HA, g,
    geoRa, geoDec,
    cosN, sinN, cosvw, sinvw, sinvw_cosi, cosecl, sinecl, rcoslatEcl,
    FlesstwoD, MlesstwoD, twoD, twoM, twolat, alpha;

  double max_loglux = -0.504030345621;
  double min_loglux = -4.39964634562;
  double conv = 1.0319696543787917;    // The log foot-candle to log lux conversion factor.
  updateOrbElements(mjd);
  actTime = sgCalcActTime(mjd);

  // calculate the angle between ecliptic and equatorial coordinate system
  // in Radians
  ecl = SGD_DEGREES_TO_RADIANS * (23.4393 - 3.563E-7 * actTime);
  eccAnom = sgCalcEccAnom(M, e);  // Calculate the eccentric anomaly
  xv = a * (cos(eccAnom) - e);
  yv = a * (sqrt(1.0 - e*e) * sin(eccAnom));
  v = atan2(yv, xv);               // the moon's true anomaly
  r = sqrt (xv*xv + yv*yv);       // and its distance

  // repetitive calculations, minimised for speed
  cosN = cos(N);
  sinN = sin(N);
  cosvw = cos(v+w);
  sinvw = sin(v+w);
  sinvw_cosi = sinvw * cos(i);
  cosecl = cos(ecl);
  sinecl = sin(ecl);

  // estimate the geocentric rectangular coordinates here
  xh = r * (cosN * cosvw - sinN * sinvw_cosi);
  yh = r * (sinN * cosvw + cosN * sinvw_cosi);
  zh = r * (sinvw * sin(i));

  // calculate the ecliptic latitude and longitude here
  lonEcl = atan2 (yh, xh);
  latEcl = atan2(zh, sqrt(xh*xh + yh*yh));

  /* Calculate a number of perturbation, i.e. disturbances caused by the
   * gravitational influence of the sun and the other major planets.
   * The largest of these even have a name */
  Ls = ourSun->getM() + ourSun->getw();
  Lm = M + w + N;
  D = Lm - Ls;
  F = Lm - N;

  twoD = 2 * D;
  twoM = 2 * M;
  FlesstwoD = F - twoD;
  MlesstwoD = M - twoD;

  lonEcl += SGD_DEGREES_TO_RADIANS * (-1.274 * sin(MlesstwoD)
			  +0.658 * sin(twoD)
			  -0.186 * sin(ourSun->getM())
			  -0.059 * sin(twoM - twoD)
			  -0.057 * sin(MlesstwoD + ourSun->getM())
			  +0.053 * sin(M + twoD)
			  +0.046 * sin(twoD - ourSun->getM())
			  +0.041 * sin(M - ourSun->getM())
			  -0.035 * sin(D)
			  -0.031 * sin(M + ourSun->getM())
			  -0.015 * sin(2*F - twoD)
			  +0.011 * sin(M - 4*D)
			  );
  latEcl += SGD_DEGREES_TO_RADIANS * (-0.173 * sin(FlesstwoD)
			  -0.055 * sin(M - FlesstwoD)
			  -0.046 * sin(M + FlesstwoD)
			  +0.033 * sin(F + twoD)
			  +0.017 * sin(twoM + F)
			  );
  r += (-0.58 * cos(MlesstwoD)
	-0.46 * cos(twoD)
	);
  distance = r;
  distance_in_a = r/a;
  // SG_LOG(SG_GENERAL, SG_INFO, "Running moon update");
  rcoslatEcl = r * cos(latEcl);
  xg =     cos(lonEcl) * rcoslatEcl;
  yg =     sin(lonEcl) * rcoslatEcl;
  zg = r *               sin(latEcl);

  xe = xg;
  ye = yg * cosecl -zg * sinecl;
  ze = yg * sinecl +zg * cosecl;

  geoRa  = atan2(ye, xe);
  geoDec = atan2(ze, sqrt(xe*xe + ye*ye));

  /* SG_LOG( SG_GENERAL, SG_INFO,
	  "(geocentric) geoRa = (" << (SGD_RADIANS_TO_DEGREES * geoRa)
	  << "), geoDec= (" << (SGD_RADIANS_TO_DEGREES * geoDec) << ")" ); */


  // Given the moon's geocentric ra and dec, calculate its
  // topocentric ra and dec. i.e. the position as seen from the
  // surface of the earth, instead of the center of the earth

  // First calculate the moon's parallax, that is, the apparent size of the
  // (equatorial) radius of the earth, as seen from the moon
  mpar = asin ( 1 / r);
  // SG_LOG( SG_GENERAL, SG_INFO, "r = " << r << " mpar = " << mpar );
  // SG_LOG( SG_GENERAL, SG_INFO, "lat = " << f->get_Latitude() );

  twolat = 2 * SGD_DEGREES_TO_RADIANS * lat;
  gclat = lat - 0.003358 * sin(twolat);
  // SG_LOG( SG_GENERAL, SG_INFO, "gclat = " << gclat );

  rho = 0.99883 + 0.00167 * cos(twolat);
  // SG_LOG( SG_GENERAL, SG_INFO, "rho = " << rho );

  if (geoRa < 0)
    geoRa += SGD_2PI;

  HA = lst - (3.8197186 * geoRa);
  /* SG_LOG( SG_GENERAL, SG_INFO, "t->getLst() = " << t->getLst()
	  << " HA = " << HA ); */

  g = atan (tan(gclat) / cos ((HA / 3.8197186)));
  // SG_LOG( SG_GENERAL, SG_INFO, "g = " << g );

  rightAscension = geoRa - mpar * rho * cos(gclat) * sin(HA) / cos (geoDec);
  if (fabs(lat) > 0) {
      declination
	  = geoDec - mpar * rho * sin (gclat) * sin (g - geoDec) / sin(g);
  } else {
      declination = geoDec;
      // cerr << "Geocentric vs. Topocentric position" << endl;
      // cerr << "RA (difference)  : "
      //      << SGD_RADIANS_TO_DEGREES * (geoRa - rightAscension) << endl;
      // cerr << "Dec (difference) : "
      //      << SGD_RADIANS_TO_DEGREES * (geoDec - declination) << endl;
  }

  /* SG_LOG( SG_GENERAL, SG_INFO,
	  "Ra = (" << (SGD_RADIANS_TO_DEGREES *rightAscension)
	  << "), Dec= (" << (SGD_RADIANS_TO_DEGREES *declination) << ")" ); */

  // Moon age and phase calculation
  age = lonEcl - ourSun->getlonEcl();
  phase = (1 - cos(age)) / 2;

  // The log of the illuminance of the moon outside the atmosphere.
  // This is the base 10 log of equation 20 from Krisciunas K. and Schaefer B.E.
  // (1991). A model of the brightness of moonlight, Publ. Astron. Soc. Pacif.
  // 103(667), 1033-1039 (DOI: http://dx.doi.org/10.1086/132921).
  alpha = SGD_RADIANS_TO_DEGREES * SGMiscd::normalizeAngle(age + SGMiscd::pi());
  log_I = -0.4 * (3.84 + 0.026*fabs(alpha) + 4e-9*pow(alpha, 4.0));

  // Convert from foot-candles to lux.
  log_I += conv;

  // The moon's illuminance factor, bracketed between 0 and 1.
  I_factor = (log_I - max_loglux) / (max_loglux - min_loglux) + 1.0;
  I_factor = SGMiscd::clip(I_factor, 0, 1);
}



/*****************************************************************************
 * void MoonPos::updatePosition(double mjd, Star *ourSun) this member
 * function calculates the geocentric position (i.e.)  the position of
 * the moon as seen from the center of earth. As such, it does not
 * include any parallax effects. These are taken into account during
 * the rendering.
 ****************************************************************************/
void MoonPos::updatePosition(double mjd, Star *ourSun)
{
  double
    eccAnom, ecl, actTime,
    xv, yv, v, r, xh, yh, zh, zg, xe,
    Ls, Lm, D, F, geoRa, geoDec,
    cosN, sinN, cosvw, sinvw, sinvw_cosi, cosecl, sinecl, rcoslatEcl,
    FlesstwoD, MlesstwoD, twoD, twoM, alpha;

  double max_loglux = -0.504030345621;
  double min_loglux = -4.39964634562;
  double conv = 1.0319696543787917;    // The log foot-candle to log lux conversion factor.
  updateOrbElements(mjd);
  actTime = sgCalcActTime(mjd);

  // calculate the angle between ecliptic and equatorial coordinate system
  // in Radians
  ecl = SGD_DEGREES_TO_RADIANS * (23.4393 - 3.563E-7 * actTime);
  eccAnom = sgCalcEccAnom(M, e);  // Calculate the eccentric anomaly
  xv = a * (cos(eccAnom) - e);
  yv = a * (sqrt(1.0 - e*e) * sin(eccAnom));
  v = atan2(yv, xv);               // the moon's true anomaly
  r = sqrt (xv*xv + yv*yv);       // and its distance

  // repetitive calculations, minimised for speed
  cosN = cos(N);
  sinN = sin(N);
  cosvw = cos(v+w);
  sinvw = sin(v+w);
  sinvw_cosi = sinvw * cos(i);
  cosecl = cos(ecl);
  sinecl = sin(ecl);

  // estimate the geocentric rectangular coordinates here
  xh = r * (cosN * cosvw - sinN * sinvw_cosi);
  yh = r * (sinN * cosvw + cosN * sinvw_cosi);
  zh = r * (sinvw * sin(i));

  // calculate the ecliptic latitude and longitude here
  lonEcl = atan2 (yh, xh);
  latEcl = atan2(zh, sqrt(xh*xh + yh*yh));

  /* Calculate a number of perturbation, i.e. disturbances caused by the
   * gravitational influence of the sun and the other major planets.
   * The largest of these even have a name */
  Ls = ourSun->getM() + ourSun->getw();
  Lm = M + w + N;
  D = Lm - Ls;
  F = Lm - N;

  twoD = 2 * D;
  twoM = 2 * M;
  FlesstwoD = F - twoD;
  MlesstwoD = M - twoD;

  lonEcl += SGD_DEGREES_TO_RADIANS * (-1.274 * sin(MlesstwoD)
			  +0.658 * sin(twoD)
			  -0.186 * sin(ourSun->getM())
			  -0.059 * sin(twoM - twoD)
			  -0.057 * sin(MlesstwoD + ourSun->getM())
			  +0.053 * sin(M + twoD)
			  +0.046 * sin(twoD - ourSun->getM())
			  +0.041 * sin(M - ourSun->getM())
			  -0.035 * sin(D)
			  -0.031 * sin(M + ourSun->getM())
			  -0.015 * sin(2*F - twoD)
			  +0.011 * sin(M - 4*D)
			  );
  latEcl += SGD_DEGREES_TO_RADIANS * (-0.173 * sin(FlesstwoD)
			  -0.055 * sin(M - FlesstwoD)
			  -0.046 * sin(M + FlesstwoD)
			  +0.033 * sin(F + twoD)
			  +0.017 * sin(twoM + F)
			  );
  r += (-0.58 * cos(MlesstwoD)
	-0.46 * cos(twoD)
	);
  // from the orbital elements, the unit of the distance is in Earth radius, around 60.
  distance = r;
  distance_in_a = r/a;
  // SG_LOG(SG_GENERAL, SG_INFO, "Running moon update");
  rcoslatEcl = r * cos(latEcl);
  xg =     cos(lonEcl) * rcoslatEcl;
  yg =     sin(lonEcl) * rcoslatEcl;
  zg = r *               sin(latEcl);

  xe = xg;
  ye = yg * cosecl -zg * sinecl;
  ze = yg * sinecl +zg * cosecl;

  geoRa  = atan2(ye, xe);
  geoDec = atan2(ze, sqrt(xe*xe + ye*ye));

  if (geoRa < 0)
    geoRa += SGD_2PI;

  rightAscension = geoRa;
  declination = geoDec;


  /* SG_LOG( SG_GENERAL, SG_INFO,
	  "Ra = (" << (SGD_RADIANS_TO_DEGREES *rightAscension)
	  << "), Dec= (" << (SGD_RADIANS_TO_DEGREES *declination) << ")" ); */

  // Moon age and phase calculation
  age = lonEcl - ourSun->getlonEcl();
  phase = (1 - cos(age)) / 2;

  // The log of the illuminance of the moon outside the atmosphere.
  // This is the base 10 log of equation 20 from Krisciunas K. and Schaefer B.E.
  // (1991). A model of the brightness of moonlight, Publ. Astron. Soc. Pacif.
  // 103(667), 1033-1039 (DOI: http://dx.doi.org/10.1086/132921).
  alpha = SGD_RADIANS_TO_DEGREES * SGMiscd::normalizeAngle(age + SGMiscd::pi());
  log_I = -0.4 * (3.84 + 0.026*fabs(alpha) + 4e-9*pow(alpha, 4.0));

  // Convert from foot-candles to lux.
  log_I += conv;

  // The moon's illuminance factor, bracketed between 0 and 1.
  I_factor = (log_I - max_loglux) / (max_loglux - min_loglux) + 1.0;
  I_factor = SGMiscd::clip(I_factor, 0, 1);
}
