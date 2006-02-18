
#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <cmath>

#include "SGMath.hxx"

// These are hard numbers from the WGS84 standard.  DON'T MODIFY
// unless you want to change the datum.
#define _EQURAD 6378137.0
#define _FLATTENING 298.257223563

// These are derived quantities more useful to the code:
#if 0
#define _SQUASH (1 - 1/_FLATTENING)
#define _STRETCH (1/_SQUASH)
#define _POLRAD (EQURAD * _SQUASH)
#else
// High-precision versions of the above produced with an arbitrary
// precision calculator (the compiler might lose a few bits in the FPU
// operations).  These are specified to 81 bits of mantissa, which is
// higher than any FPU known to me:
#define _SQUASH    0.9966471893352525192801545
#define _STRETCH   1.0033640898209764189003079
#define _POLRAD    6356752.3142451794975639668
#endif

// The constants from the WGS84 standard
const double SGGeodesy::EQURAD = _EQURAD;
const double SGGeodesy::iFLATTENING = _FLATTENING;
const double SGGeodesy::SQUASH = _SQUASH;
const double SGGeodesy::STRETCH = _STRETCH;
const double SGGeodesy::POLRAD = _POLRAD;

// additional derived and precomputable ones
// for the geodetic conversion algorithm

#define E2 fabs(1 - _SQUASH*_SQUASH)
static double a = _EQURAD;
static double ra2 = 1/(_EQURAD*_EQURAD);
static double e = sqrt(E2);
static double e2 = E2;
static double e4 = E2*E2;

#undef _EQURAD
#undef _FLATTENING
#undef _SQUASH
#undef _STRETCH
#undef _POLRAD
#undef E2

void
SGGeodesy::SGCartToGeod(const SGVec3<double>& cart, SGGeod& geod)
{
  // according to
  // H. Vermeille,
  // Direct transformation from geocentric to geodetic ccordinates,
  // Journal of Geodesy (2002) 76:451-454
  double X = cart(0);
  double Y = cart(1);
  double Z = cart(2);
  double XXpYY = X*X+Y*Y;
  double sqrtXXpYY = sqrt(XXpYY);
  double p = XXpYY*ra2;
  double q = Z*Z*(1-e2)*ra2;
  double r = 1/6.0*(p+q-e4);
  double s = e4*p*q/(4*r*r*r);
  double t = pow(1+s+sqrt(s*(2+s)), 1/3.0);
  double u = r*(1+t+1/t);
  double v = sqrt(u*u+e4*q);
  double w = e2*(u+v-q)/(2*v);
  double k = sqrt(u+v+w*w)-w;
  double D = k*sqrtXXpYY/(k+e2);
  geod.setLongitudeRad(2*atan2(Y, X+sqrtXXpYY));
  double sqrtDDpZZ = sqrt(D*D+Z*Z);
  geod.setLatitudeRad(2*atan2(Z, D+sqrtDDpZZ));
  geod.setElevationM((k+e2-1)*sqrtDDpZZ/k);
}

void
SGGeodesy::SGGeodToCart(const SGGeod& geod, SGVec3<double>& cart)
{
  // according to
  // H. Vermeille,
  // Direct transformation from geocentric to geodetic ccordinates,
  // Journal of Geodesy (2002) 76:451-454
  double lambda = geod.getLongitudeRad();
  double phi = geod.getLatitudeRad();
  double h = geod.getElevationM();
  double sphi = sin(phi);
  double n = a/sqrt(1-e2*sphi*sphi);
  double cphi = cos(phi);
  double slambda = sin(lambda);
  double clambda = cos(lambda);
  cart(0) = (h+n)*cphi*clambda;
  cart(1) = (h+n)*cphi*slambda;
  cart(2) = (h+n-e2*n)*sphi;
}

double
SGGeodesy::SGGeodToSeaLevelRadius(const SGGeod& geod)
{
  // this is just a simplified version of the SGGeodToCart function above,
  // substitute h = 0, take the 2-norm of the cartesian vector and simplify
  double phi = geod.getLatitudeRad();
  double sphi = sin(phi);
  double sphi2 = sphi*sphi;
  return a*sqrt((1 + (e4 - 2*e2)*sphi2)/(1 - e2*sphi2));
}

void
SGGeodesy::SGCartToGeoc(const SGVec3<double>& cart, SGGeoc& geoc)
{
  double minVal = SGLimits<double>::min();
  if (fabs(cart(0)) < minVal && fabs(cart(1)) < minVal)
    geoc.setLongitudeRad(0);
  else
    geoc.setLongitudeRad(atan2(cart(1), cart(0)));

  double nxy = sqrt(cart(0)*cart(0) + cart(1)*cart(1));
  if (fabs(nxy) < minVal && fabs(cart(2)) < minVal)
    geoc.setLatitudeRad(0);
  else
    geoc.setLatitudeRad(atan2(cart(2), nxy));

  geoc.setRadiusM(norm(cart));
}

void
SGGeodesy::SGGeocToCart(const SGGeoc& geoc, SGVec3<double>& cart)
{
  double lat = geoc.getLatitudeRad();
  double lon = geoc.getLongitudeRad();
  double slat = sin(lat);
  double clat = cos(lat);
  double slon = sin(lon);
  double clon = cos(lon);
  cart = geoc.getRadiusM()*SGVec3<double>(clat*clon, clat*slon, slat);
}
