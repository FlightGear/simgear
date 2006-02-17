#ifndef SGGeodesy_H
#define SGGeodesy_H

class SGGeoc;
class SGGeod;

template<typename T>
class SGVec3;

class SGGeodesy {
public:
  // Hard numbers from the WGS84 standard.
  static const double EQURAD;
  static const double iFLATTENING;
  static const double SQUASH;
  static const double STRETCH;
  static const double POLRAD;

  /// Takes a cartesian coordinate data and returns the geodetic
  /// coordinates.
  static void SGCartToGeod(const SGVec3<double>& cart, SGGeod& geod);
  
  /// Takes a geodetic coordinate data and returns the cartesian
  /// coordinates.
  static void SGGeodToCart(const SGGeod& geod, SGVec3<double>& cart);
  
  /// Takes a geodetic coordinate data and returns the sea level radius.
  static double SGGeodToSeaLevelRadius(const SGGeod& geod);

  /// Takes a cartesian coordinate data and returns the geocentric
  /// coordinates.
  static void SGCartToGeoc(const SGVec3<double>& cart, SGGeoc& geoc);
  
  /// Takes a geocentric coordinate data and returns the cartesian
  /// coordinates.
  static void SGGeocToCart(const SGGeoc& geoc, SGVec3<double>& cart);
};

#endif
