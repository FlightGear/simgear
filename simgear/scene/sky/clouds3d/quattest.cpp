//------------------------------------------------------------------------------
// File : quattest.cpp
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

//----------------------------------------------------------------------------
// Quattest.cpp
//  Quaternion coverage (and maybe functionality) test.
//  The main objective is to just make sure every API gets called
//  to flush out lurking template bugs that might not otherwise get tickled.
//  A few checks to make sure things are functioning properly are also 
//  included.
//
//  For the API coverage, if the thing simply compiles then it was a success.
//----------------------------------------------------------------------------


#include "quat.hpp"

void coverage_test()
{
  float x = 0.5f, y = 1.1f, z = -0.1f, s = 0.2f;
  // TEST CONSTRUCTORS
  Quatf q1;
  Quatf q2 = Quatf(x, y, z, s);
  Quatf q3(x, y, z);
  Quatf q4(Vec3f(1,2,3)); 
  Quatf q5(Vec3f(2,3,4), 2.0);
  Quatf q6(1.0, Vec3f(-1,-2,-3));
  float floatarray[4] = { 1, 2, 3, 4 };
  Quatf q7(floatarray);
  Quatf q8(q7);

  // TEST SETTERS
  q1.Set(1,2,3);
  q2.Set(1,2,3,4);
  q3.Set(Vec3f(1.0,1.0,1.0));
  q3.Set(Vec3f(1.0,1.0,1.0),-0.5);

  // TEST OPERATORS
  // Quat &operator  = ( const Quat &v );      /* assignment of a Quat */
  q4 = q6;
  // Quat &operator += ( const Quat &v );      /* incrementation by a Quat */
  q3 += q2;
  // Quat &operator -= ( const Quat &v );      /* decrementation by a Quat */
  q3 -= q2;
  // Quat &operator *= ( const Type d );       /* multiplication by a scalar */
  q4 *= 0.5f;
  // Quat &operator *= ( const Quat &v );      /* quat product (this*v) */
  q2 *= q1;
  // Quat &operator /= ( const Type d );       /* division by a scalar */
  q5 /= 2.0f;
  // Type &operator [] ( int i);               /* indexing x=0, s=3 */
  float c0 = q1[0];
  float c1 = q1[1];
  float c2 = q1[2];
  float c3 = q1[3];
  
  // TEST SPECIAL FUNCTIONS
  
  // Type Length(void) const;                  /* length of a Quat */
  float l = q4.Length();
  // Type LengthSqr(void) const;               /* squared length of a Quat */
  l = q4.LengthSqr();
  // Type LengthSqr(void) const;               /* squared length of a Quat */
  l = q4.Norm();
  // Quat &Normalize(void);                    /* normalize a Quat */
  Quatf q9 = q4.Normalize();
  // Quat &Invert(void);                       /* q = q^-1 */
  q9 = q4.Invert();
  // Quat &Conjugate(void);                    /* q = q* */
  q9 = q4.Conjugate();
  // qvec Xform( const qvec &v );              /* q*v*q-1 */
  Vec3f v1 = q4.Xform(Vec3f(1.0,1.0,1.0));
  // Quat &Log(void);                          /* log(q) */
  q9 = q4.Log();
  // Quat &Exp(void);                          /* exp(q) */
  q9 = q4.Exp();
  // qvec GetAxis( void ) const;               /* Get rot axis */
  v1 = q5.GetAxis();
  // Type GetAngle( void ) const;              /* Get rot angle (radians) */
  float a = q5.GetAngle();
  // void SetAngle( Type rad_ang );            /* set rot angle (radians) */
  q2.SetAngle(a);
  // void ScaleAngle( Type f );                /* scale rot angle */
  q2.ScaleAngle(1.5);
  // void Print( ) const;                      /* print Quat */
  q3.Print();

  /* TEST CONVERSIONS */
  Mat44f m44;
  Mat33f m33;
  //Mat44& ToMat( Mat44 &dest ) const;        
  //Mat33& ToMat( Mat33 &dest ) const;        
  // Quat& FromMat( const Mat44<Type>& src ) const;
  // Quat& FromMat( const Mat33<Type>& src ) const;
  q3.Normalize();
  m44 = q3.ToMat(m44);
  m33 = q3.ToMat(m33);
  q5 = q3.FromMat(m44);
  q5 = q3.FromMat(m33);
  //void ToAngleAxis( Type &ang, qvec &ax ) const;  
  q3.ToAngleAxis( a, v1 );
  //Quat& FromAngleAxis( Type ang, const qvec &ax );
  q4 = q3.FromAngleAxis( a, v1 );
  //Quat& FromTwoVecs(const qvec &a, const qvec& b); 
  Vec3f v2(-1,-2,-3);
  q4 = q3.FromTwoVecs( v2, v1 );
  //Quat& FromEuler( Type yaw, Type pitch, Type roll);
  q4 = q3.FromEuler( 2.2f, 1.2f, -0.4f );
  //void ToEuler(Type &yaw, Type &pitch, Type &roll) const;
  float p=0.3f,r=-1.57f; y= 0.1f;
  q3.ToEuler( y,p,r );

  /* TEST FRIENDS */

  //friend Quat operator - (const Quat &v);                 /* -q1 */
  q1 = -q2;
  //friend Quat operator + (const Quat &a, const Quat &b);  /* q1 + q2 */
  q1 = q2 + q3;
  //friend Quat operator - (const Quat &a, const Quat &b);  /* q1 - q2 */
  q1 = q2 - q3;
  //friend Quat operator * (const Quat &a, const Type d);   /* q1 * 3.0 */
  q1 = q2 * 0.2f;
  //friend Quat operator * (const Type d, const Quat &a);   /* 3.0 * q1 */
  q1 = 0.2f * q2;
  //friend Quat operator * (const Quat &a, const Quat &b);  /* q1 * q2 */
  q1 = q2 * q3;
  //friend Quat operator / (const Quat &a, const Type d);   /* q1 / 3.0 */
  q1 = q2 / 1.2f;
  //friend bool operator == (const Quat &a, const Quat &b); /* q1 == q2 ? */
  bool eq = (q1 == q2);
  //friend bool operator != (const Quat &a, const Quat &b); /* q1 != q2 ? */
  bool neq = (q1 != q2);

  // HELPERS
  // static Type DEG2RAD(Type d);
  // static Type RAD2DEG(Type d);
  a = Quatf::RAD2DEG(a);
  a = Quatf::DEG2RAD(a);
  a = Quatf::Sin(a);
  a = Quatf::Cos(a);
  a = Quatf::ACos(a);
  a = Quatf::ASin(a);
  a = Quatf::ATan(a);
  a = Quatf::ATan2(a, p);

  // CONSTANTS
  // static const Type FUDGE;
  // static const Quat ZERO;
  // static const Quat IDENTITY;
  a = Quatf::FUDGE;
  q3 = Quatf::ZERO();
  q4 = Quatf::IDENTITY();

  q1 = QuatSlerp(q1, q3, q4, 0.5f );
  q1 = QuatSlerp(q3, q4, 0.5f );
  
}

Quatf StatQuat(Quatf::IDENTITY());
Quatf StatQuat2 = Quatf::IDENTITY();

void functional_test()
{
  printf("The ZERO quat: ");
  Quatf::ZERO().Print();
  printf("The IDENTITY quat: ");
  Quatf::IDENTITY().Print();
  printf("Statically constructed copy of IDENTITY quat: ");
  StatQuat.Print();
  printf("A different static copy of IDENTITY quat: ");
  StatQuat2.Print();


}

int main(int argc, char *argv[])
{
  coverage_test();
  functional_test();

  return (0);
}
