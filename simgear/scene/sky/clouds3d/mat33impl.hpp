//------------------------------------------------------------------------------
// File : mat33impl.hpp
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
// mat33impl.hpp : 3x3 matrix template.
// This is the template implementation file. It is included by mat33.hpp.
//============================================================================

//---------------------------------------------------------------------------
// CONSTRUCTORS
//---------------------------------------------------------------------------
template<class Type>
Mat33<Type>::Mat33()
{ 
  Identity(); 
}

template<class Type>
Mat33<Type>::Mat33(const Type *N)
{
  M[0]=N[0]; M[3]=N[3];  M[6]=N[6];
  M[1]=N[1]; M[4]=N[4];  M[7]=N[7];
  M[2]=N[2]; M[5]=N[5];  M[8]=N[8];
}

template<class Type>
Mat33<Type>::Mat33(Type M0, Type M3, Type M6,
                   Type M1, Type M4, Type M7,
                   Type M2, Type M5, Type M8)
      
{
  M[0]=M0; M[3]=M3; M[6]=M6;
  M[1]=M1; M[4]=M4; M[7]=M7;
  M[2]=M2; M[5]=M5; M[8]=M8; 
}

//---------------------------------------------------------------------------
// MATRIX/MATRIX AND MATRIX/VECTOR OPERATORS
//---------------------------------------------------------------------------
template<class Type>
Mat33<Type>& Mat33<Type>::operator = (const Mat33& A)      // ASSIGNMENT (=)
{
  M[0]=A.M[0]; M[3]=A.M[3]; M[6]=A.M[6]; 
  M[1]=A.M[1]; M[4]=A.M[4]; M[7]=A.M[7]; 
  M[2]=A.M[2]; M[5]=A.M[5]; M[8]=A.M[8];
  return(*this);
}

template<class Type>
Mat33<Type>& Mat33<Type>::operator = (const Type* a) {
  for (int i=0;i<9;i++) {
    M[i] = a[i];
  }
  return *this;
}

template<class Type>
Mat33<Type> Mat33<Type>::operator * (const Mat33& A) const  // MULTIPLICATION (*)
{
  Mat33<Type> NewM(
              M[0]*A.M[0] + M[3]*A.M[1] + M[6]*A.M[2],      // ROW 1
              M[0]*A.M[3] + M[3]*A.M[4] + M[6]*A.M[5],
              M[0]*A.M[6] + M[3]*A.M[7] + M[6]*A.M[8],
              
              M[1]*A.M[0] + M[4]*A.M[1] + M[7]*A.M[2],      // ROW 2
              M[1]*A.M[3] + M[4]*A.M[4] + M[7]*A.M[5],
              M[1]*A.M[6] + M[4]*A.M[7] + M[7]*A.M[8],
              
              M[2]*A.M[0] + M[5]*A.M[1] + M[8]*A.M[2],      // ROW 3
              M[2]*A.M[3] + M[5]*A.M[4] + M[8]*A.M[5],
              M[2]*A.M[6] + M[5]*A.M[7] + M[8]*A.M[8]);              
  return(NewM);
}

// MAT-VECTOR MULTIPLICATION (*)
template<class Type>
Vec3<Type> Mat33<Type>::operator * (const Vec3<Type>& V) const
{
  Vec3<Type> NewV;
  NewV.x = M[0]*V.x + M[3]*V.y + M[6]*V.z;
  NewV.y = M[1]*V.x + M[4]*V.y + M[7]*V.z;
  NewV.z = M[2]*V.x + M[5]*V.y + M[8]*V.z;
  return(NewV);
}

// MAT-VECTOR PRE-MULTIPLICATON (*) (non-member)
// interpreted as V^t M  
template<class Type>
Vec3<Type> operator *(const Vec3<Type>& V, const Mat33<Type>& A)
{
  Vec3<Type> NewV;
  NewV.x = A[0]*V.x + A[1]*V.y + A[2]*V.z;
  NewV.y = A[3]*V.x + A[4]*V.y + A[5]*V.z;
  NewV.z = A[6]*V.x + A[7]*V.y + A[8]*V.z;
  return(NewV);
}

// SCALAR POST-MULTIPLICATION
template<class Type>
Mat33<Type> Mat33<Type>::operator * (Type a) const
{
  Mat33<Type> NewM;
  for (int i = 0; i < 9; i++) 
    NewM[i] = M[i]*a;
  return(NewM);
}

// SCALAR PRE-MULTIPLICATION (non-member)
template <class Type>
Mat33<Type> operator * (Type a, const Mat44<Type>& M) 
{
  Mat33<Type> NewM;
  for (int i = 0; i < 9; i++) 
    NewM[i] = a*M[i];
  return(NewM);
}

template <class Type>
Mat33<Type> Mat33<Type>::operator / (Type a) const      // SCALAR DIVISION
{
  Mat33<Type> NewM;
  Type ainv = Type(1.0)/a;
  for (int i = 0; i < 9; i++) 
    NewM[i] = M[i]*ainv;
  return(NewM);
}

template <class Type>
Mat33<Type> Mat33<Type>::operator + (Mat33& N) const    // ADDITION (+)
{
  Mat33<Type> NewM;
  for (int i = 0; i < 9; i++) 
    NewM[i] = M[i]+N.M[i];
  return(NewM);
}
template <class Type>
Mat33<Type>& Mat33<Type>::operator += (Mat33& N)  // ACCUMULATE ADD (+=)
{
  for (int i = 0; i < 9; i++) 
    M[i] += N.M[i];
  return(*this);
}

template <class Type>
Mat33<Type>& Mat33<Type>::operator -= (Mat33& N)  // ACCUMULATE SUB (-=)
{
  for (int i = 0; i < 9; i++) 
    M[i] -= N.M[i];
  return(*this);
}

template <class Type>
Mat33<Type>& Mat33<Type>::operator *= (Type a)   // ACCUMULATE MULTIPLY (*=)
{
  for (int i = 0; i < 9; i++) 
    M[i] *= a;
  return(*this);
}
template <class Type>
Mat33<Type>& Mat33<Type>::operator /= (Type a)   // ACCUMULATE DIVIDE (/=)
{
  Type ainv = Type(1.0)/a;
  for (int i = 0; i < 9; i++) 
    M[i] *= ainv;
  return(*this);
}

template<class Type>
bool Mat33<Type>::Inverse(Mat33<Type> &inv, Type tolerance) const // MATRIX INVERSE
{
  // Invert using cofactors.  

  inv[0] = M[4]*M[8] - M[7]*M[5];
  inv[3] = M[6]*M[5] - M[3]*M[8];
  inv[6] = M[3]*M[7] - M[6]*M[4];
  inv[1] = M[7]*M[2] - M[1]*M[8];
  inv[4] = M[0]*M[8] - M[6]*M[2];
  inv[7] = M[6]*M[1] - M[0]*M[7];
  inv[2] = M[1]*M[5] - M[4]*M[2];
  inv[5] = M[3]*M[2] - M[0]*M[5];
  inv[8] = M[0]*M[4] - M[3]*M[1];

  Type det = M[0]*inv[0] + M[3]*inv[1] + M[6]*inv[2];

  if (fabs(det) <= tolerance) // singular
    return false;

  Type invDet = 1.0f / det;
  for (int i = 0; i < 9; i++) {
    inv[i] *= invDet;
  }

  return true;
}

template<class Type>
Mat33<Type>::operator const Type*() const
{
  return M;
}

template<class Type>
Mat33<Type>::operator Type*()
{
  return M;
}

template<class Type>
void Mat33<Type>::RowMajor(Type m[9])
{
  m[0] = M[0]; m[1] = M[3]; m[2] = M[6];
  m[3] = M[1]; m[4] = M[4]; m[5] = M[7];
  m[6] = M[2]; m[7] = M[5]; m[8] = M[8];
}

template<class Type>
Type& Mat33<Type>::operator()(int col, int row)
{
  return M[3*col+row];
}

template<class Type>
const Type& Mat33<Type>::operator()(int col, int row) const
{
  return M[3*col+row];
}

template<class Type>
void Mat33<Type>::Set(const Type* a)
{
  for (int i=0;i<9;i++) {
    M[i] = a[i];
  }
}

template<class Type>
void Mat33<Type>::Set(Type M0, Type M3, Type M6,
                      Type M1, Type M4, Type M7,
                      Type M2, Type M5, Type M8)
{
  M[0]=M0; M[3]=M3; M[6]=M6;
  M[1]=M1; M[4]=M4; M[7]=M7;
  M[2]=M2; M[5]=M5; M[8]=M8; 
}


//---------------------------------------------------------------------------
// Standard Matrix Operations
//---------------------------------------------------------------------------
template<class Type>
void Mat33<Type>::Identity()
{
  M[0]=M[4]=M[8]=1;
  M[1]=M[2]=M[3]=M[5]=M[6]=M[7]=0;
}
template<class Type>
void Mat33<Type>::Zero()
{
  M[0]=M[1]=M[2]=M[3]=M[4]=M[5]=M[6]=M[7]=M[8]=0;
}

template<class Type>
void Mat33<Type>::Transpose()
{
  SWAP(M[1],M[3]);
  SWAP(M[2],M[6]);
  SWAP(M[5],M[7]);
}

//---------------------------------------------------------------------------
// Standard Matrix Affine Transformations
//---------------------------------------------------------------------------
template<class Type>
void Mat33<Type>::Scale(Type Sx, Type Sy, Type Sz)
{
  M[0]=Sx; M[3]=0;  M[6]=0;
  M[1]=0;  M[4]=Sy; M[7]=0;
  M[2]=0;  M[5]=0;  M[8]=Sz;
}

template<class Type>
void Mat33<Type>::Scale(const Vec3<Type>& S)
{
  M[0]=S.x; M[3]=0;   M[6]=0;  
  M[1]=0;   M[4]=S.y; M[7]=0;  
  M[2]=0;   M[5]=0;   M[8]=S.z;
}

template<class Type>
void Mat33<Type>::invScale(Type Sx, Type Sy, Type Sz)
{
  M[0]=1/Sx; M[3]=0;    M[6]=0;    
  M[1]=0;    M[4]=1/Sy; M[7]=0;    
  M[2]=0;    M[5]=0;    M[8]=1/Sz;
}

template<class Type>
void Mat33<Type>::invScale(const Vec3<Type>& S)
{
  M[0]=1/S.x; M[3]=0;     M[6]=0;     
  M[1]=0;     M[4]=1/S.y; M[7]=0;     
  M[2]=0;     M[5]=0;     M[8]=1/S.z;
}

template<class Type>
void Mat33<Type>::Rotate(Type DegAng, const Vec3<Type>& Axis)
{
  Type RadAng = DegAng*Mat33TORADS;
  Type ca=(Type)cos(RadAng),
        sa=(Type)sin(RadAng);
  if (Axis.x==1 && Axis.y==0 && Axis.z==0)  // ABOUT X-AXIS
  {
   M[0]=1; M[3]=0;  M[6]=0; 
   M[1]=0; M[4]=ca; M[7]=-sa;
   M[2]=0; M[5]=sa; M[8]=ca;
    
  }
  else if (Axis.x==0 && Axis.y==1 && Axis.z==0)  // ABOUT Y-AXIS
  {
   M[0]=ca;  M[3]=0; M[6]=sa; 
   M[1]=0;   M[4]=1; M[7]=0;  
   M[2]=-sa; M[5]=0; M[8]=ca;
  }
  else if (Axis.x==0 && Axis.y==0 && Axis.z==1)  // ABOUT Z-AXIS
  {
   M[0]=ca; M[3]=-sa; M[6]=0;
   M[1]=sa; M[4]=ca;  M[7]=0;
   M[2]=0;  M[5]=0;   M[8]=1;
  }
  else                                      // ARBITRARY AXIS
  {
   Type l = Axis.LengthSqr();
   Type x, y, z;
   x=Axis.x, y=Axis.y, z=Axis.z;
   if (l > Type(1.0001) || l < Type(0.9999) && l!=0)
   {
     // needs normalization
     l=Type(1.0)/sqrt(l);
     x*=l; y*=l; z*=l;
   }
   Type x2=x*x, y2=y*y, z2=z*z;
   M[0]=x2+ca*(1-x2); M[3]=(x*y)+ca*(-x*y)+sa*(-z); M[6]=(x*z)+ca*(-x*z)+sa*y;
   M[1]=(x*y)+ca*(-x*y)+sa*z; M[4]=y2+ca*(1-y2); M[7]=(y*z)+ca*(-y*z)+sa*(-x);
   M[2]=(x*z)+ca*(-x*z)+sa*(-y); M[5]=(y*z)+ca*(-y*z)+sa*x; M[8]=z2+ca*(1-z2);
  }
}

template<class Type>
void Mat33<Type>::invRotate(Type DegAng, const Vec3<Type>& Axis)
{
  Rotate(DegAng,Axis);
  Transpose();
}

template <class Type>
inline void Mat33<Type>::Star(const Vec3<Type>& v)
{
  M[0]=   0; M[3]=-v.z;  M[6]= v.y;
  M[1]= v.z; M[4]=   0;  M[7]=-v.x;
  M[2]=-v.y; M[5]= v.x; M[8]=   0;
}

template <class Type>
inline void Mat33<Type>::OuterProduct(const Vec3<Type>& u, const Vec3<Type>& v)
{
  M[0]=u.x*v.x; M[3]=u.x*v.y;  M[6]=u.x*v.z;
  M[1]=u.y*v.x; M[4]=u.y*v.y;  M[7]=u.y*v.z;
  M[2]=u.z*v.x; M[5]=u.z*v.y; M[8]=u.z*v.z;
}

template<class Type>
inline Type Mat33<Type>::Trace() const
{
  return M[0] + M[4] + M[8];
}

//---------------------------------------------------------------------------
// Handy matrix printing routine.
//---------------------------------------------------------------------------
template<class Type>
void Mat33<Type>::Print() const
{
  printf("\n%f %f %f\n",M[0],M[3],M[6]);
  printf("%f %f %f\n",M[1],M[4],M[7]);
  printf("%f %f %f\n",M[2],M[5],M[8]);
}

//---------------------------------------------------------------------------
// Copy contents of matrix into matrix array.
//---------------------------------------------------------------------------
template<class Type>
void Mat33<Type>::CopyInto(Type *Mat) const
{
  Mat[0]=M[0]; Mat[3]=M[3];  Mat[6]=M[6];  
  Mat[1]=M[1]; Mat[4]=M[4];  Mat[7]=M[7];  
  Mat[2]=M[2]; Mat[5]=M[5]; Mat[8]=M[8]; 
}

