//------------------------------------------------------------------------------
// File : camera.hpp
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
// camera.hpp : OPENGL-style camera class definition
//  Defines and stores a "camera" composed of 3 coord axes, an origin for
//  these axes (center-of-projection or eye location), a near and far plane
//  as distances from the origin projected along the viewing direction, 
//  the viewplane window extents (projection window defined in cam coords 
//  on the viewplane - near plane). ViewPlane is defined as the near plane.
//  Viewing direction is always defined along the -Z axis with eye at origin.
//----------------------------------------------------------------------------
// $Id$
//============================================================================

#ifndef CAMERA
#define CAMERA

#include <stdio.h>
#include "vec3f.hpp"
#include "mat16fv.hpp"

class Camera
{
 public:

  Vec3f X, Y, Z;            // NORMALIZED CAMERA COORDINATE-AXIS VECTORS
  Vec3f Orig;               // LOCATION OF ORIGIN OF CAMERA SYSTEM IN WORLD COORDS
  float wL,wR,wB,wT;        // WINDOW EXTENTS DEFINED AS A RECT ON NearPlane
  float Near,Far;           // DISTANCES TO NEAR AND FAR PLANES (IN VIEWING DIR)

  Camera();
  Camera(const Camera &Cam);
  void Copy(const Camera &Cam);

  void LookAt(const Vec3f& Eye, const Vec3f& ViewRefPt, const Vec3f& ViewUp);
  void Perspective(float Yfov, float Aspect, float Ndist, float Fdist);
  void Frustum(float l, float r, float b, float t, float Ndist, float Fdist);

  void TightlyFitToSphere(
    const Vec3f& Eye, const Vec3f& ViewUp, const Vec3f& Cntr, float Rad);

  void GetLookAtParams(Vec3f *Eye, Vec3f *ViewRefPt, Vec3f *ViewUp) const;
  void GetPerspectiveParams(float *Yfov, float *Aspect, 
                            float *Ndist, float *Fdist) const;
  void GetFrustumParams(float *l, float *r, float *b, float *t, 
                        float *Ndist, float *Fdist) const;
  const Vec3f& wCOP() const; // WORLD COORDINATE CENTER-OF-PROJECTION (EYE)
  Vec3f ViewDir() const;     // VIEWING DIRECTION
  Vec3f ViewDirOffAxis() const;

  float* GetXform_Screen2Obj(float* M, int WW, int WH) const;
  float* GetXform_Obj2Screen(float* M, int WW, int WH) const;

  float* GetModelviewMatrix(float* M) const;
  float* GetProjectionMatrix(float* M) const;
  float* GetViewportMatrix(float* M, int WW, int WH) const;

  void SetModelviewMatrix(const float* M);

  float* GetInvModelviewMatrix(float* M) const;
  float* GetInvProjectionMatrix(float* M) const;
  float* GetInvViewportMatrix(float* M, int WW, int WH) const;

  Vec3f WorldToCam(const Vec3f &wP) const;
  float WorldToCamZ(const Vec3f &wP) const;
  Vec3f CamToWorld(const Vec3f &cP) const;

  void LoadIdentityXform();
  void Xform(const float M[16]);

  void Translate(const Vec3f& trans);
  void Rotate(const float M[9]);

  void GetPixelRay(float sx, float sy, int ww, int wh, 
                   Vec3f *Start, Vec3f *Dir) const;

  void WriteToFile(FILE *fp) const;
  int ReadFromFile(FILE *fp);  // RETURNS "1" IF SUCCESSFUL, "0" IF EOF

  void CalcVerts(Vec3f *V) const;    // CALCS EIGHT CORNERS OF VIEW-FRUSTUM
  void Print() const;
  void Display() const;
  void DisplaySolid() const;
  void DisplayInGreen() const;
};

#endif
