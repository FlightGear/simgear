//-----------------------------------------------------------------------------
// File : vec4f.hpp
//-----------------------------------------------------------------------------
// GLVU : Copyright 1997 - 2002 
//        The University of North Carolina at Chapel Hill
//-----------------------------------------------------------------------------
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without
// fee, provided that the above copyright notice appear in all copies
// and that both that copyright notice and this permission notice
// appear in supporting documentation.  Binaries may be compiled with
// this software without any royalties or restrictions.
//
// The University of North Carolina at Chapel Hill makes no representations 
// about the suitability of this software for any purpose. It is provided 
// "as is" without express or implied warranty.

//==========================================================================
// vec4.hpp : 4d vector class template. Works for any integer or real type.
//==========================================================================

#ifndef VEC4_H
#define VEC4_H

#include <stdio.h>
#include <math.h>

template <class Type>
class Vec4
{
  public:
    Type x, y, z, w;

    Vec4 (void) 
      {};
    Vec4 (const Type X, const Type Y, const Type Z, const Type W) 
      { x=X; y=Y; z=Z; w=W; };
    Vec4 (const Vec4& v) 
      { x=v.x; y=v.y; z=v.z; w=v.w; };
    Vec4 (const Type v[4])
      { x=v[0]; y=v[1]; z=v[2]; w=v[3]; };
    void Set (const Type X, const Type Y, const Type Z, const Type W)
      { x=X; y=Y; z=Z; w=W; }
    void Set (const Type v[4])
      { x=v[0]; y=v[1]; z=v[2]; w=v[3]; };

    operator Type*()                             // Type * CONVERSION
      { return (Type *)&x; }
    operator const Type*() const                 // CONST Type * CONVERSION
      { return &x; }

    Vec4& operator = (const Vec4& A)            // ASSIGNMENT (=)
      { x=A.x; y=A.y; z=A.z; w=A.w; 
        return(*this);  };

    bool operator == (const Vec4& A) const        // COMPARISON (==)
      { return (x==A.x && y==A.y && 
        z==A.z && w==A.w); }
    bool operator != (const Vec4& A) const        // COMPARISON (!=)
      { return (x!=A.x || y!=A.y || 
        z!=A.z || w!=A.w); }

    Vec4 operator + (const Vec4& A) const       // ADDITION (+)
      { Vec4 Sum(x+A.x, y+A.y, z+A.z, w+A.w); 
        return(Sum); };
    Vec4 operator - (const Vec4& A) const       // SUBTRACTION (-)
      { Vec4 Diff(x-A.x, y-A.y, z-A.z, w-A.w);
        return(Diff); };
    Type operator * (const Vec4& A) const       // DOT-PRODUCT (*)
      { Type DotProd = x*A.x+y*A.y+z*A.z+w*A.w; 
        return(DotProd); };
    Vec4 operator * (const Type s) const        // MULTIPLY BY SCALAR (*)
      { Vec4 Scaled(x*s, y*s, z*s, w*s); 
        return(Scaled); };
    Vec4 operator / (const Type s) const        // DIVIDE BY SCALAR (/)
      { Vec4 Scaled(x/s, y/s, z/s, w/s);
        return(Scaled); };
    Vec4 operator & (const Vec4& A) const       // COMPONENT MULTIPLY (&)
      { Vec4 CompMult(x*A.x, y*A.y, z*A.z, w*A.w);
        return(CompMult); }

    friend inline Vec4 operator *(Type s, const Vec4& v)  // SCALAR MULT s*V
      { return Vec4(v.x*s, v.y*s, v.z*s, v.w*s); }

    Vec4& operator += (const Vec4& A)      // ACCUMULATED VECTOR ADDITION (+=)
      { x+=A.x; y+=A.y; z+=A.z; w+=A.w; 
        return *this; }
    Vec4& operator -= (const Vec4& A)      // ACCUMULATED VECTOR SUBTRCT (-=)
      { x-=A.x; y-=A.y; z-=A.z; w-=A.w;
        return *this; }
    Vec4& operator *= (const Type s)       // ACCUMULATED SCALAR MULT (*=)
      { x*=s; y*=s; z*=s; w*=s; 
        return *this; }
    Vec4& operator /= (const Type s)       // ACCUMULATED SCALAR DIV (/=)
      { x/=s; y/=s; z/=s; w/=s;
        return *this; }
    Vec4& operator &= (const Vec4& A)  // ACCUMULATED COMPONENT MULTIPLY (&=)
      { x*=A.x; y*=A.y; z*=A.z; w*=A.w; return *this; }
    Vec4 operator - (void) const          // NEGATION (-)
      { Vec4 Negated(-x, -y, -z, -w);
        return(Negated); };

/*
    const Type& operator [] (const int i) const // ALLOWS VECTOR ACCESS AS AN ARRAY.
      { return( (i==0)?x:((i==1)?y:((i==2)?z:w)) ); };
    Type & operator [] (const int i)
      { return( (i==0)?x:((i==1)?y:((i==2)?z:w)) ); };
*/

    Type Length (void) const                     // LENGTH OF VECTOR
      { return ((Type)sqrt(x*x+y*y+z*z+w*w)); };
    Type LengthSqr (void) const                  // LENGTH OF VECTOR (SQUARED)
      { return (x*x+y*y+z*z+w*w); };
    Vec4& Normalize (void)                       // NORMALIZE VECTOR
      { Type L = Length();                       // CALCULATE LENGTH
        if (L>0) { x/=L; y/=L; z/=L; w/=L; }
        return *this;
      };                                          // DIV COMPONENTS BY LENGTH

    void Wdiv(void)
      { x/=w; y/=w; z/=w; w=1; }

    void Print() const
      { printf("(%.3f, %.3f, %.3f, %.3f)\n",x, y, z, w); }

    static Vec4 ZERO;
};

typedef Vec4<float> Vec4f;
typedef Vec4<double> Vec4d;

template<class Type> Vec4<Type> Vec4<Type>::ZERO = Vec4<Type>(0,0,0,0);

#endif


