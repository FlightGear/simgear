//------------------------------------------------------------------------------
// File : camutils.cpp
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
// camutils.cpp : a set of camera utility functions
//============================================================================

#include "camutils.hpp"
#include <plane.hpp>
#include <tri.hpp>
#include <minmaxbox.hpp>

//----------------------------------------------------------------------------
// Given a camera's 8 corner vertices and its 6 side planes and a triangle ABC, 
// return whether or not the tri overlaps the camera's frustum.
//----------------------------------------------------------------------------
int CamTriOverlap(const Vec3f V[8], const float P[][4], 
                  const Vec3f& A, const Vec3f& B, const Vec3f& C)
{
  // TEST TRIANGLE AGAINST ALL PLANES OF CAMERA, FOR EACH VERTEX CLASSIFY
  // AS INSIDE OR OUTSIDE OF PLANE (BY SETTING APPROPRIATE BIT IN BITMASK)
  // FOR EACH VERTEX, WE HAVE A BITMASK INDICATING WHETHER THE VERTEX
  // IS IN OR OUT OF EACH PLANE (LS 6-BITS, SET MEANS "OUT")
  unsigned int BitMaskA=0, BitMaskB=0, BitMaskC=0;
  unsigned int PlaneBitMask=1;  // CURRENT PLANE BEING CHECKED
  int i;
  for (i=0; i<6; i++)
  {
    if ( PlanePtOutTest(P[i], &(A.x)) ) BitMaskA |= PlaneBitMask;
    if ( PlanePtOutTest(P[i], &(B.x)) ) BitMaskB |= PlaneBitMask;
    if ( PlanePtOutTest(P[i], &(C.x)) ) BitMaskC |= PlaneBitMask;
    PlaneBitMask<<=1;
  }

  // TRIVIAL ACCEPTANCE: IF ANY VERTEX IS COMPLETELY INSIDE ALL PLANES (=0)
  if (BitMaskA==0 || BitMaskB==0 || BitMaskC==0) return(1);

  // TRIVIAL REJECTION: IF ALL VERTICES ARE OUTSIDE OF ANY PLANE
  PlaneBitMask=1;
  for (i=0; i<6; i++)
  {
    if ((BitMaskA & BitMaskB & BitMaskC & PlaneBitMask) > 0) return(0);
    PlaneBitMask<<=1;
  }

  // TEST EDGES OF TRIANGLE AGAINST PLANES OF CAMERA
  float InT, OutT;
  if ( PlanesEdgeIsect(P,6,&(A.x),&(B.x),&InT,&OutT) ) return(1);
  if ( PlanesEdgeIsect(P,6,&(B.x),&(C.x),&InT,&OutT) ) return(1);
  if ( PlanesEdgeIsect(P,6,&(C.x),&(A.x),&InT,&OutT) ) return(1);

  // TEST EDGES OF CAMERA AGAINST TRIANGLE
  float IsectPt[3];
  if ( EdgeTriIsect(&(V[0].x),&(V[4].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[1].x),&(V[5].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[2].x),&(V[6].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[3].x),&(V[7].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[1].x),&(V[0].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[5].x),&(V[4].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[6].x),&(V[7].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[2].x),&(V[3].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[4].x),&(V[7].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[5].x),&(V[6].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[1].x),&(V[2].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);
  if ( EdgeTriIsect(&(V[0].x),&(V[3].x),&(A.x),&(B.x),&(C.x),IsectPt) ) return(1);

  return(0);
} 

int CamTriOverlap(const Camera *Cam, 
                  const Vec3f& A, const Vec3f& B, const Vec3f& C)
{
  // GET CAMERA VERTICES
  Vec3f V[8];
  Cam->CalcVerts(V);

  // CALCULATE SIX CAMERA PLANES FROM CAMERA VERTICES
  float P[6][4];
  CalcCamPlanes(V,P);

  return( CamTriOverlap(V,P,A,B,C) );
} 

//----------------------------------------------------------------------------
// Temporary implementation of CamQuadOverlap (uses CamTriOverlap).
//----------------------------------------------------------------------------
int CamQuadOverlap(
  const Camera *Cam, 
  const Vec3f& A, const Vec3f& B, const Vec3f& C, const Vec3f& D)
{
  if ( CamTriOverlap(Cam,A,B,C) ) return(1);
  if ( CamTriOverlap(Cam,C,D,A) ) return(1);
  return(0);
}

int CamQuadOverlap(
  const Camera *Cam, 
  const float A[3], const float B[3], const float C[3], const float D[3])
{
  Vec3f a(A),b(B),c(C),d(D);
  return( CamQuadOverlap(Cam,a,b,c,d) );
}

//----------------------------------------------------------------------------
// Calculate the six planes for a camera. User must have prealloced an array
// of 24 floats (6 planes * 4 coeffs each). Two version: 1 that calculates
// the camera vertices, requires the cam verts to be precomputed
//----------------------------------------------------------------------------
void CalcCamPlanes(const Vec3f *V, float P[][4])
{
  PlaneEquation(P[0], &(V[2].x),&(V[5].x),&(V[6].x)); // LEFT
  PlaneEquation(P[1], &(V[0].x),&(V[7].x),&(V[4].x)); // RIGHT
  PlaneEquation(P[2], &(V[3].x),&(V[6].x),&(V[7].x)); // BOTTOM
  PlaneEquation(P[3], &(V[1].x),&(V[4].x),&(V[5].x)); // TOP
  PlaneEquation(P[4], &(V[1].x),&(V[2].x),&(V[0].x)); // NEAR
  PlaneEquation(P[5], &(V[4].x),&(V[6].x),&(V[5].x)); // FAR
}

void CalcCamPlanes(const Camera *Cam, float P[][4])
{
  Vec3f V[8];
  Cam->CalcVerts(V);
  CalcCamPlanes(V,P);
}

//--------------------------------------------------------------------------
// Camera view-frustum/MinMaxBox (AABB) overlap test: given the extents of the 
// AABB returns the type of overlap determined (complete out(1), partial (0), 
// complete in (-1)) m and M are the min and max extents of the AABB respectively.
// Version is provided that takes in a precomputed set of camera vertices
// and planes
//--------------------------------------------------------------------------
int CamMinMaxBoxOverlap(
  const Camera *Cam, const Vec3f V1[8], const float cP[][4], 
  const Vec3f& m, const Vec3f& M)
{
  // GO FOR TRIVIAL REJECTION OR ACCEPTANCE USING "FASTER OVERLAP TEST"
  int CompletelyIn=1;    // ASSUME COMPLETELY IN UNTIL ONE COUNTEREXAMPLE
  int R;                 // TEST RETURN VALUE
  for (int i=0; i<6; i++)
  {
    R=PlaneMinMaxBoxOverlap(cP[i],&(m.x),&(M.x));
    if(R==COMPLETEOUT) return(COMPLETEOUT); 
    else if(R==PARTIAL) CompletelyIn=0;
  }

  if (CompletelyIn) return(COMPLETEIN);  // CHECK IF STILL COMPLETELY "IN"

  // TEST IF VIEW-FRUSTUM EDGES PROTRUDE THROUGH AABB
  float InT, OutT;
  if ( EdgeMinMaxBoxIsect(&(V1[0].x),&(V1[4].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[1].x),&(V1[5].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[2].x),&(V1[6].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[3].x),&(V1[7].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[0].x),&(V1[1].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[1].x),&(V1[2].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[2].x),&(V1[3].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[3].x),&(V1[0].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[4].x),&(V1[5].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[5].x),&(V1[6].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[6].x),&(V1[7].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);
  if ( EdgeMinMaxBoxIsect(&(V1[7].x),&(V1[0].x),&(m.x),&(M.x),&InT,&OutT) ) return(PARTIAL);

  // COMPUTE VERTICES OF AABB
  float bV[8][3];
  GetMinMaxBoxVerts(&(m.x),&(M.x),bV);

  // TEST FOR PROTRUSION OF AABB EDGES THROUGH VIEW-FRUSTUM
  if ( PlanesEdgeIsect(cP,6,bV[0],bV[4],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[1],bV[5],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[2],bV[6],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[3],bV[7],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[0],bV[1],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[1],bV[2],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[2],bV[3],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[3],bV[0],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[4],bV[5],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[5],bV[6],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[6],bV[7],&InT,&OutT) ) return(PARTIAL);
  if ( PlanesEdgeIsect(cP,6,bV[7],bV[0],&InT,&OutT) ) return(PARTIAL);

  // VF MUST BE COMPLETELY ENCLOSED SINCE PT IS NOT "OUT "OF ANY AABB PLANE.
  //return(COMPLETEOUT);
	return (PARTIAL);
};

int CamMinMaxBoxOverlap(
  const Camera *Cam, const float m[3], const float M[3])
{
  // GET CAMERA VERTICES
  Vec3f V1[8];
  Cam->CalcVerts(V1);

  // CALCULATE SIX CAMERA PLANES FROM CAMERA VERTICES
  float cP[6][4];
  CalcCamPlanes(V1,cP);

  return( CamMinMaxBoxOverlap(Cam,V1,cP,m,M) );
};

// ---------------------------------------------------------------------- 
// Returns 1 if and only if the specified box is completely culled away.
bool VFC(const Camera *Cam, const float m[3], const float M[3])
{
  return (CamMinMaxBoxOverlap(Cam, m, M) == COMPLETEOUT);
}

//----------------------------------------------------------------------------
// Given the 8 corner vertices and the 6 side planes for two cameras,
// returns the type of overlap (complete out(1), partial (0), complete in (-1))
// with respect to the first camera.
//----------------------------------------------------------------------------
int CamCamOverlap(
  const Vec3f V1[8], const float P1[][4], 
  const Vec3f V2[8], const float P2[][4])
{
  int i, NumVertsOutAllPlanes, NumVertsOutOnePlane;
  float InT, OutT;

  // TEST ALL CAM1 VERTICES AGAINST PLANES OF CAM2
  NumVertsOutAllPlanes=0;
  for (i=0; i<6; i++)
  {
    NumVertsOutOnePlane=0;
    for (int j=0; j<8; j++)
      NumVertsOutOnePlane += PlanePtOutTest(P2[i],&(V1[i].x));
    if (NumVertsOutOnePlane==8) return(1);  // TRIVIAL REJECT, COMPLETELY OUT!
    NumVertsOutAllPlanes+=NumVertsOutOnePlane;
  }
  if (NumVertsOutAllPlanes==0) return(0); // TRIVIAL ACCEPT, PARTIAL!

  // TEST ALL CAM2 VERTICES AGAINST PLANES OF CAM1
  NumVertsOutAllPlanes=0;
  for (i=0; i<6; i++)
  {
    NumVertsOutOnePlane=0;
    for (int j=0; j<8; j++)
      NumVertsOutOnePlane += PlanePtOutTest(P1[i],&(V2[i].x));
    if (NumVertsOutOnePlane==8) return(1);  // TRIVIAL REJECT, COMPLETELY OUT!
    NumVertsOutAllPlanes+=NumVertsOutOnePlane;
  }
  if (NumVertsOutAllPlanes==0) return(-1); // TRIVIAL ACCEPT, COMPLETELY IN!

  // TEST ALL CAM1 EDGES AGAINST SET OF CAM2 PLANES
  if ( PlanesEdgeIsect(P2,6,&(V1[0].x),&(V1[4].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[1].x),&(V1[5].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[2].x),&(V1[6].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[3].x),&(V1[7].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[0].x),&(V1[1].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[1].x),&(V1[2].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[2].x),&(V1[3].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[3].x),&(V1[0].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[4].x),&(V1[5].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[5].x),&(V1[6].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[6].x),&(V1[7].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P2,6,&(V1[7].x),&(V1[0].x),&InT,&OutT) ) return(0);

  // TEST ALL CAM2 EDGES AGAINST SET OF CAM1 PLANES
  if ( PlanesEdgeIsect(P1,6,&(V2[0].x),&(V2[4].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[1].x),&(V2[5].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[2].x),&(V2[6].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[3].x),&(V2[7].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[0].x),&(V2[1].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[1].x),&(V2[2].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[2].x),&(V2[3].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[3].x),&(V2[0].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[4].x),&(V2[5].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[5].x),&(V2[6].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[6].x),&(V2[7].x),&InT,&OutT) ) return(0);
  if ( PlanesEdgeIsect(P1,6,&(V2[7].x),&(V2[0].x),&InT,&OutT) ) return(0);

  return(1);
} 

int CamCamOverlap(const Camera *Cam1, const Camera *Cam2)
{
  // GET CAMERA VERTICES
  Vec3f V1[8], V2[8];
  Cam1->CalcVerts(V1);
  Cam2->CalcVerts(V2);

  // CALCULATE SIX CAMERA PLANES FROM CAMERA VERTICES
  float P1[6][4], P2[6][4];
  CalcCamPlanes(V1,P1);
  CalcCamPlanes(V2,P2);

  return( CamCamOverlap(V1,P1,V2,P2) );
}

//--------------------------------------------------------------------------
// Given a camera and a plane (defined by four coefficient of implicit
// form: Ax+By+Cz+D=0), reflect the camera about the plane and invert
// back into right-handed system (reflection inverts the space, so we
// have to invert the X-axis and flip the window boundaries).
//--------------------------------------------------------------------------
void CamReflectAboutPlane(Camera *Cam, const float Plane[4])
{
  // CREATE PLANAR REFLECTION MATRIX
  float M[16];
  PlanarReflection16fv(M,Plane);

  // XFORM CAMERA
  Cam->Xform(M);

  // RESULTING CAM IS LEFT-HANDED, FLIP THE X-AXIS, FLIP X WINDOW BOUNDS
  Cam->X = -(Cam->X);
  float t = -(Cam->wL);
  Cam->wL = -(Cam->wR);
  Cam->wR = t;
}
