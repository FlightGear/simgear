//-----------------------------------------------------------------------------
// File : vec2f.hpp
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
// vec2.hpp : 2d vector class template. Works for any integer or real type.
//==========================================================================

#ifndef VEC2_H
#define VEC2_H

#include <stdio.h>
#include <math.h>

template<class Type>
class Vec2
{
  public:
    Type x, y;

    Vec2 (void) {};
    Vec2 (const Type X, const Type Y) { x=X; y=Y; };
    Vec2 (const Vec2& v) { x=v.x; y=v.y; };
    Vec2 (const Type v[2]) { x=v[0]; y=v[1]; };
    void Set (const Type X, const Type Y) { x=X; y=Y; }
    void Set (const Type v[2]) { x=v[0]; y=v[1]; };

    operator Type*()                            // Type * CONVERSION
      { return (Type *)&x; }
    operator const Type*() const                // CONST Type * CONVERSION
      { return &x; }

    Vec2& operator = (const Vec2& A)            // ASSIGNMENT (=)
      { x=A.x; y=A.y;
        return(*this);  };

    bool operator == (const Vec2& A) const       // COMPARISON (==)
      { return (x==A.x && y==A.y); }
    bool operator != (const Vec2& A) const       // COMPARISON (!=)
      { return (x!=A.x || y!=A.y); }

    Vec2 operator + (const Vec2& A) const       // ADDITION (+)
      { Vec2 Sum(x+A.x, y+A.y); 
        return(Sum); }
    Vec2 operator - (const Vec2& A) const       // SUBTRACTION (-)
      { Vec2 Diff(x-A.x, y-A.y);
        return(Diff); }
    Type operator * (const Vec2& A) const       // DOT-PRODUCT (*)
      { Type DotProd = x*A.x+y*A.y; 
        return(DotProd); }
    Type operator / (const Vec2& A) const       // CROSS-PRODUCT (/)
      { Type CrossProd = x*A.y-y*A.x;
        return(CrossProd); }
    Type operator ^ (const Vec2& A) const       // ALSO CROSS-PRODUCT (^)
      { Type CrossProd = x*A.y-y*A.x;
        return(CrossProd); }
    Vec2 operator * (const Type s) const        // MULTIPLY BY SCALAR (*)
      { Vec2 Scaled(x*s, y*s); 
        return(Scaled); }
    Vec2 operator / (const Type s) const        // DIVIDE BY SCALAR (/)
      { Vec2 Scaled(x/s, y/s);
        return(Scaled); }
    Vec2 operator & (const Vec2& A) const       // COMPONENT MULTIPLY (&)
      { Vec2 CompMult(x*A.x, y*A.y);
        return(CompMult); }

    friend inline Vec2 operator *(Type s, const Vec2& v)  // SCALAR MULT s*V
      { return Vec2(v.x*s, v.y*s); }

    Vec2& operator += (const Vec2& A)  // ACCUMULATED VECTOR ADDITION (+=)
      { x+=A.x; y+=A.y; return *this; }
    Vec2& operator -= (const Vec2& A)  // ACCUMULATED VECTOR SUBTRACTION (-=)
      { x-=A.x; y-=A.y; return *this; }
    Vec2& operator *= (const Type s)   // ACCUMULATED SCALAR MULT (*=)
      { x*=s; y*=s; return *this; }
    Vec2& operator /= (const Type s)   // ACCUMULATED SCALAR DIV (/=)
      { x/=s; y/=s; return *this; }
    Vec2& operator &= (const Vec2& A)  // ACCUMULATED COMPONENT MULTIPLY (&=)
      { x*=A.x; y*=A.y; return *this; }
    Vec2 operator - (void) const      // NEGATION (-)
      { Vec2 Negated(-x, -y);
        return(Negated); };

/*
    const Type& operator [] (const int i) const // ALLOWS VECTOR ACCESS AS AN ARRAY.
      { return( (i==0)?x:y ); };
    Type & operator [] (const int i)
      { return( (i==0)?x:y ); };
*/

    Type Length (void) const                     // LENGTH OF VECTOR
      { return ((Type)sqrt(x*x+y*y)); };
    Type LengthSqr (void) const                  // LENGTH OF VECTOR (SQUARED)
      { return (x*x+y*y); };
    Vec2& Normalize (void)                       // NORMALIZE VECTOR
      { Type L = Length();                       // CALCULATE LENGTH
        if (L>0) { x/=L; y/=L; }
        return *this;
      };                                          // DIV COMPONENTS BY LENGTH

    Vec2 Perpendicular() const                    // RETURNS A PERPENDICULAR
      { Vec2 Perp(-y,x);
        return(Perp); }

    void UpdateMinMax(Vec2 &Min, Vec2 &Max)
    {
      if (x<Min.x) Min.x=x; else if (x>Max.x) Max.x=x;
      if (y<Min.y) Min.y=y; else if (y>Max.y) Max.y=y;
    }

    void Print() const
      { printf("(%.3f, %.3f)\n",x, y); }

    static Vec2 ZERO;
};

typedef Vec2<float> Vec2f;
typedef Vec2<double> Vec2d;

template<class Type> Vec2<Type> Vec2<Type>::ZERO = Vec2<Type>(0,0);

#endif


