//------------------------------------------------------------------------------
// File : camera.cpp
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
// camera.cpp : camera class implementation
//----------------------------------------------------------------------------
// $Id$
//============================================================================
#include "camera.hpp"
#include <iostream.h>

//----------------------------------------------------------------------------
// CONSTRUCTOR: defines a default camera system defined as (45 DEG FOV)
//----------------------------------------------------------------------------
Camera::Camera()
{
  X.Set(1,0,0); Y.Set(0,1,0); Z.Set(0,0,1);
  Orig.Set(0,0,0);
  Near=0.5f; Far=140.0f; wL=-1; wR=1; wT=1; wB=-1;
 
}

Camera::Camera(const Camera &Cam)
{
  Copy(Cam);
}

void Camera::Copy(const Camera &Cam)
{
  X=Cam.X;  Y=Cam.Y;  Z=Cam.Z;  Orig=Cam.Orig;
  Near=Cam.Near;  Far=Cam.Far;
  wL=Cam.wL;  wR=Cam.wR;  wT=Cam.wT;  wB=Cam.wB;
}

//----------------------------------------------------------------------------
// OpenGL CAMERA ORIENTATION ROUTINE (glLookAt)
//----------------------------------------------------------------------------
void Camera::LookAt(
  const Vec3f& Eye, const Vec3f& ViewRefPt, const Vec3f& ViewUp)
{
  Z = Eye-ViewRefPt;  Z.Normalize(); // CALC CAM AXES ("/" IS CROSS-PROD)
  X = ViewUp/Z;       X.Normalize();
  Y = Z/X;            Y.Normalize();
  Orig = Eye;
}

//----------------------------------------------------------------------------
// OpenGL PERSPECTIVE FRUSTUM DEFINITION ROUTINE (gluPerspective)
// Aspect = Width/Height; Yfov in degrees
//----------------------------------------------------------------------------
void Camera::Perspective(float Yfov, float Aspect, float Ndist, float Fdist)
{
  Yfov *= 0.0174532f;  // CONVERT TO RADIANS
  Near=Ndist;  Far=Fdist;
  wT=(float)tan(Yfov*0.5f)*Near;  wB=-wT;
  wR=wT*Aspect; wL=-wR;
}

//----------------------------------------------------------------------------
// OpenGL PERSPECTIVE FRUSTUM DEFINITION ROUTINE (glFrustum). Window extents
// are defined on the viewplane at z=-Ndist.
//----------------------------------------------------------------------------
void Camera::Frustum(float l, float r, float b, float t, float Ndist, float Fdist)
{
  Near=Ndist;  Far=Fdist;
  wR=r;  wL=l;  wB=b;  wT=t;
}

//----------------------------------------------------------------------------
// Completely defines a camera as a tight fitting frustum surrounding the
// given bounding sphere. This is useful when most precision
// is required in pixel and depth resolution (for example, shadow-maps).
// The resulting camera is not skewed!
//----------------------------------------------------------------------------
void Camera::TightlyFitToSphere(
  const Vec3f& Eye, const Vec3f& ViewUp, const Vec3f& Cntr, float Rad)
{
  // FIRST DEFINE COORDINATE FRAME
  LookAt(Eye,Cntr,ViewUp);

  // PROJECTED DIST TO CNTR ALONG VIEWDIR
  float DistToCntr = (Cntr-Orig) * ViewDir();

  // CALC TIGHT-FITTING NEAR AND FAR PLANES
  Near = DistToCntr-Rad;
  Far = DistToCntr+Rad;

  //x = n*R / sqrt(d2 - r2)

  if (Near<=0 || Far<=0) 
    printf("ERROR (Camera::TightlyFitToSphere) Eye is inside the sphere!\n");

  // CALC TIGHT-FITTING SIDES
  wT = (Near * Rad) / (float)sqrt(DistToCntr*DistToCntr - Rad*Rad);//(Near * Rad) / DistToCntr;
  wB = -wT;
  wL = wB;
  wR = wT;
}

//----------------------------------------------------------------------------
// Routines to return the Lookat, Perspective, and Frustum params.
// NOTE: Perspective is designed for non-skewed cameras: the viewing
//       direction must be centered on the viewplane window. Use frustum
//       for off-axis cameras.
//----------------------------------------------------------------------------
void Camera::GetLookAtParams(Vec3f *Eye, Vec3f *ViewRefPt, Vec3f *ViewUp) const
{
  *Eye = Orig;
  *ViewRefPt = Orig - Z;
  *ViewUp = Y;
}

void Camera::GetPerspectiveParams(float *Yfov, float *Aspect, 
                                  float *Ndist, float *Fdist) const
{
  *Yfov = (float)atan(wT/Near) * 57.29578f * 2.0f;  // CONVERT TO DEGREES
  *Aspect = wR/wT;
  *Ndist = Near;
  *Fdist = Far;
}

void Camera::GetFrustumParams(float *l, float *r, float *b, float *t, 
                              float *Ndist, float *Fdist) const
{
  *l = wL;
  *r = wR;
  *b = wB;
  *t = wT;
  *Ndist = Near;
  *Fdist = Far;
}

//----------------------------------------------------------------------------
// RETURNS THE COP OR EYE IN WORLD COORDS (ORIG OF CAMERA SYSTEM)
//----------------------------------------------------------------------------
const Vec3f& Camera::wCOP() const
{
   return( Orig );
}

//----------------------------------------------------------------------------
// RETURNS THE VIEWING DIRECTION
//----------------------------------------------------------------------------
Vec3f Camera::ViewDir() const
{
   return( -Z );
}

Vec3f Camera::ViewDirOffAxis() const
{
  float x=(wL+wR)*0.5f, y=(wT+wB)*0.5f;  // MIDPOINT ON VIEWPLANE WINDOW
  Vec3f ViewDir = X*x + Y*y - Z*Near;
  ViewDir.Normalize();
  return( ViewDir );
}

//----------------------------------------------------------------------------
// Vec3f WORLD-TO-CAM AND CAM-TO-WORLD ROUTINES
//----------------------------------------------------------------------------
Vec3f Camera::WorldToCam(const Vec3f& wP) const
{
  Vec3f sP(wP-Orig);
  Vec3f cP(X*sP,Y*sP,Z*sP);  return(cP);
}

// Return the z-value of the world point in camera space
float Camera::WorldToCamZ(const Vec3f& wP) const
{
  Vec3f sP(wP-Orig);
  float zdist = Z*sP;
  return(zdist);
}


Vec3f Camera::CamToWorld(const Vec3f& cP) const
{
  Vec3f wP(X*cP.x + Y*cP.y + Z*cP.z + Orig);  return(wP);
}

//----------------------------------------------------------------------------
// Makes the camera pose be the identity. World and camera space will be the
// same. Window extents and near/far planes are unaffected.
//----------------------------------------------------------------------------
void Camera::LoadIdentityXform()
{
  X.Set(1,0,0);
  Y.Set(0,1,0);
  Z.Set(0,0,1);
  Orig.Set(0,0,0);
}

//----------------------------------------------------------------------------
// Applies an OpenGL style premult/col vector xform to the coordinate frame.
// Only rotates, translates, and uniform scales are allowed.
// Window extents and near/far planes are affected by scaling.
//----------------------------------------------------------------------------
void Camera::Xform(const float M[16])
{
  X.Set( X.x*M[0] + X.y*M[4] + X.z*M[8],
         X.x*M[1] + X.y*M[5] + X.z*M[9],
         X.x*M[2] + X.y*M[6] + X.z*M[10] );
  Y.Set( Y.x*M[0] + Y.y*M[4] + Y.z*M[8],
         Y.x*M[1] + Y.y*M[5] + Y.z*M[9],
         Y.x*M[2] + Y.y*M[6] + Y.z*M[10] );
  Z.Set( Z.x*M[0] + Z.y*M[4] + Z.z*M[8],
         Z.x*M[1] + Z.y*M[5] + Z.z*M[9],
         Z.x*M[2] + Z.y*M[6] + Z.z*M[10] );
  Orig.Set( Orig.x*M[0] + Orig.y*M[4] + Orig.z*M[8] + M[12],
            Orig.x*M[1] + Orig.y*M[5] + Orig.z*M[9] + M[13],
            Orig.x*M[2] + Orig.y*M[6] + Orig.z*M[10] + M[14] );

  // MUST RENORMALIZE AXES TO FIND THE UNIFORM SCALE
  float Scale = X.Length();
  X /= Scale;
  Y /= Scale;
  Z /= Scale;

  // SCALE THE WINDOW EXTENTS AND THE NEAR/FAR PLANES
  wL*=Scale;
  wR*=Scale;
  wB*=Scale;
  wT*=Scale;
  Near*=Scale;
  Far*=Scale;
};


//----------------------------------------------------------------------------
// Translates the camera by the vector amount trans.
//----------------------------------------------------------------------------
void Camera::Translate(const Vec3f& trans)
{
  Orig += trans;
}


//----------------------------------------------------------------------------
// Translates the camera about its origin by the rotation matrix M.
//----------------------------------------------------------------------------
void Camera::Rotate(const float M[9])
{
  X.Set( X.x*M[0] + X.y*M[3] + X.z*M[6],
         X.x*M[1] + X.y*M[4] + X.z*M[7],
         X.x*M[2] + X.y*M[5] + X.z*M[8] );
  Y.Set( Y.x*M[0] + Y.y*M[3] + Y.z*M[6],
         Y.x*M[1] + Y.y*M[4] + Y.z*M[7],
         Y.x*M[2] + Y.y*M[5] + Y.z*M[8] );
  Z.Set( Z.x*M[0] + Z.y*M[3] + Z.z*M[6],
         Z.x*M[1] + Z.y*M[4] + Z.z*M[7],
         Z.x*M[2] + Z.y*M[5] + Z.z*M[8] );
}


//----------------------------------------------------------------------------
// Returns the COMPOSITE xform matrix that takes a point in the object space
// to a screen space (pixel) point. The inverse is also provided.
// You have to give the pixel dimensions of the viewport window.
//----------------------------------------------------------------------------
float* Camera::GetXform_Screen2Obj(float* M, int WW, int WH) const
{
  Screen2WorldXform16fv(M,&(X.x),&(Y.x),&(Z.x),&(Orig.x),
                        wL,wR,wB,wT,Near,Far,WW,WH);
  return(M);
}

float* Camera::GetXform_Obj2Screen(float* M, int WW, int WH) const
{
  World2ScreenXform16fv(M,&(X.x),&(Y.x),&(Z.x),&(Orig.x),
                        wL,wR,wB,wT,Near,Far,WW,WH);
  return(M);
}

//----------------------------------------------------------------------------
// OPENGL STYLE CAMERA VIEWING MATRIX ROUTINES (PREMULT/COL VECT, 4x4 MATRIX)
// A POINT IN THE WORLD SPACE CAN BE TRANSFORMED TO A PIXEL ON THE CAMERA
// VIEWPLANE BY TRANSFORMING WITH THE COMPOSITE MATRIX: C = V*P*M
// WHERE V IS THE Viewport XFORM, P IS THE Projection XFORM, AND M IS THE
// Modelview XFORM. The associated inverse matrices are also provided.
//----------------------------------------------------------------------------
float* Camera::GetModelviewMatrix(float* M) const
{
  Viewing16fv(M,&(X.x),&(Y.x),&(Z.x),&(Orig.x));
  return(M);
}

float* Camera::GetInvModelviewMatrix(float* M) const
{  
  invViewing16fv(M,&(X.x),&(Y.x),&(Z.x),&(Orig.x));
  return(M);
}

float* Camera::GetProjectionMatrix(float* M) const
{
  Frustum16fv(M,wL,wR,wB,wT,Near,Far);
  return(M);  
}

void Camera::SetModelviewMatrix(const float* M)
{
  Viewing2CoordFrame16fv(M, &(X.x), &(Y.x), &(Z.x), &(Orig.x));
}

float* Camera::GetInvProjectionMatrix(float* M) const
{
  invFrustum16fv(M,wL,wR,wB,wT,Near,Far);
  return(M);  
}

float* Camera::GetViewportMatrix(float* M, int WW, int WH) const
{
  Viewport16fv(M,WW,WH);
  return(M);
}

float* Camera::GetInvViewportMatrix(float* M, int WW, int WH) const
{
  invViewport16fv(M,WW,WH);
  return(M);
}

//----------------------------------------------------------------------------
// Given a screen pixel location (sx,sy) w/ (0,0) at the lower-left and the
// screen dimensions, return the ray (start,dir) of the ray in world coords.
//----------------------------------------------------------------------------
void Camera::GetPixelRay(float sx, float sy, int ww, int wh, 
                         Vec3f *Start, Vec3f *Dir) const
{
  Vec3f wTL = Orig + (X*wL) + (Y*wT) - (Z*Near);  // FIND LOWER-LEFT
  Vec3f dX = (X*(wR-wL))/(float)ww;               // WORLD WIDTH OF PIXEL
  Vec3f dY = (Y*(wT-wB))/(float)wh;               // WORLD HEIGHT OF PIXEL
  wTL += (dX*sx - dY*sy);                         // INCR TO WORLD PIXEL
  wTL += (dX*0.5 - dY*0.5);                       // INCR TO PIXEL CNTR
  *Start = Orig;
  *Dir = wTL-Orig;
}

//----------------------------------------------------------------------------
// READ AND WRITE CAMERA AXES AND ORIGIN TO AND FROM A FILE GIVEN A FILE PTR.
//----------------------------------------------------------------------------
void Camera::WriteToFile(FILE *fp) const
{
 if (fp==NULL) { printf("ERROR WRITING CAM TO FILE!\n"); return; }
 fprintf(fp,"%f %f %f %f %f %f %f %f %f %f %f %f\n",
  X.x,X.y,X.z, Y.x,Y.y,Y.z, Z.x,Z.y,Z.z, Orig.x,Orig.y,Orig.z);
}

int Camera::ReadFromFile(FILE *fp)  // RETURNS "1" IF SUCCESSFUL, "0" IF EOF
{
 int Cond = fscanf(fp,"%f %f %f %f %f %f %f %f %f %f %f %f",
                   &X.x,&X.y,&X.z, &Y.x,&Y.y,&Y.z, 
                   &Z.x,&Z.y,&Z.z, &Orig.x,&Orig.y,&Orig.z);
 return(Cond!=EOF);
}

//----------------------------------------------------------------------------
// 0=RTN,1=LTN,2=LBN,3=RBN,4=RTF,5=LTF,6=LBF,7=RBF
// (Left,Right, Bottom,Top, Near,Far)
// In order, near pts counter-clockwise starting with right-top-near (RTN) pt
// and then far pts ccw starting with right-top-far (RTF) pt
//----------------------------------------------------------------------------
void Camera::CalcVerts(Vec3f *V)  const // MUST BE PREALLOCED : "Vec3f V[8]"
{
  // WINDOW EXTENTS ARE DEFINED ON THE NEAR PLANE, CALC NEAR PTS (IN CAM COORDS)
  float NearZ = -Near;
  V[0].Set(wR,wT,NearZ);
  V[1].Set(wL,wT,NearZ);
  V[2].Set(wL,wB,NearZ);
  V[3].Set(wR,wB,NearZ);

  // CALC FAR PTS (IN CAM COORDS)
  float FarZ=-Far, FN=Far/Near;
  float fwL=wL*FN, fwR=wR*FN, fwB=wB*FN, fwT=wT*FN;
  V[4].Set(fwR,fwT,FarZ);
  V[5].Set(fwL,fwT,FarZ);
  V[6].Set(fwL,fwB,FarZ);
  V[7].Set(fwR,fwB,FarZ);

  // XFORM FRUSTUM IN CAM COORDS TO WORLD SPACE
  for (int i=0; i<8; i++)
    V[i] = CamToWorld(V[i]); 
}

//----------------------------------------------------------------------------
// PRINT ROUTINE
//----------------------------------------------------------------------------
void Camera::Print() const
{
  printf("Camera System Parameters:\n");
  printf("       X: (%.3f, %.3f, %.3f)\n", X.x, X.y, X.z);
  printf("       Y: (%.3f, %.3f, %.3f)\n", Y.x, Y.y, Y.z);
  printf("       Z: (%.3f, %.3f, %.3f)\n", Z.x, Z.y, Z.z);
  printf("  Origin: (%.3f, %.3f, %.3f)\n", Orig.x, Orig.y, Orig.z);
  printf(" NFLRBT: (%.3f, %.3f, %.3f, %.3f, %.3f, %.3f)\n",
               Near,Far,wL,wR,wB,wT);
};
