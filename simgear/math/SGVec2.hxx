// Copyright (C) 2006-2009  Mathias Froehlich - Mathias.Froehlich@web.de
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

#ifndef SGVec2_H
#define SGVec2_H

#include <iosfwd>

#include <simgear/math/SGLimits.hxx>
#include <simgear/math/SGMisc.hxx>
#include <simgear/math/SGMathFwd.hxx>
#include <simgear/math/simd.hxx>

/// 2D Vector Class
template<typename T>
class SGVec2 {
public:
  typedef T value_type;

  /// Default constructor. Does not initialize at all.
  /// If you need them zero initialized, use SGVec2::zeros()
  SGVec2(void)
  {
    /// Initialize with nans in the debug build, that will guarantee to have
    /// a fast uninitialized default constructor in the release but shows up
    /// uninitialized values in the debug build very fast ...
#ifndef NDEBUG
    for (unsigned i = 0; i < 2; ++i)
      data()[i] = SGLimits<T>::quiet_NaN();
#endif
  }
  /// Constructor. Initialize by the given values
  SGVec2(T x, T y)
  { _data = simd4_t<T,2>(x, y); }
  /// Constructor. Initialize by the content of a plain array,
  /// make sure it has at least 2 elements
  explicit SGVec2(const T* d)
  { _data = d ? simd4_t<T,2>(d) : simd4_t<T,2>(T(0)); }
  template<typename S>
  explicit SGVec2(const SGVec2<S>& d)
  { data()[0] = d[0]; data()[1] = d[1]; }

  /// Access by index, the index is unchecked
  const T& operator()(unsigned i) const
  { return data()[i]; }
  /// Access by index, the index is unchecked
  T& operator()(unsigned i)
  { return data()[i]; }

  /// Access raw data by index, the index is unchecked
  const T& operator[](unsigned i) const
  { return data()[i]; }
  /// Access raw data by index, the index is unchecked
  T& operator[](unsigned i)
  { return data()[i]; }

  /// Access the x component
  const T& x(void) const
  { return data()[0]; }
  /// Access the x component
  T& x(void)
  { return data()[0]; }
  /// Access the y component
  const T& y(void) const
  { return data()[1]; }
  /// Access the y component
  T& y(void)
  { return data()[1]; }

  /// Access raw data
  const T (&data(void) const)[2]
  { return _data.ptr(); }
  /// Access raw data
  T (&data(void))[2]
  { return _data.ptr(); }
  const simd4_t<T,2> &simd2(void) const
  { return _data; }
  /// Readonly raw storage interface
  simd4_t<T,2> &simd2(void)
  { return _data; }

  /// Inplace addition
  SGVec2& operator+=(const SGVec2& v)
  { _data += v.simd2(); return *this; }
  /// Inplace subtraction
  SGVec2& operator-=(const SGVec2& v)
  { _data -= v.simd2(); return *this; }
  /// Inplace scalar multiplication
  template<typename S>
  SGVec2& operator*=(S s)
  { _data *= s; return *this; }
  /// Inplace scalar multiplication by 1/s
  template<typename S>
  SGVec2& operator/=(S s)
  { _data*=(1/T(s)); return *this; }

  /// Return an all zero vector
  static SGVec2 zeros(void)
  { return SGVec2(0, 0); }
  /// Return unit vectors
  static SGVec2 e1(void)
  { return SGVec2(1, 0); }
  static SGVec2 e2(void)
  { return SGVec2(0, 1); }

private:
  simd4_t<T,2> _data;
};

/// Unary +, do nothing ...
template<typename T>
inline
const SGVec2<T>&
operator+(const SGVec2<T>& v)
{ return v; }

/// Unary -, do nearly nothing
template<typename T>
inline
SGVec2<T>
operator-(SGVec2<T> v)
{ v *= -1; return v; }

/// Binary +
template<typename T>
inline
SGVec2<T>
operator+(SGVec2<T> v1, const SGVec2<T>& v2)
{ v1.simd2() += v2.simd2(); return v1; }

/// Binary -
template<typename T>
inline
SGVec2<T>
operator-(SGVec2<T> v1, const SGVec2<T>& v2)
{ v1.simd2() -= v2.simd2(); return v1; }

/// Scalar multiplication
template<typename S, typename T>
inline
SGVec2<T>
operator*(S s, SGVec2<T> v)
{ v.simd2() *= s; return v; }

/// Scalar multiplication
template<typename S, typename T>
inline
SGVec2<T>
operator*(SGVec2<T> v, S s)
{ v.simd2() *= s; return v; }

/// multiplication as a multiplicator, that is assume that the first vector
/// represents a 2x2 diagonal matrix with the diagonal elements in the vector.
/// Then the result is the product of that matrix times the second vector.
template<typename T>
inline
SGVec2<T>
mult(SGVec2<T> v1, const SGVec2<T>& v2)
{ v1.simd2() *= v2.simd2(); return v1; }

/// component wise min
template<typename T>
inline
SGVec2<T>
min(SGVec2<T> v1, const SGVec2<T>& v2)
{ v1.simd2() = simd4::min(v1.simd2(), v2.simd2()); return v1; }
template<typename S, typename T>
inline
SGVec2<T>
min(SGVec2<T> v, S s)
{ v.simd2() = simd4::min(v.simd2(), simd4_t<T,2>(s)); return v; }
template<typename S, typename T>
inline
SGVec2<T>
min(S s, SGVec2<T> v)
{ v.sim2() = simd4::min(v.simd2(), simd4_t<T,2>(s)); return v; }

/// component wise max
template<typename T>
inline
SGVec2<T>
max(SGVec2<T> v1, const SGVec2<T>& v2)
{ v1.simd2() = simd4::max(v1.simd2(), v2.simd2()); return v1; }
template<typename S, typename T>
inline
SGVec2<T>
max(const SGVec2<T>& v, S s)
{ v = simd4::max(v.simd2(), simd4_t<T,2>(s)); return v; }
template<typename S, typename T>
inline
SGVec2<T>
max(S s, const SGVec2<T>& v)
{ v = simd4::max(v.simd2(), simd4_t<T,2>(s)); return v; }

/// Add two vectors taking care of (integer) overflows. The values are limited
/// to the respective minimum and maximum values.
template<class T>
SGVec2<T> addClipOverflow(SGVec2<T> const& lhs, SGVec2<T> const& rhs)
{
  return SGVec2<T>(
    SGMisc<T>::addClipOverflow(lhs.x(), rhs.x()),
    SGMisc<T>::addClipOverflow(lhs.y(), rhs.y())
  );
}

/// Scalar dot product
template<typename T>
inline
T
dot(const SGVec2<T>& v1, const SGVec2<T>& v2)
{ return simd4::dot(v1.simd2(), v2.simd2()); }

/// The euclidean norm of the vector, that is what most people call length
template<typename T>
inline
T
norm(const SGVec2<T>& v)
{ return simd4::magnitude(v.simd2()); }

/// The euclidean norm of the vector, that is what most people call length
template<typename T>
inline
T
length(const SGVec2<T>& v)
{ return simd4::magnitude(v.simd2()); }

/// The 1-norm of the vector, this one is the fastest length function we
/// can implement on modern cpu's
template<typename T>
inline
T
norm1(SGVec2<T> v)
{ v.simd2() = simd4::abs(v.simd2()); return (v(0)+v(1)); }

/// The inf-norm of the vector
template<typename T>
inline
T
normI(SGVec2<T> v)
{
  v.simd2() = simd4::abs(v.simd2());
  return SGMisc<T>::max(v(0), v(1));
}

/// The euclidean norm of the vector, that is what most people call length
template<typename T>
inline
SGVec2<T>
normalize(const SGVec2<T>& v)
{
  T normv = norm(v);
  if (normv <= SGLimits<T>::min())
    return SGVec2<T>::zeros();
  return (1/normv)*v;
}

/// Return true if exactly the same
template<typename T>
inline
bool
operator==(const SGVec2<T>& v1, const SGVec2<T>& v2)
{ return v1(0) == v2(0) && v1(1) == v2(1); }

/// Return true if not exactly the same
template<typename T>
inline
bool
operator!=(const SGVec2<T>& v1, const SGVec2<T>& v2)
{ return ! (v1 == v2); }

/// Return true if smaller, good for putting that into a std::map
template<typename T>
inline
bool
operator<(const SGVec2<T>& v1, const SGVec2<T>& v2)
{
  if (v1(0) < v2(0)) return true;
  else if (v2(0) < v1(0)) return false;
  else return (v1(1) < v2(1));
}

template<typename T>
inline
bool
operator<=(const SGVec2<T>& v1, const SGVec2<T>& v2)
{
  if (v1(0) < v2(0)) return true;
  else if (v2(0) < v1(0)) return false;
  else return (v1(1) <= v2(1));
}

template<typename T>
inline
bool
operator>(const SGVec2<T>& v1, const SGVec2<T>& v2)
{ return operator<(v2, v1); }

template<typename T>
inline
bool
operator>=(const SGVec2<T>& v1, const SGVec2<T>& v2)
{ return operator<=(v2, v1); }

/// Return true if equal to the relative tolerance tol
template<typename T>
inline
bool
equivalent(const SGVec2<T>& v1, const SGVec2<T>& v2, T rtol, T atol)
{ return norm1(v1 - v2) < rtol*(norm1(v1) + norm1(v2)) + atol; }

/// Return true if equal to the relative tolerance tol
template<typename T>
inline
bool
equivalent(const SGVec2<T>& v1, const SGVec2<T>& v2, T rtol)
{ return norm1(v1 - v2) < rtol*(norm1(v1) + norm1(v2)); }

/// Return true if about equal to roundoff of the underlying type
template<typename T>
inline
bool
equivalent(const SGVec2<T>& v1, const SGVec2<T>& v2)
{
  T tol = 100*SGLimits<T>::epsilon();
  return equivalent(v1, v2, tol, tol);
}

/// The euclidean distance of the two vectors
template<typename T>
inline
T
dist(const SGVec2<T>& v1, const SGVec2<T>& v2)
{ return simd4::magnitude(v1.simd2() - v2.simd2()); }

/// The squared euclidean distance of the two vectors
template<typename T>
inline
T
distSqr(SGVec2<T> v1, const SGVec2<T>& v2)
{ return simd4::magnitude2(v1.simd2() - v2.simd2()); }

// calculate the projection of u along the direction of d.
template<typename T>
inline
SGVec2<T>
projection(const SGVec2<T>& u, const SGVec2<T>& d)
{
  T denom = simd4::magnitude2(d.simd2());
  T ud = dot(u, d);
  if (SGLimits<T>::min() < denom) return u;
  else return d * (dot(u, d) / denom);
}

template<typename T>
inline
SGVec2<T>
interpolate(T tau, const SGVec2<T>& v1, const SGVec2<T>& v2)
{
  SGVec2<T> r;
  r.simd2() = simd4::interpolate(tau, v1.simd2(), v2.simd2());
  return r;
}

// Helper function for point_in_triangle
template <typename T>
inline
T
pt_determine(const SGVec2<T>& pt1, const SGVec2<T>& pt2, const SGVec2<T>& pt3)
{
  return (pt1.x()-pt3.x()) * (pt2.y()-pt3.y()) - (pt2.x() - pt3.x()) * (pt1.y() - pt3.y());
}

// Is testpt inside the triangle formed by the other three points?
template <typename T>
inline
bool
point_in_triangle(const SGVec2<T>& testpt, const SGVec2<T>& pt1, const SGVec2<T>& pt2, const SGVec2<T>& pt3)
{
  T d1 = pt_determine(testpt,pt1,pt2);
  T d2 = pt_determine(testpt,pt2,pt3);
  T d3 = pt_determine(testpt,pt3,pt1);
  bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
  bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
  return !(has_neg && has_pos);
}

#ifndef NDEBUG
template<typename T>
inline
bool
isNaN(const SGVec2<T>& v)
{
  return SGMisc<T>::isNaN(v(0)) || SGMisc<T>::isNaN(v(1));
}
#endif

/// Output to an ostream
template<typename char_type, typename traits_type, typename T>
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& s, const SGVec2<T>& v)
{ return s << "[ " << v(0) << ", " << v(1) << " ]"; }

inline
SGVec2f
toVec2f(const SGVec2d& v)
{ SGVec2f f(v); return f; }

inline
SGVec2d
toVec2d(const SGVec2f& v)
{ SGVec2d d(v); return d; }

#endif
