//------------------------------------------------------------------------------
// File : mat44.hpp
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
// mat44.hpp : 4x4 OpenGL-style matrix template.
//============================================================================

#ifndef _MAT44_
#define _MAT44_

#include "vec3f.hpp"
#include "vec4f.hpp"

static const float Mat44TORADS = 0.0174532f;
static const float Mat44VIEWPORT_TOL = 0.001f;

//----------------------------------------------------------------------------
// M[16] = [ 0  4  8 12 ]   |  16 floats were used instead of the normal [4][4]
//         [ 1  5  9 13 ]   |  to be compliant with OpenGL. OpenGL uses
//         [ 2  6 10 14 ]   |  premultiplication with column vectors. These
//         [ 3  7 11 15 ]   |  matrices can be fed directly into the OpenGL
//                          |  matrix stack with glLoadMatrix or glMultMatrix.
//
// [ x' y' z' w' ] = [ 0  4  8 12 ]   [ x ]
//                   [ 1  5  9 13 ] * [ y ] 
//                   [ 2  6 10 14 ]   [ z ]
//                   [ 3  7 11 15 ]   [ w ]
// 
// Loading a [4][4] format matrix directly into the matrix stack (assuming
// premult/col vecs) results in a transpose matrix. M[0]=M[0][0], but
// M[1]!=M[1][0] since M[0][1] would cast to M[1].
//
// However, if we assumed postmult/row vectors we could use [4][4] format,
// but all transformations in this module would be transposed.
//----------------------------------------------------------------------------
template<class Type>
class Mat44
{ 
  public:
    Type M[16];  // 0,1,2,3 = 1st col; 4,5,6,7 = 2nd col; etc.

    Mat44();
    Mat44(const Type *N);
    Mat44(Type M0, Type M4, Type M8, Type M12,
          Type M1, Type M5, Type M9, Type M13,
          Type M2, Type M6, Type M10, Type M14,
          Type M3, Type M7, Type M11, Type M15);
    Mat44<Type>& operator = (const Mat44& A);         // ASSIGNMENT (=)
    Mat44<Type>& operator = (const Type* a);                // ASSIGNMENT (=) FROM AN ARRAY OF TypeS 
    Mat44<Type> operator * (const Mat44& A) const;    // MULTIPLICATION (*)
    Vec3<Type> operator * (const Vec3<Type>& V) const;      // MAT-VECTOR MULTIPLICATION (*) W/ PERSP DIV
    Vec3<Type> multNormal(const Vec3<Type>& V) const;      // MAT-VECTOR MULTIPLICATION _WITHOUT_ PERSP DIV
    Vec3<Type> multPoint(const Vec3<Type>& V) const;      // MAT-POINT MULTIPLICATION _WITHOUT_ PERSP DIV
    Vec4<Type> operator * (const Vec4<Type>& V) const;      // MAT-VECTOR MULTIPLICATION (*)
    Mat44<Type> operator * (Type a) const;                  // SCALAR POST-MULTIPLICATION
    Mat44<Type>& operator *= (Type a);                    // ACCUMULATE MULTIPLY (*=)
////////////// NOT IMPLEMENTED YET. ANY TAKERS?
//    friend Vec3<Type> operator * (const Vec3<Type>& V, const Mat44& M);   // MAT-VECTOR PRE-MULTIPLICATON (*) W/ PERP DIV
//    friend Vec4<Type> operator * (const Vec4<Type>& V, const Mat44& M);   // MAT-VECTOR PRE-MULTIPLICATON (*)
//    friend Mat44<Type> operator * (Type a, const Mat44& M) const;         // SCALAR PRE-MULTIPLICATION
//    Mat44<Type> operator / (Type a) const;                // SCALAR DIVISION
//    Mat44<Type> operator + (Mat44& M) const;        // ADDITION (+)
//    Mat44<Type>& operator += (Mat44& M);            // ACCUMULATE ADD (+=)
//    Mat44<Type>& operator /= (Type a);                    // ACCUMULATE DIVIDE (/=)
//    bool Invserse();                                      // MATRIX INVERSE
////////////// NOT IMPLEMENTED YET. ANY TAKERS?

    operator const Type*() const;                           // CAST TO A Type ARRAY
    operator Type*();                                       // CAST TO A Type ARRAY
    Type& operator()(int col, int row);                     // 2D ARRAY ACCESSOR
    const Type& operator()(int col, int row) const;         // 2D ARRAY ACCESSOR
    void Set(const Type* a);                                // SAME AS ASSIGNMENT (=) FROM AN ARRAY OF TypeS
    void Set(Type M0, Type M4, Type M8, Type M12,
             Type M1, Type M5, Type M9, Type M13,
             Type M2, Type M6, Type M10, Type M14,
             Type M3, Type M7, Type M11, Type M15);


    void Identity();
    void Transpose();
    void Translate(Type Tx, Type Ty, Type Tz);
    void Translate(const Vec3<Type>& T);
    void invTranslate(Type Tx, Type Ty, Type Tz);
    void invTranslate(const Vec3<Type>& T);
    void Scale(Type Sx, Type Sy, Type Sz);
    void Scale(const Vec3<Type>& S);
    void invScale(Type Sx, Type Sy, Type Sz);
    void invScale(const Vec3<Type>& S);
    void Rotate(Type DegAng, const Vec3<Type>& Axis);
    void invRotate(Type DegAng, const Vec3<Type>& Axis);
    Type Trace(void) const;
    void Frustum(Type l, Type r, Type b, Type t, Type n, Type f);
    void invFrustum(Type l, Type r, Type b, Type t, Type n, Type f);
    void Perspective(Type Yfov, Type Aspect, Type Ndist, Type Fdist);
    void invPerspective(Type Yfov, Type Aspect, Type Ndist, Type Fdist);
    void Viewport(int WW, int WH);
    void invViewport(int WW, int WH);
    void LookAt(const Vec3<Type>& Eye, 
                const Vec3<Type>& LookAtPt,
                const Vec3<Type>& ViewUp);
    void invLookAt(const Vec3<Type>& Eye, 
                   const Vec3<Type>& LookAtPt, 
                   const Vec3<Type>& ViewUp);
    void Viewport2(int WW, int WH);
    void invViewport2(int WW, int WH);
    void Print() const;
    void CopyInto(Type *Mat) const;

    static void SWAP(Type& a, Type& b) {Type t; t=a;a=b;b=t;}
};

#include "mat44impl.hpp"

typedef Mat44<float> Mat44f;
typedef Mat44<double> Mat44d;

#endif




