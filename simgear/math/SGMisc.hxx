#ifndef SGMisc_H
#define SGMisc_H

#include <cmath>

template<typename T>
class SGMisc {
public:
  static T pi() { return T(3.1415926535897932384626433832795029L); }
  static T min(const T& a, const T& b)
  { return a < b ? a : b; }
  static T min(const T& a, const T& b, const T& c)
  { return min(min(a, b), c); }
  static T min(const T& a, const T& b, const T& c, const T& d)
  { return min(min(min(a, b), c), d); }
  static T max(const T& a, const T& b)
  { return a > b ? a : b; }
  static T max(const T& a, const T& b, const T& c)
  { return max(max(a, b), c); }
  static T max(const T& a, const T& b, const T& c, const T& d)
  { return max(max(max(a, b), c), d); }
  static int sign(const T& a)
  {
    if (a < -SGLimits<T>::min())
      return -1;
    else if (SGLimits<T>::min() < a)
      return 1;
    else
      return 0;
  }

  static T rad2deg(const T& val)
  { return val*180/pi(); }
  static T deg2rad(const T& val)
  { return val*pi()/180; }

#ifndef NDEBUG
  /// Returns true if v is a NaN value
  /// Use with care: allways code that you do not need to use that!
  static bool isNaN(const T& v)
  {
#ifdef HAVE_ISNAN
    return isnan(v);
#elif defined HAVE_STD_ISNAN
    return std::isnan(v);
#else
    // Use that every compare involving a NaN returns false
    // But be careful, some usual compiler switches like for example
    // -fast-math from gcc might optimize that expression to v != v which
    // behaves exactly like the opposite ...
    return !(v == v);
#endif
  }
#endif
};

typedef SGMisc<float> SGMiscf;
typedef SGMisc<double> SGMiscd;

#endif
