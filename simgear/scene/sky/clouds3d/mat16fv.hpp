//------------------------------------------------------------------------------
// File : mat16fv.hpp
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
// mat16fv.hpp : opengl-style float[16] matrix routines.
//----------------------------------------------------------------------------
// $Id$
//============================================================================

float* Copy16fv(float* A, const float* B); // A=B

float* Mult16fv(float* C, const float* A, const float* B); // C=A*B

float* Mult16fv3fv(
  float *NewV, const float* M, const float *V); // NewV = M * [Vx,Vy,Vz,0]

float* Mult16fv3fvPerspDiv(
  float *NewV, const float* M, const float *V); // NewV = M * [Vx,Vy,Vz,1]

float* Mult16fv4fv(
  float *NewV, const float* M, const float *V); // NewV = M * [Vx,Vy,Vz,Vw]

float* Identity16fv(float* M);
float* Transpose16fv(float* M);

float* Rotate16fv(float *M, float DegAng, const float Axis[3]);
float* invRotate16fv(float *M, float DegAng, const float Axis[3]);

float* Scale16fv(float* M, float sx, float sy, float sz);
float* invScale16fv(float* M, float sx, float sy, float sz);

float* Translate16fv(float* M, float tx, float ty, float tz);
float* invTranslate16fv(float* M, float tx, float ty, float tz);

float* LookAt(
  float* M, 
  const float Eye[3], 
  const float LookAtPt[3], 
  const float ViewUp[3]);

float* invLookAt(
  float* M, 
  const float Eye[3],
  const float LookAtPt[3],
  const float ViewUp[3]);

float* Frustum16fv(
  float* M, float l, float r, float b, float t, float n, float f);

float* invFrustum16fv(
  float* M, float l, float r, float b, float t, float n, float f);

float* Perspective(
  float* M, float Yfov, float Aspect, float Ndist, float Fdist);

float* invPerspective(
  float* M, float Yfov, float Aspect, float Ndist, float Fdist);

float* Viewing16fv(
  float* M, 
  const float X[3], const float Y[3], const float Z[3], 
  const float O[3]);

float* invViewing16fv(
  float* M, 
  const float X[3], const float Y[3], const float Z[3], 
  const float O[3]);

void Viewing2CoordFrame16fv(
  const float *M, float X[3], float Y[3], float Z[3], float O[3]);


float* Viewport16fv(float* M, int WW, int WH);

float* invViewport16fv(float* M, int WW, int WH);

float* PlanarReflection16fv(float M[16], const float P[4]);

float XformCoordFrame16fv(
  const float *M, float X[3], float Y[3], float Z[3], float O[3]);

float* Obj2WorldXform16fv(
  float *M, float X[3], float Y[3], float Z[3], float O[3], float Scale);

float* World2ObjXform16fv(
  float *M, 
  const float X[3], 
  const float Y[3], 
  const float Z[3], 
  const float O[3], 
  float Scale);

float* Screen2WorldXform16fv(
  float* M, 
  const float X[3], const float Y[3], const float Z[3], const float O[3],
  float l, float r, float b, float t, float n, float f,
  int WW, int WH);

float* World2ScreenXform16fv(
  float* M,
  const float X[3], const float Y[3], const float Z[3], const float O[3],
  float l, float r, float b, float t, float n, float f,
  int WW, int WH);

void Print16fv(const float* M);
