#ifndef _SG_GEODESY_HXX
#define _SG_GEODESY_HXX

#include <simgear/math/point3d.hxx>
#include "SGMath.hxx"

/** 
 * Convert from geocentric coordinates to geodetic coordinates
 * @param lat_geoc (in) Geocentric latitude, radians, + = North
 * @param radius (in) C.G. radius to earth center (meters)
 * @param lat_geod (out) Geodetic latitude, radians, + = North
 * @param alt (out) C.G. altitude above mean sea level (meters)
 * @param sea_level_r (out) radius from earth center to sea level at
 *        local vertical (surface normal) of C.G. (meters)
 */
inline void sgGeocToGeod(double lat_geoc, double radius,
                         double *lat_geod, double *alt, double *sea_level_r)
{
  SGVec3<double> cart;
  SGGeodesy::SGGeocToCart(SGGeoc::fromRadM(0, lat_geoc, radius), cart);
  SGGeod geod;
  SGGeodesy::SGCartToGeod(cart, geod);
  *lat_geod = geod.getLatitudeRad();
  *alt = geod.getElevationM();
  *sea_level_r = SGGeodesy::SGGeodToSeaLevelRadius(geod);
}

/**
 * Convert from geodetic coordinates to geocentric coordinates.
 * WARNING: this function is non-reversible.  Due to the fact that
 * "up" is a different direction for geocentric and geodetic frames,
 * you can not simply add your "alt" parameter to the "sl_radius"
 * result and get back (via sgGeodToGeoc()) to the coordinates you
 * started with.  The error under normal conditions will be of
 * centimeter order; whether that is important or not is application
 * dependent. Consider using sgGeodToCart() instead.
 *
 * @param lat_geod (in) Geodetic latitude, radians, + = North
 * @param alt (in) C.G. altitude above mean sea level (meters)
 * @param sl_radius (out) SEA LEVEL radius to earth center (meters)
 * @param lat_geoc (out) Geocentric latitude, radians, + = North
 */
inline void sgGeodToGeoc(double lat_geod, double alt,
                         double *sl_radius, double *lat_geoc)
{
  SGVec3<double> cart;
  SGGeod geod = SGGeod::fromRadM(0, lat_geod, alt);
  SGGeodesy::SGGeodToCart(geod, cart);
  SGGeoc geoc;
  SGGeodesy::SGCartToGeoc(cart, geoc);
  *lat_geoc = geoc.getLatitudeRad();
  *sl_radius = SGGeodesy::SGGeodToSeaLevelRadius(geod);
}


/**
 * Convert a cartesian point to a geodetic lat/lon/altitude.
 *
 * @param xyz (in) Pointer to cartesian point.
 * @param lat (out) Latitude, in radians
 * @param lon (out) Longitude, in radians
 * @param alt (out) Altitude, in meters above the WGS84 ellipsoid
 */
inline void sgCartToGeod(const double* xyz, double* lat, double* lon, double* alt)
{
  SGGeod geod;
  SGGeodesy::SGCartToGeod(SGVec3<double>(xyz), geod);
  *lat = geod.getLatitudeRad();
  *lon = geod.getLongitudeRad();
  *alt = geod.getElevationM();
}

/**
 * Convert a cartesian point to a geodetic lat/lon/altitude.
 * Alternate form using Point3D objects.
 *
 * @param cartesian point
 * @return geodetic point
 */
inline Point3D sgCartToGeod(const Point3D& p)
{
  SGGeod geod;
  SGGeodesy::SGCartToGeod(SGVec3<double>(p.x(), p.y(), p.z()), geod);
  return Point3D::fromSGGeod(geod);
}


/**
 * Convert a geodetic lat/lon/altitude to a cartesian point.
 *
 * @param lat (in) Latitude, in radians
 * @param lon (in) Longitude, in radians
 * @param alt (in) Altitude, in meters above the WGS84 ellipsoid
 * @param xyz (out) Pointer to cartesian point.
 */
inline void sgGeodToCart(double lat, double lon, double alt, double* xyz)
{
  SGVec3<double> cart;
  SGGeodesy::SGGeodToCart(SGGeod::fromRadM(lon, lat, alt), cart);
  xyz[0] = cart(0);
  xyz[1] = cart(1);
  xyz[2] = cart(2);
}

/**
 * Convert a geodetic lat/lon/altitude to a cartesian point.
 * Alternate form using Point3D objects.
 *
 * @param geodetic point
 * @return cartesian point
 */
inline Point3D sgGeodToCart(const Point3D& geod)
{
  SGVec3<double> cart;
  SGGeodesy::SGGeodToCart(SGGeod::fromRadM(geod.lon(), geod.lat(), geod.elev()), cart);
  return Point3D::fromSGVec3(cart);
}

/**
 * Given a starting position and an offset radial and distance,
 * calculate an ending positon on a wgs84 ellipsoid.
 * @param alt (in) meters (unused)
 * @param lat1 (in) degrees
 * @param lon1 (in) degrees
 * @param az1 (in) degrees
 * @param s (in) distance in meters
 * @param lat2 (out) degrees
 * @param lon2 (out) degrees
 * @param az2 (out) return course in degrees
 */
int geo_direct_wgs_84 ( double lat1, double lon1, double az1, 
			double s, double *lat2, double *lon2,
                        double *az2 );
inline int geo_direct_wgs_84 ( double alt, double lat1,
                        double lon1, double az1, 
			double s, double *lat2, double *lon2,
                        double *az2 )
{ return geo_direct_wgs_84(lat1, lon1, az1, s, lat2, lon2, az2); }

/**
 * Given a starting position and an offset radial and distance,
 * calculate an ending positon on a wgs84 ellipsoid.
 * @param p1 (in) geodetic position
 * @param az1 (in) degrees
 * @param s (in) distance in meters
 * @param p2 (out) geodetic position
 * @param az2 (out) return course in degrees
 */
inline int geo_direct_wgs_84(const SGGeod& p1, double az1,
                             double s, SGGeod& p2, double *az2 )
{
  double lat2, lon2;
  int ret = geo_direct_wgs_84(p1.getLatitudeDeg(), p1.getLongitudeDeg(),
                              az1, s, &lat2, &lon2, az2);
  p2.setLatitudeDeg(lat2);
  p2.setLongitudeDeg(lon2);
  return ret;
}

/**
 * Given an altitude and two sets of (lat, lon) calculate great circle
 * distance between them as well as the starting and ending azimuths.
 * @param alt (in) meters (unused)
 * @param lat1 (in) degrees
 * @param lon1 (in) degrees
 * @param lat2 (in) degrees
 * @param lon2 (in) degrees
 * @param az1 (out) start heading degrees
 * @param az2 (out) end heading degrees
 * @param s (out) distance meters
 */
int geo_inverse_wgs_84( double lat1, double lon1, double lat2,
			double lon2, double *az1, double *az2,
                        double *s );
inline int geo_inverse_wgs_84( double alt, double lat1,
                               double lon1, double lat2,
                               double lon2, double *az1, double *az2,
                               double *s )
{ return geo_inverse_wgs_84(lat1, lon1, lat2, lon2, az1, az2, s); }


/**
 * Given an altitude and two sets of (lat, lon) calculate great circle
 * distance between them as well as the starting and ending azimuths.
 * @param p1 (in) first position
 * @param p2 (in) fsecond position
 * @param az1 (out) start heading degrees
 * @param az2 (out) end heading degrees
 * @param s (out) distance meters
 */
inline int geo_inverse_wgs_84(const SGGeod& p1, const SGGeod& p2,
                              double *az1, double *az2, double *s )
{
  return geo_inverse_wgs_84(p1.getLatitudeDeg(), p1.getLongitudeDeg(),
                            p2.getLatitudeDeg(), p2.getLongitudeDeg(),
                            az1, az2, s);
}

#endif // _SG_GEODESY_HXX
