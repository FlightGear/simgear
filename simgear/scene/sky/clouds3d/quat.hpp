//------------------------------------------------------------------------------
// File : quat.hpp
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

/**************************************************************************
  
  quat.hpp

  A quaternion template class

  ---------------------------------------------------------------------

  Feb 1998, Paul Rademacher (rademach@cs.unc.edu)  

  Modification History:
  - Oct 2000, Bill Baxter.  Adapted to GLVU coding conventions and
      templetized.  Also implemented many methods that were declared but
      not defined.  Added some methods from quatlib and other sources.
                
**************************************************************************/

#ifndef _QUAT_H_
#define _QUAT_H_

#include "vec3f.hpp"
#include "mat44.hpp"
#include "mat33.hpp"

/****************************************************************
*                    Quaternion                                 *
****************************************************************/

template <class Type>
class Quat
{
public:
  typedef Vec3<Type> qvec;

  Type s;  /* scalar component */
  qvec v;  /* quat vector component */

  /* Constructors */

  Quat(void);
  Quat(Type s, Type x, Type y, Type z);
  Quat(Type x, Type y, Type z); // s=0
  Quat(Type s, const qvec& v);
  Quat(const qvec& v, Type s = 0.0); 
  Quat(const Type *d);        /* copy from four-element Type array s,x,y,z */
  Quat(const Quat &q);        /* copy from other Quat */

  /* Setters */

  void  Set( Type x, Type y, Type z );
  void  Set( Type s, Type x, Type y, Type z );
  void  Set( Type s, const qvec& v );
  void  Set( const qvec& v, Type s=0 );

  /* Operators */

  Quat &operator  = ( const Quat &v );      /* assignment of a Quat */
  Quat &operator += ( const Quat &v );      /* incrementation by a Quat */
  Quat &operator -= ( const Quat &v );      /* decrementation by a Quat */
  Quat &operator *= ( const Type d );       /* multiplication by a scalar */
  Quat &operator *= ( const Quat &v );      /* quat product (this*v) */
  Quat &operator /= ( const Type d );       /* division by a scalar */
  Type &operator [] ( int i);               /* indexing s=0,x=1,y=2,z=3 */
  
  /* special functions */
  
  Type Length(void) const;                  /* length of a Quat */
  Type LengthSqr(void) const;               /* squared length of a Quat */
  Type Norm(void) const;                    /* also squared length of a Quat */
  Quat &Normalize(void);                    /* normalize a Quat */
  Quat &Invert(void);                       /* q = q^-1 */
  Quat &Conjugate(void);                    /* q = q* */
  qvec Xform( const qvec &v ) const;        /* q*v*q-1 */
  Quat &Log(void);                          /* log(q) */
  Quat &Exp(void);                          /* exp(q) */
  qvec GetAxis( void ) const;               /* Get rot axis */
  Type GetAngle( void ) const;              /* Get rot angle (radians) */
  void SetAngle( Type rad_ang );            /* set rot angle (radians) */
  void ScaleAngle( Type f );                /* scale rot angle */
  void Print( ) const;                      /* print Quat */

  /* Conversions */
  Mat44<Type>& ToMat( Mat44<Type> &dest ) const;   /* to 4x4 matrix */
  Mat33<Type>& ToMat( Mat33<Type> &dest ) const;   /* to 3x3 matrix */
  Quat& FromMat( const Mat44<Type>& src );         /* from 4x4 rot matrix */
  Quat& FromMat( const Mat33<Type>& src );         /* from 3x3 rot matrix */

  void ToAngleAxis( Type &ang, qvec &ax ) const;  /* to rot angle AND axis */
  Quat& FromAngleAxis( Type ang, const qvec &ax );/*from rot angle AND axis */
  Quat& FromTwoVecs(const qvec &a, const qvec& b); /* quat from a to b */
  // to/from Euler Angles (XYZ-Fixed/ZYX-Relative, angles in radians)
  // See quatimpl.hpp for more detailed comments.
  Quat& FromEuler( Type yaw_Z, Type pitch_Y, Type roll_X);
  void ToEuler(Type &yaw_Z, Type &pitch_Y, Type &roll_X) const;
  

  // HELPERS
  static Type DEG2RAD(Type d);
  static Type RAD2DEG(Type d);
  static Type Sin(double d);
  static Type Cos(double d);
  static Type ACos(double d);
  static Type ASin(double d);
  static Type ATan(double d);
  static Type ATan2(double n, double d);

  // CONSTANTS
  static Type FUDGE();
  static Quat ZERO();
  static Quat IDENTITY();
}; 

/* Utility functions */
template <class Type>
Quat<Type>& QuatSlerp(
  Quat<Type> &dest, const Quat<Type>& from, const Quat<Type>& to, Type t );
template <class Type>
Quat<Type> QuatSlerp(const Quat<Type>& from, const Quat<Type>& to, Type t );

/* "Friends" */
template <class Type>
Quat<Type> operator -(const Quat<Type> &v);                      // -q1
template <class Type>
Quat<Type> operator +(const Quat<Type> &a, const Quat<Type> &b); // q1 + q2
template <class Type>
Quat<Type> operator -(const Quat<Type> &a, const Quat<Type> &b); // q1 - q2
template <class Type>
Quat<Type> operator *(const Quat<Type> &a, const Type d);        // q1 * 3.0
template <class Type>
Quat<Type> operator *(const Type d, const Quat<Type> &a);        // 3.0 * q1
template <class Type>
Quat<Type> operator *(const Quat<Type> &a, const Quat<Type> &b); // q1 * q2
template <class Type>
Quat<Type> operator /(const Quat<Type> &a, const Type d);        // q1 / 3.0
template <class Type>
bool operator ==(const Quat<Type> &a, const Quat<Type> &b);      // q1 == q2 ?
template <class Type>
bool operator !=(const Quat<Type> &a, const Quat<Type> &b);      // q1 != q2 ?



#include "quatimpl.hpp"



typedef Quat<float> Quatf;
typedef Quat<double> Quatd;

#endif
