//------------------------------------------------------------------------------
// File : tri.cpp
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
// tri.cpp : triangle defs and intersection routines.
//============================================================================

#include <math.h>

//----------------------------------------------------------------------------
// CHECKS IF 2D POINT P IS IN A 2D TRI ABC.
//----------------------------------------------------------------------------
int PtInTri2D(float Px, float Py, 
              float Ax, float Ay, float Bx, float By, float Cx, float Cy)
{
  float dABx=Bx-Ax, dABy=By-Ay, dBCx=Cx-Bx, dBCy=Cy-By; // "REPEATS"
  if (dABx*dBCy < dABy*dBCx) // CW
  {
    if (dABx*(Py-Ay) >= dABy*(Px-Ax)) return(0);        // ABxAP
    if (dBCx*(Py-By) >= dBCy*(Px-Bx)) return(0);        // BCxBP
    if ((Ax-Cx)*(Py-Cy) >= (Ay-Cy)*(Px-Cx)) return(0);  // CAxCP
  }
  else // CCW
  {
    if (dABx*(Py-Ay) < dABy*(Px-Ax)) return(0);         // ABxAP
    if (dBCx*(Py-By) < dBCy*(Px-Bx)) return(0);         // BCxBP
    if ((Ax-Cx)*(Py-Cy) < (Ay-Cy)*(Px-Cx)) return(0);   // CAxCP
  }
  return(1); // "INSIDE" EACH EDGE'S IN-HALF-SPACE (PT P IS INSIDE TRIANGLE)
};

//----------------------------------------------------------------------------
// CHECKS IF 3D POINT P (ON ABC'S PLANE) IS IN 3D TRI ABC.
// P=PtOnTriPlane, N=PlaneNormal (does not have to be normalized)
//----------------------------------------------------------------------------
int PtInTri3D(const float P[3], float N[3], 
              const float A[3], const float B[3], const float C[3])
{
  #define ABS(x) (((x)<0)?(-(x)):x)  // HANDY UNIVERSAL ABSOLUTE VALUE FUNC

  // DETERMINE LARGEST COMPONENT OF NORMAL (magnitude, since we want the largest projection)
  N[0]=ABS(N[0]); N[1]=ABS(N[1]); N[2]=ABS(N[2]);

  // PROJECT ONTO PLANE WHERE PERPENDICULAR TO LARGEST NORMAL COMPONENT AXIS
  if (N[0]>N[1] && N[0]>N[2])      // X IS LARGEST SO PROJECT ONTO YZ-PLANE
    return( PtInTri2D(P[1],P[2], A[1],A[2], B[1],B[2], C[1],C[2]) );
  else if (N[1]>N[0] && N[1]>N[2]) // Y IS LARGEST SO PROJECT ONTO XZ-PLANE
    return( PtInTri2D(P[0],P[2], A[0],A[2], B[0],B[2], C[0],C[2]) );
  else                         // Z IS LARGEST SO PROJECT ONTO XY-PLANE
    return( PtInTri2D(P[0],P[1], A[0],A[1], B[0],B[1], C[0],C[1]) );
}

//--------------------------------------------------------------------------
// Checks if an edge UV intersects a triangle ABC. Returns whether or
// not the edge intersects the plane (0 or 1) and the IsectPt.
//--------------------------------------------------------------------------
int EdgeTriIsect(const float U[3], const float V[3],
                 const float A[3], const float B[3], const float C[3],
                 float IsectPt[3])
{
  // CALCULATE PLANE EQUATION COEFFICIENTS P FOR TRI ABC
  float P[4];
  float u[3] = {B[0]-A[0],B[1]-A[1],B[2]-A[2]};
  float v[3] = {C[0]-A[0],C[1]-A[1],C[2]-A[2]};
  P[0] = u[1]*v[2] - u[2]*v[1];  // CROSS-PROD BETWEEN u AND v
  P[1] = u[2]*v[0] - u[0]*v[2];  // DEFINES UNNORMALIZED NORMAL
  P[2] = u[0]*v[1] - u[1]*v[0];
  float l = (float)sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);  // NORMALIZE NORMAL
  P[0]/=l; P[1]/=l; P[2]/=l; 
  P[3] = -(P[0]*A[0] + P[1]*A[1] + P[2]*A[2]);

  // FIND INTERSECTION OF EDGE UV WITH PLANE P
  int EdgeIsectsPlane=0;
  float Dir[3] = { V[0]-U[0], V[1]-U[1], V[2]-U[2] };
  float NdotDir = P[0]*Dir[0] + P[1]*Dir[1] + P[2]*Dir[2];
  float NdotU = P[0]*U[0] + P[1]*U[1] + P[2]*U[2];
  if (NdotDir==(float)0) return(0);
  float t = (-P[3] - NdotU) / NdotDir;
  if (t>=0 && t<=1)
  {
    IsectPt[0] = U[0] + Dir[0]*t;
    IsectPt[1] = U[1] + Dir[1]*t;
    IsectPt[2] = U[2] + Dir[2]*t;
    EdgeIsectsPlane=1;
  }

  // FIRST, DOES THE EDGE INTERSECT THE PLANE?
  if (!EdgeIsectsPlane) return(0);

  // SEE IF ISECT PT IS IN THE TRI
  return( PtInTri3D(IsectPt,P,A,B,C) );
}
