//------------------------------------------------------------------------------
// File : camutils.hpp
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
// camutils.hpp : a set of camera utility functions
//============================================================================

#include "camera.hpp"

#define COMPLETEOUT 1
#define PARTIAL 0
#define COMPLETEIN -1

int CamTriOverlap(const Vec3f V[8], const float P[][4], 
                  const Vec3f& A, const Vec3f& B, const Vec3f& C);
int CamTriOverlap(const Camera *Cam, 
                  const Vec3f& A, const Vec3f& B, const Vec3f& C);

int CamQuadOverlap(
  const Camera *Cam, 
  const Vec3f& A, const Vec3f& B, const Vec3f& C, const Vec3f& D); 

int CamQuadOverlap(
  const Camera *Cam, 
  const float A[3], const float B[3], const float C[3], const float D[3]);

void CalcCamPlanes(const Vec3f V[8], float P[][4]);

void CalcCamPlanes(const Camera *Cam, float P[][4]);

int CamMinMaxBoxOverlap(const Camera *Cam, 
                        const Vec3f cV[8], const float cP[][4], 
                        const Vec3f& m, const Vec3f& M);

int CamMinMaxBoxOverlap(const Camera *Cam, 
                        const float m[3], const float M[3]);

bool VFC(const Camera *Cam, const float m[3], const float M[3]);

int CamCamOverlap(const Vec3f V1[8], const float P1[][4], 
                  const Vec3f V2[8], const float P2[][4]);
int CamCamOverlap(const Camera *Cam1, const Camera *Cam2);

void CamReflectAboutPlane(Camera *Cam, const float Plane[4]);
