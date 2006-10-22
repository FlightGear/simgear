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

#ifndef SGQuat_H
#define SGQuat_H

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif


/// 3D Vector Class
template<typename T>
class SGQuat {
public:
  typedef T value_type;

  /// Default constructor. Does not initialize at all.
  /// If you need them zero initialized, SGQuat::zeros()
  SGQuat(void)
  {
    /// Initialize with nans in the debug build, that will guarantee to have
    /// a fast uninitialized default constructor in the release but shows up
    /// uninitialized values in the debug build very fast ...
#ifndef NDEBUG
    for (unsigned i = 0; i < 4; ++i)
      _data[i] = SGLimits<T>::quiet_NaN();
#endif
  }
  /// Constructor. Initialize by the given values
  SGQuat(T _x, T _y, T _z, T _w)
  { x() = _x; y() = _y; z() = _z; w() = _w; }
  /// Constructor. Initialize by the content of a plain array,
  /// make sure it has at least 4 elements
  explicit SGQuat(const T* d)
  { _data[0] = d[0]; _data[1] = d[1]; _data[2] = d[2]; _data[3] = d[3]; }

  /// Return a unit quaternion
  static SGQuat unit(void)
  { return fromRealImag(1, SGVec3<T>(0)); }

  /// Return a quaternion from euler angles
  static SGQuat fromEulerRad(T z, T y, T x)
  {
    SGQuat q;
    T zd2 = T(0.5)*z; T yd2 = T(0.5)*y; T xd2 = T(0.5)*x;
    T Szd2 = sin(zd2); T Syd2 = sin(yd2); T Sxd2 = sin(xd2);
    T Czd2 = cos(zd2); T Cyd2 = cos(yd2); T Cxd2 = cos(xd2);
    T Cxd2Czd2 = Cxd2*Czd2; T Cxd2Szd2 = Cxd2*Szd2;
    T Sxd2Szd2 = Sxd2*Szd2; T Sxd2Czd2 = Sxd2*Czd2;
    q.w() = Cxd2Czd2*Cyd2 + Sxd2Szd2*Syd2;
    q.x() = Sxd2Czd2*Cyd2 - Cxd2Szd2*Syd2;
    q.y() = Cxd2Czd2*Syd2 + Sxd2Szd2*Cyd2;
    q.z() = Cxd2Szd2*Cyd2 - Sxd2Czd2*Syd2;
    return q;
  }

  /// Return a quaternion from euler angles in degrees
  static SGQuat fromEulerDeg(T z, T y, T x)
  {
    return fromEulerRad(SGMisc<T>::deg2rad(z), SGMisc<T>::deg2rad(y),
                        SGMisc<T>::deg2rad(x));
  }

  /// Return a quaternion from euler angles
  static SGQuat fromYawPitchRoll(T y, T p, T r)
  { return fromEulerRad(y, p, r); }

  /// Return a quaternion from euler angles
  static SGQuat fromYawPitchRollDeg(T y, T p, T r)
  { return fromEulerDeg(y, p, r); }

  /// Return a quaternion from euler angles
  static SGQuat fromHeadAttBank(T h, T a, T b)
  { return fromEulerRad(h, a, b); }

  /// Return a quaternion from euler angles
  static SGQuat fromHeadAttBankDeg(T h, T a, T b)
  { return fromEulerDeg(h, a, b); }

  /// Return a quaternion rotation the the horizontal local frame from given
  /// longitude and latitude
  static SGQuat fromLonLatRad(T lon, T lat)
  {
    SGQuat q;
    T zd2 = T(0.5)*lon;
    T yd2 = T(-0.25)*SGMisc<value_type>::pi() - T(0.5)*lat;
    T Szd2 = sin(zd2);
    T Syd2 = sin(yd2);
    T Czd2 = cos(zd2);
    T Cyd2 = cos(yd2);
    q.w() = Czd2*Cyd2;
    q.x() = -Szd2*Syd2;
    q.y() = Czd2*Syd2;
    q.z() = Szd2*Cyd2;
    return q;
  }

  /// Return a quaternion rotation the the horizontal local frame from given
  /// longitude and latitude
  static SGQuat fromLonLatDeg(T lon, T lat)
  { return fromLonLatRad(SGMisc<T>::deg2rad(lon), SGMisc<T>::deg2rad(lat)); }

  /// Return a quaternion rotation the the horizontal local frame from given
  /// longitude and latitude
  static SGQuat fromLonLat(const SGGeod& geod)
  { return fromLonLatRad(geod.getLongitudeRad(), geod.getLatitudeRad()); }

  /// Create a quaternion from the angle axis representation
  static SGQuat fromAngleAxis(T angle, const SGVec3<T>& axis)
  {
    T angle2 = 0.5*angle;
    return fromRealImag(cos(angle2), T(sin(angle2))*axis);
  }

  /// Create a quaternion from the angle axis representation
  static SGQuat fromAngleAxisDeg(T angle, const SGVec3<T>& axis)
  { return fromAngleAxis(SGMisc<T>::deg2rad(angle), axis); }

  /// Create a quaternion from the angle axis representation where the angle
  /// is stored in the axis' length
  static SGQuat fromAngleAxis(const SGVec3<T>& axis)
  {
    T nAxis = norm(axis);
    if (nAxis <= SGLimits<T>::min())
      return SGQuat(1, 0, 0, 0);
    T angle2 = 0.5*nAxis;
    return fromRealImag(cos(angle2), T(sin(angle2)/nAxis)*axis);
  }

  /// Return a quaternion from real and imaginary part
  static SGQuat fromRealImag(T r, const SGVec3<T>& i)
  {
    SGQuat q;
    q.w() = r;
    q.x() = i(0);
    q.y() = i(1);
    q.z() = i(2);
    return q;
  }

  /// Return an all zero vector
  static SGQuat zeros(void)
  { return SGQuat(0, 0, 0, 0); }

  /// write the euler angles into the references
  void getEulerRad(T& zRad, T& yRad, T& xRad) const
  {
    value_type sqrQW = w()*w();
    value_type sqrQX = x()*x();
    value_type sqrQY = y()*y();
    value_type sqrQZ = z()*z();

    value_type num = 2*(y()*z() + w()*x());
    value_type den = sqrQW - sqrQX - sqrQY + sqrQZ;
    if (fabs(den) < SGLimits<value_type>::min() &&
        fabs(num) < SGLimits<value_type>::min())
      xRad = 0;
    else
      xRad = atan2(num, den);
    
    value_type tmp = 2*(x()*z() - w()*y());
    if (tmp < -1)
      yRad = 0.5*SGMisc<value_type>::pi();
    else if (1 < tmp)
      yRad = -0.5*SGMisc<value_type>::pi();
    else
      yRad = -asin(tmp);
   
    num = 2*(x()*y() + w()*z()); 
    den = sqrQW + sqrQX - sqrQY - sqrQZ;
    if (fabs(den) < SGLimits<value_type>::min() &&
        fabs(num) < SGLimits<value_type>::min())
      zRad = 0;
    else {
      value_type psi = atan2(num, den);
      if (psi < 0)
        psi += 2*SGMisc<value_type>::pi();
      zRad = psi;
    }
  }

  /// write the euler angles in degrees into the references
  void getEulerDeg(T& zDeg, T& yDeg, T& xDeg) const
  {
    getEulerRad(zDeg, yDeg, xDeg);
    zDeg = SGMisc<T>::rad2deg(zDeg);
    yDeg = SGMisc<T>::rad2deg(yDeg);
    xDeg = SGMisc<T>::rad2deg(xDeg);
  }

  /// write the angle axis representation into the references
  void getAngleAxis(T& angle, SGVec3<T>& axis) const
  {
    T nrm = norm(*this);
    if (nrm < SGLimits<T>::min()) {
      angle = 0;
      axis = SGVec3<T>(0, 0, 0);
    } else {
      T rNrm = 1/nrm;
      angle = acos(SGMisc<T>::max(-1, SGMisc<T>::min(1, rNrm*w())));
      T sAng = sin(angle);
      if (fabs(sAng) < SGLimits<T>::min())
        axis = SGVec3<T>(1, 0, 0);
      else 
        axis = (rNrm/sAng)*imag(*this);
      angle *= 2;
    }
  }

  /// write the angle axis representation into the references
  void getAngleAxis(SGVec3<T>& axis) const
  {
    T angle;
    getAngleAxis(angle, axis);
    axis *= angle;
  }

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
  /// Access the w component
  const T& w(void) const
  { return _data[3]; }
  /// Access the w component
  T& w(void)
  { return _data[3]; }

  /// Get the data pointer, usefull for interfacing with plib's sg*Vec
  const T* data(void) const
  { return _data; }
  /// Get the data pointer, usefull for interfacing with plib's sg*Vec
  T* data(void)
  { return _data; }

  /// Readonly interface function to ssg's sgQuat/sgdQuat
  const T (&sg(void) const)[4]
  { return _data; }
  /// Interface function to ssg's sgQuat/sgdQuat
  T (&sg(void))[4]
  { return _data; }

  /// Inplace addition
  SGQuat& operator+=(const SGQuat& v)
  { _data[0]+=v(0);_data[1]+=v(1);_data[2]+=v(2);_data[3]+=v(3);return *this; }
  /// Inplace subtraction
  SGQuat& operator-=(const SGQuat& v)
  { _data[0]-=v(0);_data[1]-=v(1);_data[2]-=v(2);_data[3]-=v(3);return *this; }
  /// Inplace scalar multiplication
  template<typename S>
  SGQuat& operator*=(S s)
  { _data[0] *= s; _data[1] *= s; _data[2] *= s; _data[3] *= s; return *this; }
  /// Inplace scalar multiplication by 1/s
  template<typename S>
  SGQuat& operator/=(S s)
  { return operator*=(1/T(s)); }
  /// Inplace quaternion multiplication
  SGQuat& operator*=(const SGQuat& v);

  /// Transform a vector from the current coordinate frame to a coordinate
  /// frame rotated with the quaternion
  SGVec3<T> transform(const SGVec3<T>& v) const
  {
    value_type r = 2/dot(*this, *this);
    SGVec3<T> qimag = imag(*this);
    value_type qr = real(*this);
    return (r*qr*qr - 1)*v + (r*dot(qimag, v))*qimag - (r*qr)*cross(qimag, v);
  }
  /// Transform a vector from the coordinate frame rotated with the quaternion
  /// to the current coordinate frame
  SGVec3<T> backTransform(const SGVec3<T>& v) const
  {
    value_type r = 2/dot(*this, *this);
    SGVec3<T> qimag = imag(*this);
    value_type qr = real(*this);
    return (r*qr*qr - 1)*v + (r*dot(qimag, v))*qimag + (r*qr)*cross(qimag, v);
  }

  /// Rotate a given vector with the quaternion
  SGVec3<T> rotate(const SGVec3<T>& v) const
  { return backTransform(v); }
  /// Rotate a given vector with the inverse quaternion
  SGVec3<T> rotateBack(const SGVec3<T>& v) const
  { return transform(v); }

  /// Return the time derivative of the quaternion given the angular velocity
  SGQuat
  derivative(const SGVec3<T>& angVel)
  {
    SGQuat deriv;

    deriv.w() = 0.5*(-x()*angVel(0) - y()*angVel(1) - z()*angVel(2));
    deriv.x() = 0.5*( w()*angVel(0) - z()*angVel(1) + y()*angVel(2));
    deriv.y() = 0.5*( z()*angVel(0) + w()*angVel(1) - x()*angVel(2));
    deriv.z() = 0.5*(-y()*angVel(0) + x()*angVel(1) + w()*angVel(2));
    
    return deriv;
  }

private:
  /// The actual data
  T _data[4];
};

/// Unary +, do nothing ...
template<typename T>
inline
const SGQuat<T>&
operator+(const SGQuat<T>& v)
{ return v; }

/// Unary -, do nearly nothing
template<typename T>
inline
SGQuat<T>
operator-(const SGQuat<T>& v)
{ return SGQuat<T>(-v(0), -v(1), -v(2), -v(3)); }

/// Binary +
template<typename T>
inline
SGQuat<T>
operator+(const SGQuat<T>& v1, const SGQuat<T>& v2)
{ return SGQuat<T>(v1(0)+v2(0), v1(1)+v2(1), v1(2)+v2(2), v1(3)+v2(3)); }

/// Binary -
template<typename T>
inline
SGQuat<T>
operator-(const SGQuat<T>& v1, const SGQuat<T>& v2)
{ return SGQuat<T>(v1(0)-v2(0), v1(1)-v2(1), v1(2)-v2(2), v1(3)-v2(3)); }

/// Scalar multiplication
template<typename S, typename T>
inline
SGQuat<T>
operator*(S s, const SGQuat<T>& v)
{ return SGQuat<T>(s*v(0), s*v(1), s*v(2), s*v(3)); }

/// Scalar multiplication
template<typename S, typename T>
inline
SGQuat<T>
operator*(const SGQuat<T>& v, S s)
{ return SGQuat<T>(s*v(0), s*v(1), s*v(2), s*v(3)); }

/// Quaterion multiplication
template<typename T>
inline
SGQuat<T>
operator*(const SGQuat<T>& v1, const SGQuat<T>& v2)
{
  SGQuat<T> v;
  v.x() = v1.w()*v2.x() + v1.x()*v2.w() + v1.y()*v2.z() - v1.z()*v2.y();
  v.y() = v1.w()*v2.y() - v1.x()*v2.z() + v1.y()*v2.w() + v1.z()*v2.x();
  v.z() = v1.w()*v2.z() + v1.x()*v2.y() - v1.y()*v2.x() + v1.z()*v2.w();
  v.w() = v1.w()*v2.w() - v1.x()*v2.x() - v1.y()*v2.y() - v1.z()*v2.z();
  return v;
}

/// Now define the inplace multiplication
template<typename T>
inline
SGQuat<T>&
SGQuat<T>::operator*=(const SGQuat& v)
{ (*this) = (*this)*v; return *this; }

/// The conjugate of the quaternion, this is also the
/// inverse for normalized quaternions
template<typename T>
inline
SGQuat<T>
conj(const SGQuat<T>& v)
{ return SGQuat<T>(-v(0), -v(1), -v(2), v(3)); }

/// The conjugate of the quaternion, this is also the
/// inverse for normalized quaternions
template<typename T>
inline
SGQuat<T>
inverse(const SGQuat<T>& v)
{ return (1/dot(v, v))*SGQuat<T>(-v(0), -v(1), -v(2), v(3)); }

/// The imagniary part of the quaternion
template<typename T>
inline
T
real(const SGQuat<T>& v)
{ return v.w(); }

/// The imagniary part of the quaternion
template<typename T>
inline
SGVec3<T>
imag(const SGQuat<T>& v)
{ return SGVec3<T>(v.x(), v.y(), v.z()); }

/// Scalar dot product
template<typename T>
inline
T
dot(const SGQuat<T>& v1, const SGQuat<T>& v2)
{ return v1(0)*v2(0) + v1(1)*v2(1) + v1(2)*v2(2) + v1(3)*v2(3); }

/// The euclidean norm of the vector, that is what most people call length
template<typename T>
inline
T
norm(const SGQuat<T>& v)
{ return sqrt(dot(v, v)); }

/// The euclidean norm of the vector, that is what most people call length
template<typename T>
inline
T
length(const SGQuat<T>& v)
{ return sqrt(dot(v, v)); }

/// The 1-norm of the vector, this one is the fastest length function we
/// can implement on modern cpu's
template<typename T>
inline
T
norm1(const SGQuat<T>& v)
{ return fabs(v(0)) + fabs(v(1)) + fabs(v(2)) + fabs(v(3)); }

/// The euclidean norm of the vector, that is what most people call length
template<typename T>
inline
SGQuat<T>
normalize(const SGQuat<T>& q)
{ return (1/norm(q))*q; }

/// Return true if exactly the same
template<typename T>
inline
bool
operator==(const SGQuat<T>& v1, const SGQuat<T>& v2)
{ return v1(0)==v2(0) && v1(1)==v2(1) && v1(2)==v2(2) && v1(3)==v2(3); }

/// Return true if not exactly the same
template<typename T>
inline
bool
operator!=(const SGQuat<T>& v1, const SGQuat<T>& v2)
{ return ! (v1 == v2); }

/// Return true if equal to the relative tolerance tol
/// Note that this is not the same than comparing quaternions to represent
/// the same rotation
template<typename T>
inline
bool
equivalent(const SGQuat<T>& v1, const SGQuat<T>& v2, T tol)
{ return norm1(v1 - v2) < tol*(norm1(v1) + norm1(v2)); }

/// Return true if about equal to roundoff of the underlying type
/// Note that this is not the same than comparing quaternions to represent
/// the same rotation
template<typename T>
inline
bool
equivalent(const SGQuat<T>& v1, const SGQuat<T>& v2)
{ return equivalent(v1, v2, 100*SGLimits<T>::epsilon()); }

#ifndef NDEBUG
template<typename T>
inline
bool
isNaN(const SGQuat<T>& v)
{
  return SGMisc<T>::isNaN(v(0)) || SGMisc<T>::isNaN(v(1))
    || SGMisc<T>::isNaN(v(2)) || SGMisc<T>::isNaN(v(3));
}
#endif

/// quaternion interpolation for t in [0,1] interpolate between src (=0)
/// and dst (=1)
template<typename T>
inline
SGQuat<T>
interpolate(T t, const SGQuat<T>& src, const SGQuat<T>& dst)
{
  T cosPhi = dot(src, dst);
  // need to take the shorter way ...
  int signCosPhi = SGMisc<T>::sign(cosPhi);
  // cosPhi must be corrected for that sign
  cosPhi = fabs(cosPhi);

  // first opportunity to fail - make sure acos will succeed later -
  // result is correct
  if (1 <= cosPhi)
    return dst;

  // now the half angle between the orientations
  T o = acos(cosPhi);

  // need the scales now, if the angle is very small, do linear interpolation
  // to avoid instabilities
  T scale0, scale1;
  if (fabs(o) < SGLimits<T>::epsilon()) {
    scale0 = 1 - t;
    scale1 = t;
  } else {
    // note that we can give a positive lower bound for sin(o) here
    T sino = sin(o);
    T so = 1/sino;
    scale0 = sin((1 - t)*o)*so;
    scale1 = sin(t*o)*so;
  }

  return scale0*src + signCosPhi*scale1*dst;
}

/// Output to an ostream
template<typename char_type, typename traits_type, typename T>
inline
std::basic_ostream<char_type, traits_type>&
operator<<(std::basic_ostream<char_type, traits_type>& s, const SGQuat<T>& v)
{ return s << "[ " << v(0) << ", " << v(1) << ", " << v(2) << ", " << v(3) << " ]"; }

inline
SGQuatf
toQuatf(const SGQuatd& v)
{ return SGQuatf((float)v(0), (float)v(1), (float)v(2), (float)v(3)); }

inline
SGQuatd
toQuatd(const SGQuatf& v)
{ return SGQuatd(v(0), v(1), v(2), v(3)); }

#endif
