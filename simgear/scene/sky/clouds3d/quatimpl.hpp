//------------------------------------------------------------------------------
// File : quatimpl.hpp
//------------------------------------------------------------------------------
// GLVU : Copyright 1997 - 2002 
//        The University of North Carolina at Chapel Hill
//------------------------------------------------------------------------------
// Permission to use, copy, modify, distribute and sell this software and its 
// documentation for any purpose is hereby granted without fee, provided that 
// the above copyright notice appear in all copies and that both that copyright 
// notice and this permission notice appear in supporting documentation. 
// Binaries may be compiled with this software without any royalties or 
// restrictions. 
//
// The University of North Carolina at Chapel Hill makes no representations 
// about the suitability of this software for any purpose. It is provided 
// "as is" without express or implied warranty.

/***********************************************************************

  quatimpl.hpp

  A quaternion template class 

  -------------------------------------------------------------------

  Feb 1998, Paul Rademacher (rademach@cs.unc.edu)
  Oct 2000, Bill Baxter

  Modification History:
  - See main header file, quat.hpp

************************************************************************/

#include "quat.hpp"
#include <math.h>
#include <assert.h>


//============================================================================
// CONSTRUCTORS
//============================================================================

template <class Type>
Quat<Type>::Quat( void )
{
  // do nothing so default construction is fast
}

template <class Type>
Quat<Type>::Quat( Type _s, Type x, Type y, Type z )
{
  s = _s;
  v.Set( x, y, z );
}
template <class Type>
Quat<Type>::Quat( Type x, Type y, Type z )
{
  s = 0.0;
  v.Set( x, y, z );
}

template <class Type>
Quat<Type>::Quat( const qvec& _v, Type _s )
{
  Set( _v, _s );
}

template <class Type>
Quat<Type>::Quat( Type _s, const qvec& _v )
{
  Set( _v, _s );
}


template <class Type>
Quat<Type>::Quat( const Type *d )
{
  s = *d++;
  v.Set(d);
}

template <class Type>
Quat<Type>::Quat( const Quat &q )
{
  s = q.s;
  v = q.v;
}

//============================================================================
// SETTERS
//============================================================================

template <class Type>
void Quat<Type>::Set( Type _s, Type x, Type y, Type z )
{
  s = _s;
  v.Set(x,y,z);
}
template <class Type>
void Quat<Type>::Set( Type x, Type y, Type z )
{
  s = 0.0;
  v.Set(x,y,z);
}

template <class Type>
void Quat<Type>::Set( const qvec& _v, Type _s )
{
  s = _s;
  v = _v;
}
template <class Type>
void Quat<Type>::Set( Type _s, const qvec& _v )
{
  s = _s;
  v = _v;
}


//============================================================================
// OPERATORS
//============================================================================

template <class Type>
Quat<Type>& Quat<Type>::operator = (const Quat& q)
{ 
  v = q.v;  s = q.s; return *this; 
}

template <class Type>
Quat<Type>& Quat<Type>::operator += ( const Quat &q )
{
  v += q.v; s += q.s; return *this;
}

template <class Type>
Quat<Type>& Quat<Type>::operator -= ( const Quat &q )
{
  v -= q.v; s -= q.s; return *this;
}

template <class Type>
Quat<Type> &Quat<Type>::operator *= ( const Type d ) 
{
  v *= d; s *= d; return *this;
}

template <class Type>
Quat<Type> &Quat<Type>::operator *= ( const Quat& q ) 
{
#if 0
  // Quaternion multiplication with 
  // temporary object construction minimized (hopefully)
  Type ns = s*q.s - v*q.v;
  qvec nv(v^q.v);
  v *= q.s;
  v += nv;
  nv.Set(s*q.v);
  v += nv;
  s = ns;
  return *this;
#else
  // optimized (12 mults, and no compiler-generated temp objects)
  Type A, B, C, D, E, F, G, H;

  A = (s   + v.x)*(q.s   + q.v.x);
  B = (v.z - v.y)*(q.v.y - q.v.z);
  C = (s   - v.x)*(q.v.y + q.v.z); 
  D = (v.y + v.z)*(q.s   - q.v.x);
  E = (v.x + v.z)*(q.v.x + q.v.y);
  F = (v.x - v.z)*(q.v.x - q.v.y);
  G = (s   + v.y)*(q.s   - q.v.z);
  H = (s   - v.y)*(q.s   + q.v.z);

  v.x = A - (E + F + G + H) * Type(0.5); 
  v.y = C + (E - F + G - H) * Type(0.5); 
  v.z = D + (E - F - G + H) * Type(0.5);
  s = B + (-E - F + G + H) * Type(0.5);

  return *this;
#endif
}

template <class Type>
Quat<Type> &Quat<Type>::operator /= ( const Type d )
{
  Type r = Type(1.0)/d;
  v *= r;
  s *= r;
  return *this;
}

template <class Type>
Type &Quat<Type>::operator [] ( int i)
{
  switch (i) {
    case 0: return s;
    case 1: return v.x;
    case 2: return v.y;
    case 3: return v.z;
  }
  assert(false);
  return s;
}

//============================================================================
// SPECIAL FUNCTIONS
//============================================================================
template <class Type>
inline Type Quat<Type>::Length( void ) const
{
  return Type( sqrt( v*v + s*s ) );
}

template <class Type>
inline Type Quat<Type>::LengthSqr( void ) const
{
  return Norm();
}

template <class Type>
inline Type Quat<Type>::Norm( void ) const
{
  return v*v + s*s;
}

template <class Type>
inline Quat<Type>& Quat<Type>::Normalize( void )
{
  *this *= Type(1.0) / Type(sqrt(v*v + s*s));
  return *this;
}

template <class Type>
inline Quat<Type>& Quat<Type>::Invert( void )
{
  Type scale = Type(1.0)/Norm();
  v *= -scale;
  s *= scale;
  return *this;
}

template <class Type>
inline Quat<Type>& Quat<Type>::Conjugate( void )
{
  v.x = -v.x;
  v.y = -v.y;
  v.z = -v.z;
  return *this;
}


//----------------------------------------------------------------------------
// Xform
//----------------------------------------------------------------------------
// Transform a vector by this quaternion using q * v * q^-1
//----------------------------------------------------------------------------
template <class Type>
Quat<Type>::qvec Quat<Type>::Xform( const qvec &vec ) const
{
  /* copy vector into temp quaternion for multiply	*/
  Quat  vecQuat(vec);
  /* invert multiplier	*/
  Quat  inverse(*this);
  inverse.Invert();

  /* do q * vec * q(inv)	*/
  Quat  tempVecQuat(*this * vecQuat);
  tempVecQuat *= inverse;

  /* return vector part */
  return tempVecQuat.v;
}

//----------------------------------------------------------------------------
// Log
//----------------------------------------------------------------------------
// Natural log of quat
//----------------------------------------------------------------------------
template <class Type>
Quat<Type> &Quat<Type>::Log(void)
{
  Type theta, scale;

  scale = v.Length();
  theta = ATan2(scale, s);

  if (scale > 0.0)
    scale = theta/scale;

  v *= scale;
  s = 0.0;
  return *this;
}

//----------------------------------------------------------------------------
// Exp
//----------------------------------------------------------------------------
// e to the quat: e^quat 
// -- assuming scalar part 0
//----------------------------------------------------------------------------
template <class Type>
Quat<Type> &Quat<Type>::Exp(void)
{
  Type scale;
  Type theta = v.Length();

  if (theta > FUDGE()) {
    scale = Sin(theta)/theta ;
    v *= scale;
  }

  s = Cos(theta) ; 
  return *this;
}


//----------------------------------------------------------------------------
// SetAngle (radians)
//----------------------------------------------------------------------------
template <class Type>
void Quat<Type>::SetAngle( Type f )
{
  qvec axis(GetAxis());
  f *= Type(0.5);
  s = Cos( f );
  v = axis * Sin( f );
}

//----------------------------------------------------------------------------
// ScaleAngle 
//----------------------------------------------------------------------------
template <class Type>
inline void  Quat<Type>::ScaleAngle( Type f )
{
  SetAngle( f * GetAngle() );
}

//----------------------------------------------------------------------------
// GetAngle (radians)
//----------------------------------------------------------------------------
// get rot angle in radians.  Assumes s is between -1 and 1, which will always
// be the case for unit quaternions.
//----------------------------------------------------------------------------
template <class Type>
inline Type Quat<Type>::GetAngle( void ) const
{
  return ( Type(2.0) * ACos( s ) );
}

//----------------------------------------------------------------------------
// GetAxis
//----------------------------------------------------------------------------
template <class Type>
Quat<Type>::qvec Quat<Type>::GetAxis( void ) const
{
  Type scale;

  scale = Sin( acos( s ) ) ;
  if ( scale < FUDGE() && scale > -FUDGE() )
    return qvec( 0.0, 0.0, 0.0 );
  else
    return  v / scale;
}

//---------------------------------------------------------------------------- 
// Print
//---------------------------------------------------------------------------- 
template <class Type>
inline void Quat<Type>::Print( ) const
{
  printf( "(%3.2f, <%3.2f %3.2f %3.2f>)\n", s, v.x, v.y, v.z );
}	


//============================================================================
// CONVERSIONS
//============================================================================

template <class Type>
Mat44<Type>& Quat<Type>::ToMat( Mat44<Type>& dest ) const
{
  Type t, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;
  qvec  a, c, b, d;
  
  t  = Type(2.0) / (v*v + s*s);
  const Type ONE(1.0);
  
  xs = v.x*t;   ys = v.y*t;   zs = v.z*t;
  wx = s*xs;    wy = s*ys;    wz = s*zs;
  xx = v.x*xs;  xy = v.x*ys;  xz = v.x*zs;
  yy = v.y*ys;  yz = v.y*zs;  zz = v.z*zs;
  
  dest.Set( ONE-(yy+zz), xy-wz,       xz+wy,       0.0,
            xy+wz,       ONE-(xx+zz), yz-wx,       0.0,
            xz-wy,       yz+wx,       ONE-(xx+yy), 0.0,
            0.0,         0.0,         0.0,         ONE );
  
  return dest;
}

template <class Type>
Mat33<Type>&  Quat<Type>::ToMat( Mat33<Type>& dest ) const
{
  Type t, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;
  qvec  a, c, b, d;
  
  t  = Type(2.0) / Norm();
  const Type ONE(1.0);
  
  xs = v.x*t;   ys = v.y*t;   zs = v.z*t;
  wx = s*xs;    wy = s*ys;    wz = s*zs;
  xx = v.x*xs;  xy = v.x*ys;  xz = v.x*zs;
  yy = v.y*ys;  yz = v.y*zs;  zz = v.z*zs;
  
  dest.Set( ONE-(yy+zz), xy-wz,       xz+wy,       
            xy+wz,       ONE-(xx+zz), yz-wx,       
            xz-wy,       yz+wx,       ONE-(xx+yy) );

  return dest;
}

//----------------------------------------------------------------------------
// FromMat
//----------------------------------------------------------------------------
// Convert rotation matrix to quaternion
// Results will be bad if matrix is not (very close to) orthonormal
// Modified from gamasutra.com article:
// http://www.gamasutra.com/features/programming/19980703/quaternions_07.htm
//----------------------------------------------------------------------------
template <class Type>
Quat<Type>&  Quat<Type>::FromMat( const Mat44<Type>& m )
{
  Type tr = m.Trace();

  // check the diagonal
  if (tr > 0.0) {
    Type scale = Type( sqrt (tr) );
    s = scale * Type(0.5);
    scale = Type(0.5) / scale;
    v.x = (m(1,2) - m(2,1)) * scale;
    v.y = (m(2,0) - m(0,2)) * scale;
    v.z = (m(0,1) - m(1,0)) * scale;
  } else {		
    // diagonal is negative or zero
    int i, j, k;
    i = 0;
    if (m(1,1) > m(0,0)) i = 1;
    if (m(2,2) > m(i,i)) i = 2;
    int nxt[3] = {1, 2, 0};
    j = nxt[i];
    k = nxt[j];

    Type scale = Type( sqrt (Type(1.0) + m(i,i) - (m(j,j) + m(k,k)) ) );
      
    v[i] = scale * Type(0.5);
            
    if (scale != 0.0) scale = Type(0.5) / scale;

    s = (m(j,k) - m(k,j)) * scale;
    v[j] = (m(i,j) + m(j,i)) * scale;
    v[k] = (m(i,k) + m(k,i)) * scale;
  }
  return *this;
}

template <class Type>
Quat<Type>&  Quat<Type>::FromMat( const Mat33<Type>& m )
{
  Type tr = m.Trace();

  // check the diagonal
  if (tr > 0.0) {
    Type scale = Type( sqrt (tr + Type(1.0)) );
    s = scale * Type(0.5);
    scale = Type(0.5) / scale;
    v.x = (m(1,2) - m(2,1)) * scale;
    v.y = (m(2,0) - m(0,2)) * scale;
    v.z = (m(0,1) - m(1,0)) * scale;
  } else {		
    // diagonal is negative or zero
    int i, j, k;
    i = 0;
    if (m(1,1) > m(0,0)) i = 1;
    if (m(2,2) > m(i,i)) i = 2;
    int nxt[3] = {1, 2, 0};
    j = nxt[i];
    k = nxt[j];

    Type scale = Type( sqrt (Type(1.0) + m(i,i) - (m(j,j) + m(k,k)) ) );
      
    v[i] = scale * Type(0.5);
            
    if (scale != 0.0) scale = Type(0.5) / scale;

    s = (m(j,k) - m(k,j)) * scale;
    v[j] = (m(i,j) + m(j,i)) * scale;
    v[k] = (m(i,k) + m(k,i)) * scale;
  }
  return *this;
}

//----------------------------------------------------------------------------
// ToAngleAxis (radians)
//----------------------------------------------------------------------------
// Convert to angle & axis representation
//----------------------------------------------------------------------------
template <class Type>
void Quat<Type>::ToAngleAxis( Type &angle, qvec &axis ) const
{
  Type cinv = ACos( s );
  angle = Type(2.0) * cinv;

  Type scale;

  scale = Sin( cinv );
  if ( scale < FUDGE() && scale > -FUDGE() )
    axis = qvec::ZERO;
  else {
    axis = v;
    axis /= scale;
  }
}

//----------------------------------------------------------------------------
// FromAngleAxis (radians)
//----------------------------------------------------------------------------
// Convert to quat from angle & axis representation
//----------------------------------------------------------------------------
template <class Type>
Quat<Type>& Quat<Type>::FromAngleAxis( Type angle, const qvec &axis )
{
  /* normalize vector */
  Type length = axis.Length();

  /* if zero vector passed in, just set to identity quaternion */
  if ( length < FUDGE() )
  {
    *this = IDENTITY();
    return *this;
  }
  length = Type(1.0)/length;
  angle *= 0.5;
  v = axis;
  v *= length;
  v *= Sin(angle);

  s = Cos(angle);
  return *this;
}

//----------------------------------------------------------------------------
// FromTwoVecs
//----------------------------------------------------------------------------
// Return the quat that rotates vector a into vector b
//----------------------------------------------------------------------------
template <class Type>
Quat<Type>& Quat<Type>::FromTwoVecs(const qvec &a, const qvec& b)
{
  qvec u1(a);
  qvec u2(b);
  double theta ;				     /* angle of rotation about axis */
  double theta_complement ;
  double crossProductMagnitude ;


  // Normalize both vectors and take cross product to get rotation axis. 
  u1.Normalize();
  u2.Normalize();
  qvec axis( u1 ^ u2 );


  // | u1 X u2 | = |u1||u2|sin(theta)
  //
  // Since u1 and u2 are normalized, 
  //
  //  theta = arcsin(|axis|)
  crossProductMagnitude = axis.Length();

  // Occasionally, even though the vectors are normalized, the
  // magnitude will be calculated to be slightly greater than one.  If
  // this happens, just set it to 1 or asin() will barf.
  if( crossProductMagnitude > Type(1.0) )
    crossProductMagnitude = Type(1.0) ;

  // Take arcsin of magnitude of rotation axis to compute rotation
  // angle.  Since crossProductMagnitude=[0,1], we will have
  // theta=[0,pi/2].
  theta = ASin( crossProductMagnitude ) ;
  theta_complement = Type(3.14159265358979323846) - theta ;

  // If cos(theta) < 0, use complement of theta as rotation angle.
  if( u1 * u2 < 0.0 )
  {
    double tmp = theta;
    theta = theta_complement ;
    theta_complement = tmp;
  }

  // if angle is 0, just return identity quaternion
  if( theta < FUDGE() )
  {
    *this = IDENTITY();
  }
  else
  {
    if( theta_complement < FUDGE() )
    {
      // The two vectors are opposed.  Find some arbitrary axis vector.
      // First try cross product with x-axis if u1 not parallel to x-axis.
      if( (u1.y*u1.y + u1.z*u1.z) >= FUDGE() )
      {
        axis.Set( 0.0, u1.z, -u1.y ) ;
      }
      else
      {
        // u1 is parallel to to x-axis.  Use z-axis as axis of rotation.
        axis.Set(0.0, 0.0, 1.0);
      }
    }

    axis.Normalize();
    FromAngleAxis(Type(theta), axis);
    Normalize();
  }
  return *this;  
}

//----------------------------------------------------------------------------
// FromEuler
//----------------------------------------------------------------------------
// converts 3 euler angles (in radians) to a quaternion
//
// angles are in radians; Assumes roll is rotation about X, pitch is
// rotation about Y, yaw is about Z.  (So thinking of
// Z as up) Assumes order of yaw, pitch, roll applied as follows:
//
//	    p' = roll( pitch( yaw(p) ) )
//
// Where yaw, pitch, and roll are defined in the BODY coordinate sys.
// In other words these are ZYX-relative (or XYZ-fixed) Euler Angles.
//
// For a complete Euler angle implementation that handles all 24 angle
// sets, see "Euler Angle Conversion" by Ken Shoemake, in "Graphics
// Gems IV", Academic Press, 1994
//----------------------------------------------------------------------------
template <class Type>
Quat<Type>& Quat<Type>::FromEuler(Type yaw, Type pitch, Type roll)
{
  Type  cosYaw, sinYaw, cosPitch, sinPitch, cosRoll, sinRoll;
  Type  half_roll, half_pitch, half_yaw;

  /* put angles into radians and divide by two, since all angles in formula
   *  are (angle/2)
   */
  const Type HALF(0.5);
  half_yaw   = yaw   * HALF;
  half_pitch = pitch * HALF;
  half_roll  = roll  * HALF;

  cosYaw = Cos(half_yaw);
  sinYaw = Sin(half_yaw);

  cosPitch = Cos(half_pitch);
  sinPitch = Sin(half_pitch);

  cosRoll = Cos(half_roll);
  sinRoll = Sin(half_roll);

  Type cpcy = cosPitch * cosYaw;
  Type spsy = sinPitch * sinYaw;

  v.x = sinRoll * cpcy              - cosRoll * spsy;
  v.y = cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw;
  v.z = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;
  s   = cosRoll * cpcy              + sinRoll * spsy;

  return *this;
}

//----------------------------------------------------------------------------
// ToEuler
//----------------------------------------------------------------------------
// converts a quaternion to  3 euler angles (in radians)
//
// See FromEuler for details of which set of Euler Angles are returned
//----------------------------------------------------------------------------
template <class Type>
void Quat<Type>::ToEuler(Type& yaw, Type& pitch, Type& roll) const
{
  // This is probably wrong
  Mat33<Type> M;
  ToMat(M);
  const int i = 0, j = 1, k = 2;
  double cy = sqrt(M(i,i)*M(i,i) + M(i,j)*M(i,j));
  if (cy > FUDGE()) {
    roll = ATan2(M(j,k), M(k,k));
    pitch = ATan2(-M(i,k), cy);
    yaw = ATan2(M(i,j), M(i,i));
  } else {
    roll = ATan2(-M(k,j), M(j,j));
    pitch = ATan2(-M(i,k), cy);
    yaw = 0;
  }
}

//============================================================================
// QUAT FRIENDS
//============================================================================


template <class Type>
Quat<Type> operator + (const Quat<Type> &a, const Quat<Type> &b)
{
  return Quat<Type>( a.s+b.s, a.v+b.v );
}

template <class Type>
Quat<Type> operator - (const Quat<Type> &a, const Quat<Type> &b)
{
  return Quat<Type>( a.s-b.s, a.v-b.v );
}

template <class Type>
Quat<Type> operator - (const Quat<Type> &a )
{
  return Quat<Type>( -a.s, -a.v );
}

template <class Type>
Quat<Type> operator * ( const Quat<Type> &a, const Quat<Type> &b)
{
#if 0
  // 16 mults
  return Quat<Type>( a.s*b.s - a.v*b.v, a.s*b.v + b.s*a.v + a.v^b.v );
#else 
  // optimized (12 mults, and no compiler-generated temp objects)
  Type A, B, C, D, E, F, G, H;

  A = (a.s   + a.v.x)*(b.s   + b.v.x);
  B = (a.v.z - a.v.y)*(b.v.y - b.v.z);
  C = (a.s   - a.v.x)*(b.v.y + b.v.z); 
  D = (a.v.y + a.v.z)*(b.s   - b.v.x);
  E = (a.v.x + a.v.z)*(b.v.x + b.v.y);
  F = (a.v.x - a.v.z)*(b.v.x - b.v.y);
  G = (a.s   + a.v.y)*(b.s   - b.v.z);
  H = (a.s   - a.v.y)*(b.s   + b.v.z);

  return Quat<Type>(
    B + (-E - F + G + H) * Type(0.5),
    A - (E + F + G + H) * Type(0.5), 
    C + (E - F + G - H) * Type(0.5), 
    D + (E - F - G + H) * Type(0.5));
#endif
}

template <class Type>
Quat<Type> operator * ( const Quat<Type> &a, const Type t)
{
  return Quat<Type>( a.v * t, a.s * t );
}

template <class Type>
Quat<Type> operator * ( const Type t, const Quat<Type> &a )
{
  return Quat<Type>( a.v * t, a.s * t );
}
template <class Type>
Quat<Type> operator / ( const Quat<Type> &a, const Type t )
{
  return Quat<Type>( a.v / t, a.s / t );
}

template <class Type>
bool operator == (const Quat<Type> &a, const Quat<Type> &b)
{
  return (a.s == b.s && a.v == b.v);
}
template <class Type>
bool operator != (const Quat<Type> &a, const Quat<Type> &b)
{
  return (a.s != b.s || a.v != b.v);
}


//============================================================================
// UTILS
//============================================================================
template <class Type>
inline Type Quat<Type>::DEG2RAD(Type d)
{
  return d * Type(0.0174532925199432957692369076848861);
}
template <class Type>
inline Type Quat<Type>::RAD2DEG(Type d)
{
  return d * Type(57.2957795130823208767981548141052);
}

template <class Type>
inline Type Quat<Type>::Sin(double d)  { return Type(sin(d)); }
template <class Type>
inline Type Quat<Type>::Cos(double d)  { return Type(cos(d)); }
template <class Type>
inline Type Quat<Type>::ACos(double d) { return Type(acos(d)); }
template <class Type>
inline Type Quat<Type>::ASin(double d) { return Type(asin(d)); }
template <class Type>
inline Type Quat<Type>::ATan(double d) { return Type(atan(d)); }
template <class Type>
inline Type Quat<Type>::ATan2(double n, double d) {return Type(atan2(n,d));}

template <class Type>
inline Quat<Type> Quat<Type>::ZERO() {return Quat(0,0,0,0); }
template <class Type>
inline Quat<Type> Quat<Type>::IDENTITY() {return Quat(1,0,0,0); }

template<class Type>
inline Type Quat<Type>::FUDGE()    { return 1e-6; }
template<>
inline double Quat<double>::FUDGE() { return 1e-10; }

//----------------------------------------------------------------------------
// QuatSlerp
//----------------------------------------------------------------------------
template <class Type>
Quat<Type>& QuatSlerp(
  Quat<Type> &dest, 
  const Quat<Type> &from, const Quat<Type> &to, Type t )
{
#if 0
  // compact mathematical version
  // exp(t*log(to*from^-1))*from
  Quat<Type> fminv(from); 
  Quat<Type> tofrom(to*fminv.Invert());
  Quat<Type> slerp = t*tofrom.Log();
  slerp.Exp();
  slerp *= from;
  return slerp;
#endif
  Quat<Type> to1;
  double omega, cosom, sinom, scale0, scale1;

  /* calculate cosine */
  cosom = from.v * to.v + from.s + to.s;

  /* Adjust signs (if necessary) so we take shorter path */
  if ( cosom < 0.0 ) {
    cosom = -cosom;
    to1 = -to;
  }
  else
  {
    to1 = to;
  }

  /* Calculate coefficients */
  if ((1.0 - cosom) > Quat<Type>::FUDGE ) {
    /* standard case (slerp) */
    omega = acos( cosom );
    sinom = sin( omega );
    scale0 = sin((1.0 - t) * omega) / sinom;
    scale1 = sin(t * omega) / sinom;
  }
  else {
    /* 'from' and 'to' are very close - just do linear interpolation */
    scale0 = 1.0 - t;
    scale1 = t;      
  }

  dest = from;
  dest *= Type(scale0);
  dest += Type(scale1) * to1;
  return dest;
}

// This version creates more temporary objects
template <class Type>
inline Quat<Type> QuatSlerp(
  const Quat<Type>& from, const Quat<Type>& to, Type t )
{
  Quat<Type> q;
  return QuatSlerp(q, from, to, t);
}


