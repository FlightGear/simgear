//------------------------------------------------------------------------------
// File : mat44impl.hpp
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
// This is the template implementation file. It is included by mat44.hpp.
//============================================================================

//---------------------------------------------------------------------------
// CONSTRUCTORS
//---------------------------------------------------------------------------
template<class Type>
Mat44<Type>::Mat44()
{ 
  Identity(); 
}

template<class Type>
Mat44<Type>::Mat44(const Type *N)
{
  M[0]=N[0]; M[4]=N[4];  M[8]=N[8];  M[12]=N[12];
  M[1]=N[1]; M[5]=N[5];  M[9]=N[9];  M[13]=N[13];
  M[2]=N[2]; M[6]=N[6]; M[10]=N[10]; M[14]=N[14];
  M[3]=N[3]; M[7]=N[7]; M[11]=N[11]; M[15]=N[15];
}

template<class Type>
Mat44<Type>::Mat44(Type M0, Type M4, Type M8, Type M12,
      Type M1, Type M5, Type M9, Type M13,
      Type M2, Type M6, Type M10, Type M14,
      Type M3, Type M7, Type M11, Type M15)
{
  M[0]=M0; M[4]=M4;  M[8]=M8;  M[12]=M12;
  M[1]=M1; M[5]=M5;  M[9]=M9;  M[13]=M13;
  M[2]=M2; M[6]=M6; M[10]=M10; M[14]=M14;
  M[3]=M3; M[7]=M7; M[11]=M11; M[15]=M15;
}

//---------------------------------------------------------------------------
// MATRIX/MATRIX AND MATRIX/VECTOR OPERATORS
//---------------------------------------------------------------------------
template<class Type>
Mat44<Type>& Mat44<Type>::operator = (const Mat44& A)      // ASSIGNMENT (=)
{
  M[0]=A.M[0]; M[4]=A.M[4];  M[8]=A.M[8];  M[12]=A.M[12];
  M[1]=A.M[1]; M[5]=A.M[5];  M[9]=A.M[9];  M[13]=A.M[13];
  M[2]=A.M[2]; M[6]=A.M[6]; M[10]=A.M[10]; M[14]=A.M[14];
  M[3]=A.M[3]; M[7]=A.M[7]; M[11]=A.M[11]; M[15]=A.M[15];
  return(*this);
}

template<class Type>
Mat44<Type>& Mat44<Type>::operator = (const Type* a) {
  for (int i=0;i<16;i++) {
    M[i] = a[i];
  }
  return *this;
}

template<class Type>
Mat44<Type> Mat44<Type>::operator * (const Mat44& A) const  // MULTIPLICATION (*)
{
  Mat44<Type> NewM( M[0]*A.M[0] + M[4]*A.M[1] + M[8]*A.M[2] + M[12]*A.M[3],      // ROW 1
              M[0]*A.M[4] + M[4]*A.M[5] + M[8]*A.M[6] + M[12]*A.M[7],
              M[0]*A.M[8] + M[4]*A.M[9] + M[8]*A.M[10] + M[12]*A.M[11],
              M[0]*A.M[12] + M[4]*A.M[13] + M[8]*A.M[14] + M[12]*A.M[15],

              M[1]*A.M[0] + M[5]*A.M[1] + M[9]*A.M[2] + M[13]*A.M[3],      // ROW 2
              M[1]*A.M[4] + M[5]*A.M[5] + M[9]*A.M[6] + M[13]*A.M[7],
              M[1]*A.M[8] + M[5]*A.M[9] + M[9]*A.M[10] + M[13]*A.M[11],
              M[1]*A.M[12] + M[5]*A.M[13] + M[9]*A.M[14] + M[13]*A.M[15],

              M[2]*A.M[0] + M[6]*A.M[1] + M[10]*A.M[2] + M[14]*A.M[3],     // ROW 3
              M[2]*A.M[4] + M[6]*A.M[5] + M[10]*A.M[6] + M[14]*A.M[7],
              M[2]*A.M[8] + M[6]*A.M[9] + M[10]*A.M[10] + M[14]*A.M[11],
              M[2]*A.M[12] + M[6]*A.M[13] + M[10]*A.M[14] + M[14]*A.M[15],

              M[3]*A.M[0] + M[7]*A.M[1] + M[11]*A.M[2] + M[15]*A.M[3],     // ROW 4
              M[3]*A.M[4] + M[7]*A.M[5] + M[11]*A.M[6] + M[15]*A.M[7],
              M[3]*A.M[8] + M[7]*A.M[9] + M[11]*A.M[10] + M[15]*A.M[11],
              M[3]*A.M[12] + M[7]*A.M[13] + M[11]*A.M[14] + M[15]*A.M[15] );
  return(NewM);
}

template<class Type>
Vec3<Type> Mat44<Type>::operator * (const Vec3<Type>& V) const  // MAT-VECTOR MULTIPLICATION (*) W/ PERSP DIV
{
  Type       W = M[3]*V.x + M[7]*V.y + M[11]*V.z + M[15];
  Vec3<Type> NewV( (M[0]*V.x + M[4]*V.y + M[8]*V.z  + M[12]) / W,
              (M[1]*V.x + M[5]*V.y + M[9]*V.z  + M[13]) / W,
              (M[2]*V.x + M[6]*V.y + M[10]*V.z + M[14]) / W );
  return(NewV);
}


// MAT-VECTOR MULTIPLICATION _WITHOUT_ PERSP DIV
// For transforming normals or other pure vectors. 
// Assumes matrix is affine, i.e. bottom row is 0,0,0,1
template<class Type>
Vec3<Type> Mat44<Type>::multNormal(const Vec3<Type>& N) const
{
  Vec3<Type> NewN( (M[0]*N.x + M[4]*N.y + M[8]*N.z ),
                   (M[1]*N.x + M[5]*N.y + M[9]*N.z ),
                   (M[2]*N.x + M[6]*N.y + M[10]*N.z) );
  return (NewN);
}

// MAT-POINT MULTIPLICATION _WITHOUT_ PERSP DIV 
// (for transforming points in space)
// Assumes matrix is affine, i.e. bottom row is 0,0,0,1
template<class Type>
Vec3<Type> Mat44<Type>::multPoint(const Vec3<Type>& P) const
{
  Vec3<Type> NewP( (M[0]*P.x + M[4]*P.y + M[8]*P.z  + M[12]),
                   (M[1]*P.x + M[5]*P.y + M[9]*P.z  + M[13]),
                   (M[2]*P.x + M[6]*P.y + M[10]*P.z + M[14]) );
  return (NewP);
}


template<class Type>
Vec4<Type> Mat44<Type>::operator * (const Vec4<Type>& V) const  // MAT-VECTOR MULTIPLICATION (*)
{
  Vec4<Type> NewV;
  NewV.x = M[0]*V.x + M[4]*V.y + M[8]*V.z  + M[12]*V.w;
  NewV.y = M[1]*V.x + M[5]*V.y + M[9]*V.z  + M[13]*V.w;
  NewV.z = M[2]*V.x + M[6]*V.y + M[10]*V.z + M[14]*V.w;
  NewV.w = M[3]*V.x + M[7]*V.y + M[11]*V.z + M[15]*V.w;
  return(NewV);
}


template<class Type>
Mat44<Type> Mat44<Type>::operator * (Type a) const              // SCALAR POST-MULTIPLICATION
{
  Mat44<Type> NewM( M[0] * a, M[1] * a, M[2] * a, M[3] * a, 
                    M[4] * a, M[5] * a, M[6] * a, M[7] * a,
                    M[8] * a, M[9] * a, M[10] * a, M[11] * a, 
                    M[12] * a, M[13] * a, M[14] * a, M[15] * a);
  return NewM;
}

template<class Type>
Mat44<Type>& Mat44<Type>::operator *= (Type a)                  // SCALAR ACCUMULATE POST-MULTIPLICATION
{
  for (int i = 0; i < 16; i++)
  {
    M[i] *= a;
  }

  return *this;
}

template<class Type>
Mat44<Type>::operator const Type*() const
{
  return M;
}

template<class Type>
Mat44<Type>::operator Type*()
{
  return M;
}

template<class Type>
Type& Mat44<Type>::operator()(int col, int row)
{
  return M[4*col+row];
}

template<class Type>
const Type& Mat44<Type>::operator()(int col, int row) const
{
  return M[4*col+row];
}

template<class Type>
void Mat44<Type>::Set(const Type* a)
{
  for (int i=0;i<16;i++) {
    M[i] = a[i];
  }
  
}

template<class Type>
void Mat44<Type>::Set(Type M0, Type M4, Type M8, Type M12,
                      Type M1, Type M5, Type M9, Type M13,
                      Type M2, Type M6, Type M10, Type M14,
                      Type M3, Type M7, Type M11, Type M15)
{
  M[0]=M0; M[4]=M4;  M[8]=M8;  M[12]=M12;
  M[1]=M1; M[5]=M5;  M[9]=M9;  M[13]=M13;
  M[2]=M2; M[6]=M6; M[10]=M10; M[14]=M14;
  M[3]=M3; M[7]=M7; M[11]=M11; M[15]=M15;
}


//---------------------------------------------------------------------------
// Standard Matrix Operations
//---------------------------------------------------------------------------
template<class Type>
void Mat44<Type>::Identity()
{
  M[0]=M[5]=M[10]=M[15]=1;
  M[1]=M[2]=M[3]=M[4]=M[6]=M[7]=M[8]=M[9]=M[11]=M[12]=M[13]=M[14]=0;
}

template<class Type>
void Mat44<Type>::Transpose()
{
  SWAP(M[1],M[4]);
  SWAP(M[2],M[8]);
  SWAP(M[6],M[9]);
  SWAP(M[3],M[12]);
  SWAP(M[7],M[13]);
  SWAP(M[11],M[14]);
}

//---------------------------------------------------------------------------
// Standard Matrix Affine Transformations
//---------------------------------------------------------------------------
template <class Type>
void Mat44<Type>::Translate(Type Tx, Type Ty, Type Tz)
{
  M[0]=1; M[4]=0;  M[8]=0;  M[12]=Tx;
  M[1]=0; M[5]=1;  M[9]=0;  M[13]=Ty;
  M[2]=0; M[6]=0;  M[10]=1; M[14]=Tz;
  M[3]=0; M[7]=0;  M[11]=0; M[15]=1;
}

template<class Type>
void Mat44<Type>::Translate(const Vec3<Type>& T)
{
  M[0]=1; M[4]=0;  M[8]=0;  M[12]=T.x;
  M[1]=0; M[5]=1;  M[9]=0;  M[13]=T.y;
  M[2]=0; M[6]=0;  M[10]=1; M[14]=T.z;
  M[3]=0; M[7]=0;  M[11]=0; M[15]=1;
}

template<class Type>
void Mat44<Type>::invTranslate(Type Tx, Type Ty, Type Tz)
{
  M[0]=1; M[4]=0;  M[8]=0;  M[12]=-Tx;
  M[1]=0; M[5]=1;  M[9]=0;  M[13]=-Ty;
  M[2]=0; M[6]=0;  M[10]=1; M[14]=-Tz;
  M[3]=0; M[7]=0;  M[11]=0; M[15]=1;
}

template<class Type>
void Mat44<Type>::invTranslate(const Vec3<Type>& T)
{
  M[0]=1; M[4]=0;  M[8]=0;  M[12]=-T.x;
  M[1]=0; M[5]=1;  M[9]=0;  M[13]=-T.y;
  M[2]=0; M[6]=0;  M[10]=1; M[14]=-T.z;
  M[3]=0; M[7]=0;  M[11]=0; M[15]=1;
}

template<class Type>
void Mat44<Type>::Scale(Type Sx, Type Sy, Type Sz)
{
  M[0]=Sx; M[4]=0;  M[8]=0;   M[12]=0;
  M[1]=0;  M[5]=Sy; M[9]=0;   M[13]=0;
  M[2]=0;  M[6]=0;  M[10]=Sz; M[14]=0;
  M[3]=0;  M[7]=0;  M[11]=0;  M[15]=1;
}

template<class Type>
void Mat44<Type>::Scale(const Vec3<Type>& S)
{
  M[0]=S.x; M[4]=0;   M[8]=0;    M[12]=0;
  M[1]=0;   M[5]=S.y; M[9]=0;    M[13]=0;
  M[2]=0;   M[6]=0;   M[10]=S.z; M[14]=0;
  M[3]=0;   M[7]=0;   M[11]=0;   M[15]=1;
}

template<class Type>
void Mat44<Type>::invScale(Type Sx, Type Sy, Type Sz)
{
  M[0]=1/Sx; M[4]=0;    M[8]=0;     M[12]=0;
  M[1]=0;    M[5]=1/Sy; M[9]=0;     M[13]=0;
  M[2]=0;    M[6]=0;    M[10]=1/Sz; M[14]=0;
  M[3]=0;    M[7]=0;    M[11]=0;    M[15]=1;
}

template<class Type>
void Mat44<Type>::invScale(const Vec3<Type>& S)
{
  M[0]=1/S.x; M[4]=0;     M[8]=0;      M[12]=0;
  M[1]=0;     M[5]=1/S.y; M[9]=0;      M[13]=0;
  M[2]=0;     M[6]=0;     M[10]=1/S.z; M[14]=0;
  M[3]=0;     M[7]=0;     M[11]=0;     M[15]=1;
}

template<class Type>
void Mat44<Type>::Rotate(Type DegAng, const Vec3<Type>& Axis)
{
  Type RadAng = DegAng*Mat44TORADS;
  Type ca=(Type)cos(RadAng),
        sa=(Type)sin(RadAng);
  if (Axis.x==1 && Axis.y==0 && Axis.z==0)  // ABOUT X-AXIS
  {
   M[0]=1; M[4]=0;  M[8]=0;   M[12]=0;
   M[1]=0; M[5]=ca; M[9]=-sa; M[13]=0;
   M[2]=0; M[6]=sa; M[10]=ca; M[14]=0;
   M[3]=0; M[7]=0;  M[11]=0;  M[15]=1;
  }
  else if (Axis.x==0 && Axis.y==1 && Axis.z==0)  // ABOUT Y-AXIS
  {
   M[0]=ca;  M[4]=0; M[8]=sa;  M[12]=0;
   M[1]=0;   M[5]=1; M[9]=0;   M[13]=0;
   M[2]=-sa; M[6]=0; M[10]=ca; M[14]=0;
   M[3]=0;   M[7]=0; M[11]=0;  M[15]=1;
  }
  else if (Axis.x==0 && Axis.y==0 && Axis.z==1)  // ABOUT Z-AXIS
  {
   M[0]=ca; M[4]=-sa; M[8]=0;  M[12]=0;
   M[1]=sa; M[5]=ca;  M[9]=0;  M[13]=0;
   M[2]=0;  M[6]=0;   M[10]=1; M[14]=0;
   M[3]=0;  M[7]=0;   M[11]=0; M[15]=1;
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
   M[0]=x2+ca*(1-x2); M[4]=(x*y)+ca*(-x*y)+sa*(-z); M[8]=(x*z)+ca*(-x*z)+sa*y;
   M[1]=(x*y)+ca*(-x*y)+sa*z; M[5]=y2+ca*(1-y2); M[9]=(y*z)+ca*(-y*z)+sa*(-x);
   M[2]=(x*z)+ca*(-x*z)+sa*(-y); M[6]=(y*z)+ca*(-y*z)+sa*x; M[10]=z2+ca*(1-z2);
   M[12]=M[13]=M[14]=M[3]=M[7]=M[11]=0;
   M[15]=1;
  }
}

template<class Type>
void Mat44<Type>::invRotate(Type DegAng, const Vec3<Type>& Axis)
{
  Rotate(DegAng,Axis);
  Transpose();
}

template<class Type>
inline Type Mat44<Type>::Trace() const
{
  return M[0] + M[5] + M[10] + M[15];
}

//---------------------------------------------------------------------------
// Same as glFrustum() : Perspective transformation matrix defined by a
//  truncated pyramid viewing frustum that starts at the origin (eye)
//  going in the -Z axis direction (viewer looks down -Z)
//  with the four pyramid sides passing through the sides of a window
//  defined through x=l, x=r, y=b, y=t on the viewplane at z=-n.
//  The top and bottom of the pyramid are truncated by the near and far
//  planes at z=-n and z=-f. A 4-vector (x,y,z,w) inside this frustum
//  transformed by this matrix will have x,y,z values in the range
//  [-w,w]. Homogeneous clipping is applied to restrict (x,y,z) to
//  this range. Later, a perspective divide by w will result in an NDC
//  coordinate 3-vector of the form (x/w,y/w,z/w,w/w)=(x',y',z',1) where
//  x', y', and z' all are in the range [-1,1]. Perspectively divided z'
//  will be in [-1,1] with -1 being the near plane and +1 being the far.
//---------------------------------------------------------------------------
template<class Type>
void Mat44<Type>::Frustum(Type l, Type r, Type b, Type t, Type n, Type f)
{
  M[0]=(2*n)/(r-l); M[4]=0;           M[8]=(r+l)/(r-l);   M[12]=0;
  M[1]=0;           M[5]=(2*n)/(t-b); M[9]=(t+b)/(t-b);   M[13]=0;
  M[2]=0;           M[6]=0;           M[10]=-(f+n)/(f-n); M[14]=(-2*f*n)/(f-n);
  M[3]=0;           M[7]=0;           M[11]=-1;           M[15]=0;
}

template<class Type>
void Mat44<Type>::invFrustum(Type l, Type r, Type b, Type t, Type n, Type f)
{
  M[0]=(r-l)/(2*n); M[4]=0;           M[8]=0;               M[12]=(r+l)/(2*n);
  M[1]=0;           M[5]=(t-b)/(2*n); M[9]=0;               M[13]=(t+b)/(2*n);
  M[2]=0;           M[6]=0;           M[10]=0;              M[14]=-1;
  M[3]=0;           M[7]=0;           M[11]=-(f-n)/(2*f*n); M[15]=(f+n)/(2*f*n);
}

//---------------------------------------------------------------------------
// Same as gluPerspective : calls Frustum()
//---------------------------------------------------------------------------
template<class Type>
void Mat44<Type>::Perspective(Type Yfov, Type Aspect, Type Ndist, Type Fdist)
{
  Yfov *= 0.0174532f;  // CONVERT TO RADIANS
  Type wT=tanf(Yfov*0.5f)*Ndist, wB=-wT;
  Type wR=wT*Aspect, wL=-wR;
  Frustum(wL,wR,wB,wT,Ndist,Fdist);
}

template<class Type>
void Mat44<Type>::invPerspective(Type Yfov, Type Aspect, Type Ndist, Type Fdist)
{
  Yfov *= 0.0174532f;  // CONVERT TO RADIANS
  Type wT=tanf(Yfov*0.5f)*Ndist, wB=-wT;
  Type wR=wT*Aspect, wL=-wR;
  invFrustum(wL,wR,wB,wT,Ndist,Fdist);
}

//---------------------------------------------------------------------------
// OpenGL VIEWPORT XFORM MATRIX : given Window width and height in pixels
// Transforms the x,y,z NDC values in [-1,1] to (x',y') pixel values and
// normalized z' in [0,1] (near and far respectively).
//---------------------------------------------------------------------------
template<class Type>
void Mat44<Type>::Viewport(int WW, int WH)
{
  Type WW2=(Type)WW*0.5f, WH2=(Type)WH*0.5f;
  M[0]=WW2;  M[4]=0;     M[8]=0;    M[12]=WW2;
  M[1]=0;    M[5]=WH2;   M[9]=0;    M[13]=WH2;
  M[2]=0;    M[6]=0;     M[10]=0.5; M[14]=0.5;
  M[3]=0;    M[7]=0;     M[11]=0;   M[15]=1;
}

template<class Type>
void Mat44<Type>::invViewport(int WW, int WH)
{
  Type WW2=2.0f/(Type)WW, WH2=2.0f/(Type)WH;
  M[0]=WW2;  M[4]=0;     M[8]=0;    M[12]=-1.0;
  M[1]=0;    M[5]=WH2;   M[9]=0;    M[13]=-1.0;
  M[2]=0;    M[6]=0;     M[10]=2.0; M[14]=-1.0;
  M[3]=0;    M[7]=0;     M[11]=0;   M[15]=1;
}

//---------------------------------------------------------------------------
// Same as gluLookAt()
//---------------------------------------------------------------------------
template<class Type>
void Mat44<Type>::LookAt(const Vec3<Type>& Eye, 
                         const Vec3<Type>& LookAtPt, 
                         const Vec3<Type>& ViewUp)
{
  Vec3<Type> Z = Eye-LookAtPt;  Z.Normalize(); // CALC CAM AXES ("/" IS CROSS-PROD)
  Vec3<Type> X = ViewUp/Z;      X.Normalize();
  Vec3<Type> Y = Z/X;           Y.Normalize();
  Vec3<Type> Tr = -Eye;
  M[0]=X.x;  M[4]=X.y;   M[8]=X.z; M[12]=X*Tr;  // TRANS->ROT
  M[1]=Y.x;  M[5]=Y.y;   M[9]=Y.z; M[13]=Y*Tr;
  M[2]=Z.x;  M[6]=Z.y;  M[10]=Z.z; M[14]=Z*Tr;
  M[3]=0;    M[7]=0;    M[11]=0;   M[15]=1;
}

template<class Type>
void Mat44<Type>::invLookAt(const Vec3<Type>& Eye, 
                            const Vec3<Type>& LookAtPt,
                            const Vec3<Type>& ViewUp)
{
  Vec3<Type> Z = Eye-LookAtPt;  Z.Normalize(); // CALC CAM AXES ("/" IS CROSS-PROD)
  Vec3<Type> X = ViewUp/Z;      X.Normalize();
  Vec3<Type> Y = Z/X;           Y.Normalize();
  M[0]=X.x;  M[4]=Y.x;   M[8]=Z.x; M[12]=Eye.x;  // ROT->TRANS
  M[1]=X.y;  M[5]=Y.y;   M[9]=Z.y; M[13]=Eye.y;
  M[2]=X.z;  M[6]=Y.z;  M[10]=Z.z; M[14]=Eye.z;
  M[3]=0;    M[7]=0;    M[11]=0;   M[15]=1;
}

//---------------------------------------------------------------------------
// VIEWPORT TRANFORMATION MATRIX : given Window width and height in pixels
// FROM "JIM BLINN'S CORNER" JULY '91. This function transforms an
// 4-vector in homogeneous space of the form (x,y,z,w) where
// -w<=x,y,z<=w into (x',y',z',1) where (x',y') is the vectors
// projected pixel location in ([0,WW-1],[0,WH-1]) and z' is the
// normalized depth value mapped into [0,1]; 0 is the near plane
// and 1 is the far plane. Mat44VIEWPORT_TOL is introduced to have correct
// mappings. (-1,-1,?,?) in NDC refers to the bottom-left of the
// viewplane window and (0,0,?,?) in screen space refers to the
// bottom-left pixel.
//---------------------------------------------------------------------------
template<class Type>
void Mat44<Type>::Viewport2(int WW, int WH)
{
  Type WW2=(WW-Mat44VIEWPORT_TOL)*0.5f, WH2=(WH-Mat44VIEWPORT_TOL)*0.5f;
  M[0]=WW2;  M[4]=0;     M[8]=0;    M[12]=WW2;
  M[1]=0;    M[5]=WH2;   M[9]=0;    M[13]=WH2;
  M[2]=0;    M[6]=0;     M[10]=0.5; M[14]=0.5;
  M[3]=0;    M[7]=0;     M[11]=0;   M[15]=1;
}

template<class Type>
void Mat44<Type>::invViewport2(int WW, int WH)
{
  Type WW2=2.0f/(WW-Mat44VIEWPORT_TOL), WH2=2.0f/(WH-Mat44VIEWPORT_TOL);
  M[0]=WW2;  M[4]=0;     M[8]=0;    M[12]=-1.0;
  M[1]=0;    M[5]=WH2;   M[9]=0;    M[13]=-1.0;
  M[2]=0;    M[6]=0;     M[10]=2.0; M[14]=-1.0;
  M[3]=0;    M[7]=0;     M[11]=0;   M[15]=1;
}

//---------------------------------------------------------------------------
// Handy matrix printing routine.
//---------------------------------------------------------------------------
template<class Type>
void Mat44<Type>::Print() const
{
  printf("\n%f %f %f %f\n",M[0],M[4],M[8],M[12]);
  printf("%f %f %f %f\n",M[1],M[5],M[9],M[13]);
  printf("%f %f %f %f\n",M[2],M[6],M[10],M[14]);
  printf("%f %f %f %f\n\n",M[3],M[7],M[11],M[15]);
}

//---------------------------------------------------------------------------
// Copy contents of matrix into matrix array.
//---------------------------------------------------------------------------
template<class Type>
void Mat44<Type>::CopyInto(Type *Mat) const
{
  Mat[0]=M[0]; Mat[4]=M[4];  Mat[8]=M[8];  Mat[12]=M[12];
  Mat[1]=M[1]; Mat[5]=M[5];  Mat[9]=M[9];  Mat[13]=M[13];
  Mat[2]=M[2]; Mat[6]=M[6]; Mat[10]=M[10]; Mat[14]=M[14];
  Mat[3]=M[3]; Mat[7]=M[7]; Mat[11]=M[11]; Mat[15]=M[15];
}

