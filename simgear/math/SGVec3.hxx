// Copyright (C) 2006  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifndef SGVec3_H
#define SGVec3_H

/// 3D Vector Class
template<typename T>
class SGVec3 {
public:
  typedef T value_type;

  /// Default constructor. Does not initialize at all.
  /// If you need them zero initialized, use SGVec3::zeros()
  SGVec3(void)
  {
    /// Initialize with nans in the debug build, that will guarantee to have
    /// a fast uninitialized default constructor in the release but shows up
    /// uninitialized values in the debug build very fast ...
#ifndef NDEBUG
    for (unsigned i = 0; i < 3; ++i)
      _data[i] = SGLimits<T>::quiet_NaN();
#endif
  }
  /// Constructor. Initialize by the given values
  SGVec3(T x, T y, T z)
  { _data[0] = x; _data[1] = y; _data[2] = z; }
  /// Constructor. Initialize by the content of a plain array,
  /// make sure it has at least 3 elements
  explicit SGVec3(const T* data)
  { _data[0] = data[0]; _data[1] = data[1]; _data[2] = data[2]; }

  /// Access by index, the index is unchecked
  const T& operator()(unsigned i) const
  { return _data[i]; }
  /// Access by index, the index is unchecked
  T& operator()(unsigned i)
  { return _data[i]; }

  /// Access raw data by index, the index is unchecked
  const T& operator[](unsigned i) const
  { return _data[i]; }
  /// Access raw data by index, the index is unchecked
  T& operator[](unsigned i)
  { return _data[i]; }

  /// Access the x component
  const T& x(void) const
  { return _data[0]; }
  /// Access the x component
  T& x(void)
  { return _data[0]; }
  /// Access the y component
  const T& y(void) const
  { return _data[1]; }
  /// Access the y component
  T& y(void)
  { return _data[1]; }
  /// Access the z component
  const T& z(void) const
  { return _data[2]; }
  /// Access the z component
  T& z(void)
  { return _data[2]; }

  /// Get the data pointer
  const T* data(void) const
  { return _data; }
  /// Get the data pointer
  T* data(void)
  { return _data; }

  /// Readonly interface function to ssg's sgVec3/sgdVec3
  const T (&sg(void) const)[3]
  { return _data; }
  /// Interface function to ssg's sgVec3/sgdVec3
  T (&sg(void))[3]
  { return _data; }

  /// Inplace addition
  SGVec3& operator+=(const SGVec3& v)
  { _data[0] += v(0); _data[1] += v(1); _data[2] += v(2); return *this; }
  /// Inplace subtraction
  SGVec3& operator-=(const SGVec3& v)
  { _data[0] -= v(0); _data[1] -= v(1); _data[2] -= v(2); return *this; }
  /// Inplace scalar multiplication
  template<typename S>
  SGVec3& operator*=(S s)
  { _data[0] *= s; _data[1] *= s; _data[2] *= s; return *this; }
  /// Inplace scalar multiplication by 1/s
  template<typename S>
  SGVec3& operator/=(S s)
  { return operator*=(1/T(s)); }

  /// Return an all zero vector
  static SGVec3 zeros(void)
  { return SGVec3(0, 0, 0); }
  /// Return unit vectors
  static SGVec3 e1(void)
  { return SGVec3(1, 0, 0); }
  static SGVec3 e2(void)
  { return SGVec3(0, 1, 0); }
  static SGVec3 e3(void)
  { return SGVec3(0, 0, 1); }

  /// Constructor. Initialize by a geodetic coordinate
  /// Note that this conversion is relatively expensive to compute
  static SGVec3 fromGeod(const SGGeod& geod);
  /// Constructor. Initialize by a geocentric coordinate
  /// Note that this conversion is relatively expensive to compute
  static SGVec3 fromGeoc(const SGGeoc& geoc);

private:
  /// The actual data
  T _data[3];
};

template<>
inline
SGVec3<double>
SGVec3<double>::fromGeod(const SGGeod& geod)
{
  SGVec3<double> cart;
  SGGeodesy::SGGeodToCart(geod, cart);
  return cart;
}

template<>
inline
SGVec3<float>
SGVec3<float>::fromGeod(const SGGeod& geod)
{
  SGVec3<double> cart;
  SGGeodesy::SGGeodToCart(geod, cart);
  return SGVec3<float>(cart(0), cart(1), cart(2));
}

template<>
inline
SGVec3<double>
SGVec3<double>::fromGeoc(const SGGeoc& geoc)
{
  SGVec3<double> cart;
  SGGeodesy::SGGeocToCart(geoc, cart);
  return cart;
}

template<>
inline
SGVec3<float>
SGVec3<float>::fromGeoc(const SGGeoc& geoc)
{
  SGVec3<double> cart;
  SGGeodesy::SGGeocToCart(geoc, cart);
  return SGVec3<float>(cart(0), cart(1), cart(2));
}

/// Unary +, do nothing ...
template<typename T>
inline
const SGVec3<T>&
operator+(const SGVec3<T>& v)
{ return v; }

/// Unary -, do nearly nothing
template<typename T>
inline
SGVec3<T>
operator-(const SGVec3<T>& v)
{ return SGVec3<T>(-v(0), -v(1), -v(2)); }

/// Binary +
template<typename T>
inline
SGVec3<T>
operator+(const SGVec3<T>& v1, const SGVec3<T>& v2)
{ return SGVec3<T>(v1(0)+v2(0), v1(1)+v2(1), v1(2)+v2(2)); }

/// Binary -
template<typename T>
inline
SGVec3<T>
operator-(const SGVec3<T>& v1, const SGVec3<T>& v2)
{ return SGVec3<T>(v1(0)-v2(0), v1(1)-v2(1), v1(2)-v2(2)); }

/// Scalar multiplication
template<typename S, typename T>
inline
SGVec3<T>
operator*(S s, const SGVec3<T>& v)
{ return SGVec3<T>(s*v(0), s*v(1), s*v(2)); }

/// Scalar multiplication
template<typename S, typename T>
inline
SGVec3<T>
operator*(const SGVec3<T>& v, S s)
{ return SGVec3<T>(s*v(0), s*v(1), s*v(2)); }

/// Scalar dot product
template<typename T>
inline
T
dot(const SGVec3<T>& v1, const SGVec3<T>& v2)
{ return v1(0)*v2(0) + v1(1)*v2(1) + v1(2)*v2(2); }

/// The euclidean norm of the vector, that is what most people call length
template<typename T>
inline
T
norm(const SGVec3<T>& v)
{ return sqrt(dot(v, v)); }

/// The euclidean norm of the vector, that is what most people call length
template<typename T>
inline
T
length(const SGVec3<T>& v)
{ return sqrt(dot(v, v)); }

/// The 1-norm of the vector, this one is the fastest length function we
/// can implement on modern cpu's
template<typename T>
inline
T
norm1(const SGVec3<T>& v)
{ return fabs(v(0)) + fabs(v(1)) + fabs(v(2)); }

/// Vector cross product
template<typename T>
inline
SGVec3<T>
cross(const SGVec3<T>& v1, const SGVec3<T>& v2)
{
  return SGVec3<T>(v1(1)*v2(2) - v1(2)*v2(1),
                   v1(2)*v2(0) - v1(0)*v2(2),
                   v1(0)*v2(1) - v1(1)*v2(0));
}

/// The euclidean norm of the vector, that is what most people call length
template<typename T>
inline
SGVec3<T>
normalize(const SGVec3<T>& v)
{ return (1/norm(v))*v; }

/// Return true if exactly the same
template<typename T>
inline
bool
operator==(const SGVec3<T>& v1, const SGVec3<T>& v2)
{ return v1(0) == v2(0) && v1(1) == v2(1) && v1(2) == v2(2); }

/// Return true if not exactly the same
template<typename T>
inline
bool
operator!=(const SGVec3<T>& v1, const SGVec3<T>& v2)
{ return ! (v1 == v2); }

/// Return true if equal to the relative tolerance tol
template<typename T>
inline
bool
equivalent(const SGVec3<T>& v1, const SGVec3<T>& v2, T rtol, T atol)
{ return norm1(v1 - v2) < rtol*(norm1(v1) + norm1(v2)) + atol; }

/// Return true if equal to the relative tolerance tol
template<typename T>
inline
bool
equivalent(const SGVec3<T>& v1, const SGVec3<T>& v2, T rtol)
{ return norm1(v1 - v2) < rtol*(norm1(v1) + norm1(v2)); }

/// Return true if about equal to roundoff of the underlying type
template<typename T>
inline
bool
equivalent(const SGVec3<T>& v1, const SGVec3<T>& v2)
{
  T tol = 100*SGLimits<T>::epsilon();
  return equivalent(v1, v2, tol, tol);
}

/// The euclidean distance of the two vectors
template<typename T>
inline
T
dist(const SGVec3<T>& v1, const SGVec3<T>& v2)
{ return norm(v1 - v2); }

/// The squared euclidean distance of the two vectors
template<typename T>
inline
T
distSqr(const SGVec3<T>& v1, const SGVec3<T>& v2)
{ SGVec3<T> tmp = v1 - v2; return dot(tmp, tmp); }

#ifndef NDEBUG
template<typename T>
inline
bool
isNaN(const SGVec3<T>& v)
{
  return SGMisc<T>::isNaN(v(0)) ||
    SGMisc<T>::isNaN(v(1)) || SGMisc<T>::isNaN(v(2));
}
#endif

/// Output to an ostream
template<typename char_type, typename traits_type, typename T>
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& s, const SGVec3<T>& v)
{ return s << "[ " << v(0) << ", " << v(1) << ", " << v(2) << " ]"; }

inline
SGVec3f
toVec3f(const SGVec3d& v)
{ return SGVec3f((float)v(0), (float)v(1), (float)v(2)); }

inline
SGVec3d
toVec3d(const SGVec3f& v)
{ return SGVec3d(v(0), v(1), v(2)); }

#endif
