//-----------------------------------------------------------------------------
// File : vec3f.hpp
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
// Vec3.hpp : 3d vector class template. Works for any integer or real type.
//==========================================================================

#ifndef VEC3_H
#define VEC3_H

#include <stdio.h>
#include <math.h>

/**
 * @class Vec3
 * @brief A templated 3-vector class
 *
 * Everybody in graphics has to write their own basic 3-vector class.
 * And we're no exception.  This one uses templates, so that with one
 * definition of the class you can have a 3-vector of floats or doubles
 * depending on how precise you are, or you can make 3-vectors of ints
 * or unsigned char, handy for representing colors.
 *
 * A couple of typedefs for common instatiations are provided by
 * default: Vec3f, and Vec3d, the float and double versions,
 * respectively.
 *
 */
template<class Type>
class Vec3
{
  public:
    Type x, y, z;  ///< The storage for the three components of the vector

    /// Default constructor.
    /// Note: does \em not initialize x, y, and z!
    Vec3 (void) 
      {};
    /// Three component constructor
    Vec3 (const Type X, const Type Y, const Type Z) 
      { x=X; y=Y; z=Z; };
    /// Copy constructor
    Vec3 (const Vec3& v) 
      { x=v.x; y=v.y; z=v.z; };
    /// Construct from array
    Vec3 (const Type v[3])
      { x=v[0]; y=v[1]; z=v[2]; };
    /// Set from components
    void Set (const Type X, const Type Y, const Type Z)
      { x=X; y=Y; z=Z; }
    /// Set from array
    void Set (const Type v[3])
      { x=v[0]; y=v[1]; z=v[2]; };

    operator Type*()                            /// Type * CONVERSION
      { return (Type *)&x; }
    operator const Type*() const                /// CONST Type * CONVERSION
      { return &x; }

    bool operator == (const Vec3& A) const       /// COMPARISON (==)
      { return (x==A.x && y==A.y && z==A.z); }
    bool operator != (const Vec3& A) const       /// COMPARISON (!=)
      { return (x!=A.x || y!=A.y || z!=A.z); }

    Vec3& operator = (const Vec3& A)            /// ASSIGNMENT (=)
      { x=A.x; y=A.y; z=A.z; 
        return(*this);  };
    Vec3 operator + (const Vec3& A) const       /// ADDITION (+)
      { Vec3 Sum(x+A.x, y+A.y, z+A.z); 
        return(Sum); };
    Vec3 operator - (const Vec3& A) const       /// SUBTRACTION (-)
      { Vec3 Diff(x-A.x, y-A.y, z-A.z);
        return(Diff); };
    Type operator * (const Vec3& A) const       /// DOT-PRODUCT (*)
      { Type DotProd = x*A.x+y*A.y+z*A.z; 
        return(DotProd); };
    Vec3 operator / (const Vec3& A) const       /// CROSS-PRODUCT (/)
      { Vec3 CrossProd(y*A.z-z*A.y, z*A.x-x*A.z, x*A.y-y*A.x);
        return(CrossProd); };
    Vec3 operator ^ (const Vec3& A) const       /// ALSO CROSS-PRODUCT (^)
      { Vec3 CrossProd(y*A.z-z*A.y, z*A.x-x*A.z, x*A.y-y*A.x);
        return(CrossProd); };
    Vec3 operator * (const Type s) const        /// MULTIPLY BY SCALAR V*s (*)
      { Vec3 Scaled(x*s, y*s, z*s); 
        return(Scaled); };
    Vec3 operator / (const Type s) const        /// DIVIDE BY SCALAR (/)
      { Vec3 Scaled(x/s, y/s, z/s);
        return(Scaled); };
    Vec3 operator & (const Vec3& A) const       /// COMPONENT MULTIPLY (&)
      { Vec3 CompMult(x*A.x, y*A.y, z*A.z);
        return(CompMult); }

    friend inline Vec3 operator *(Type s, const Vec3& v)  /// SCALAR MULT s*V
      { return Vec3(v.x*s, v.y*s, v.z*s); }

    Vec3& operator += (const Vec3& A)    /// ACCUMULATED VECTOR ADDITION (+=)
      { x+=A.x; y+=A.y; z+=A.z;
        return *this;}
    Vec3& operator -= (const Vec3& A)    /// ACCUMULATED VECTOR SUBTRACTION (-=)
      { x-=A.x; y-=A.y; z-=A.z; 
        return *this; }
    Vec3& operator *= (const Type s)     /// ACCUMULATED SCALAR MULT (*=)
      { x*=s; y*=s; z*=s; 
        return *this; }
    Vec3& operator /= (const Type s)     /// ACCUMULATED SCALAR DIV (/=)
      { x/=s; y/=s; z/=s; 
        return *this; }
    Vec3& operator &= (const Vec3& A)  /// ACCUMULATED COMPONENT MULTIPLY (&=)
      { x*=A.x; y*=A.y; z*=A.z; return *this; }
    Vec3 operator - (void) const        /// NEGATION (-)
      { Vec3 Negated(-x, -y, -z);
        return(Negated); };

/*
    const Type& operator [] (const int i) const // ALLOWS VECTOR ACCESS AS AN ARRAY.
      { return( (i==0)?x:((i==1)?y:z) ); };
    Type & operator [] (const int i)
      { return( (i==0)?x:((i==1)?y:z) ); };
*/

    Type Length (void) const                     /// LENGTH OF VECTOR
      { return ((Type)sqrt(x*x+y*y+z*z)); };
    Type LengthSqr (void) const                  /// LENGTH OF VECTOR (SQUARED)
      { return (x*x+y*y+z*z); };
    Vec3& Normalize (void)                       /// NORMALIZE VECTOR
      { Type L = Length();                       //  CALCULATE LENGTH
        if (L>0) { x/=L; y/=L; z/=L; }           //  DIV COMPONENTS BY LENGTH
        return *this;
      };                                         

    /// Returns the 'star' matrix for a vector
    /** This is the skew-symmetric matrix \b A such that 
     *  \b A \b v == \p this x \b v 
     * (the cross-product of \p this and \b v), for any vector \b v.
     *  The matrix looks like this given vector (x,y,z):
     *  @verbatim
            | 0  -z   y|
            | z   0  -x|
            |-y   x   0|
        @endverbatim
     * 
     * Return format is just an array in row-major (OpenGL/Fortran) order. 
     * That is [0, -z, y, z, 0, -x, -y, x, 0].
     */
    Type* Star() const {
      Type s[] = ( 0, -z,  y,
                   z,  0, -x,
                  -y,  x,  0);
      return s;
    }

    /// Update \p Min and \p Max to enclose \p this
    /** A very handy routine for working with min-max or axis aligned 
     *  bounding boxes.
     */
    void UpdateMinMax(Vec3 &Min, Vec3 &Max) const
    {
      if (x<Min.x) Min.x=x; else if (x>Max.x) Max.x=x;
      if (y<Min.y) Min.y=y; else if (y>Max.y) Max.y=y;
      if (z<Min.z) Min.z=z; else if (z>Max.z) Max.z=z;
    }

    /// Construct an orthonormal basis from \p this
    /** Compute two unit vectors \p U and \p V that are orthogonal
     * to this vector and to each other.  Note that \p *this need
     * not be a unit vector.
     *
     * The algorithm works as follows:
     * Find smallest component of L (this), zero it, 
     * negate one of the other two and swap them.  Then normalize.
     * Ex. if x1 is the smallest, assign (x2,y2,z2):=(0,z1,-y1)
     * Clearly now v1 dot v2 = x1*0 + y1*z1 + z1*-y1 = 0;
     * Zeroing out the smallest means that the magnitude of 
     * the remaining vector will be as  big as possible so that 
     * when we normalize, we are safe from dividing by anything
     * close to zero (unless *this was near 0 magnitude to 
     * begin with, in which case lack of precision can't be avoided)
     */
  void CompleteOrthonormalBasis(Vec3 &U, Vec3 &V) const
    {
      U = *this;
      unsigned char s[3] = {0, 1, 2}; 
      unsigned char tmpa;
      U.x = U.x < 0 ? -U.x : -U.x;
      U.y = U.y < 0 ? -U.y : -U.y;
      U.z = U.z < 0 ? -U.z : -U.z;
      if ( U[0] > U[1] )
      {
        tmpa = s[0];
        s[0] = s[1];
        s[1] = tmpa;
      }
      // xy min in s[0] now.
      if ( U[s[0]] > U[2] ) {
        tmpa = s[2];
        s[2] = s[0];
        s[0] = tmpa;
      }
      // xyz min in s[0] now
      U = *this;
      U[s[0]] = 0;

      // Voila U is now perpendicular to *this
      U.Normalize();

      // And so it's easy to find a v that is too, with cross product.
      V = *this ^ U;
      // Or by removing components projected onto other two...
      // I think the cross product may be cheaper
      // V = something - V * (*this.Normalize() + U);

      V.Normalize(); // wouldn't be necessary if we knew *this were normalized
    }

    /// Dump the vector to \c stdout in a pretty way
    void Print() const
      { printf("(%.3f, %.3f, %.3f)\n",x, y, z); }

    /// This is a handy way to get the zero vector just use Vec3<Type>::ZERO
    static Vec3 ZERO;
};

typedef Vec3<float> Vec3f;
typedef Vec3<double> Vec3d;

template<class Type> Vec3<Type> Vec3<Type>::ZERO = Vec3<Type>(0,0,0);

#endif


