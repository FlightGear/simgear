//------------------------------------------------------------------------------
// File : mat33.hpp
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

//============================================================================
// mat33.hpp : 3x3 matrix template.
//============================================================================

#ifndef _MAT33_
#define _MAT33_

#include "vec3f.hpp"
#include "mat44.hpp"

static const float Mat33TORADS = 0.0174532f;
static const float Mat33VIEWPORT_TOL = 0.001f;

//----------------------------------------------------------------------------
// M[9] = [ 0  3  6 ]   
//        [ 1  4  7 ]  
//        [ 2  5  8 ]  
//
//                         
//
// [ x' y' z' ] = [ 0  3  6 ]   [ x ]
//                [ 1  4  7 ] * [ y ] 
//                [ 2  5  8 ]   [ z ]
//----------------------------------------------------------------------------
template<class Type>
class Mat33
{ 
public:
  Type M[9];  // 0,1,2 = 1st col; 3,4,5 = 2nd col; etc.
  
  Mat33();
  Mat33(const Type *N);
  Mat33(Type M0, Type M3, Type M6,
	Type M1, Type M4, Type M7,
	Type M2, Type M5, Type M8);
  Mat33<Type>& operator = (const Mat33& A);  // ASSIGNMENT (=)
  // ASSIGNMENT (=) FROM AN ARRAY OF Type 
  Mat33<Type>& operator = (const Type* a);
  Mat33<Type> operator * (const Mat33& A) const; // MULTIPLICATION (*)
  // MAT-VECTOR MULTIPLICATION (*)
  Vec3<Type> operator * (const Vec3<Type>& V) const; 
  // MAT-VECTOR PRE-MULTIPLICATON (*)
  friend Vec3<Type> operator * (const Vec3<Type>& V, const Mat33<Type>& M);   
  // SCALAR POST-MULTIPLICATION
  Mat33<Type> operator * (Type a) const;
  // SCALAR PRE-MULTIPLICATION
  friend Mat33<Type> operator * (Type a, const Mat44<Type>& M); 

  Mat33<Type> operator / (Type a) const;      // SCALAR DIVISION
  Mat33<Type> operator + (Mat33& M) const;    // ADDITION (+)
  Mat33<Type>& operator += (Mat33& M);        // ACCUMULATE ADD (+=)
  Mat33<Type>& operator -= (Mat33& M);        // ACCUMULATE SUB (-=)
  Mat33<Type>& operator *= (Type a);          // ACCUMULATE MULTIPLY (*=)
  Mat33<Type>& operator /= (Type a);          // ACCUMULATE DIVIDE (/=)

  bool Inverse(Mat33<Type> &inv, Type tolerance) const;// MATRIX INVERSE
  
  operator const Type*() const;                     // CAST TO A Type ARRAY
  operator Type*();                                 // CAST TO A Type ARRAY

  void RowMajor(Type m[9]);                // return array in row-major order

  Type& operator()(int col, int row);               // 2D ARRAY ACCESSOR
  const Type& operator()(int col, int row) const;   // 2D ARRAY ACCESSOR
  void Set(const Type* a);    // SAME AS ASSIGNMENT (=) FROM AN ARRAY OF TypeS
  void Set(Type M0, Type M3, Type M6,
           Type M1, Type M4, Type M7,
           Type M2, Type M5, Type M8);
  
  void Identity();
  void Zero();
  void Transpose();
  void Scale(Type Sx, Type Sy, Type Sz);
  void Scale(const Vec3<Type>& S);
  void invScale(Type Sx, Type Sy, Type Sz);
  void invScale(const Vec3<Type>& S);
  void Rotate(Type DegAng, const Vec3<Type>& Axis);
  void invRotate(Type DegAng, const Vec3<Type>& Axis);
  void Star(const Vec3<Type>& v);  // SKEW-SYMM MATRIX EQUIV TO CROSS PROD WITH V
  void OuterProduct(const Vec3<Type>& u, const Vec3<Type>& v);  // SET TO  u * v^t
  Type Trace() const;
  void Print() const;
  void CopyInto(Type *Mat) const;
  static void SWAP(Type& a, Type& b) {Type t; t=a;a=b;b=t;}
};

#include "mat33impl.hpp"

typedef Mat33<float> Mat33f;
typedef Mat33<double> Mat33d;

#endif




