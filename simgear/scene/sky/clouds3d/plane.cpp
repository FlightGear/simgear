//------------------------------------------------------------------------------
// File : plane.cpp
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

//=========================================================================
// plane.cpp
//=========================================================================

#include <math.h>

//--------------------------------------------------------------------------
// Calculates the plane equation coeffients P = [ A B C D ] for the plane
// equation: Ax+By+Cz+D=0 given three vertices defining the plane. The
// normalized plane normal (A,B,C) is defined using the right-hand rule.
//--------------------------------------------------------------------------
float* PlaneEquation(float P[4], 
                     const float p0[3], const float p1[3], const float p2[3])
{
  // CALCULATE PLANE NORMAL - FIRST THREE COEFFICIENTS (A,B,C)
  float u[3], v[3];
  u[0]=p1[0]-p0[0];  u[1]=p1[1]-p0[1];  u[2]=p1[2]-p0[2];
  v[0]=p2[0]-p0[0];  v[1]=p2[1]-p0[1];  v[2]=p2[2]-p0[2];

  P[0] = u[1]*v[2] - u[2]*v[1];  // CROSS-PROD BETWEEN u AND v
  P[1] = u[2]*v[0] - u[0]*v[2];  // DEFINES UNNORMALIZED NORMAL
  P[2] = u[0]*v[1] - u[1]*v[0];

  float l = (float)sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);  // NORMALIZE NORMAL
  P[0]/=l; P[1]/=l; P[2]/=l; 

  // CALCULATE D COEFFICIENT (USING FIRST PT ON PLANE - p0)
  P[3] = -(P[0]*p0[0] + P[1]*p0[1] + P[2]*p0[2]);

  return(P);
}


//--------------------------------------------------------------------------
// Calculates the plane equation coeffients P = [ A B C D ] for the plane
// equation: Ax+By+Cz+D=0 given an unnormalized normal N to the plane and
// the distance along the normal to the plane Dist. The dot product between
// the normalized normal and a point on the plane should equal the distance
// along the normal to the plane (Dist): Ax+By+Cz=Dist, so D=-Dist.
//--------------------------------------------------------------------------
float* PlaneEquation(float P[4], const float N[3], float Dist)
{
  // COPY AND NORMALIZE NORMAL
  P[0]=N[0]; P[1]=N[1]; P[2]=N[2];
  float l = (float)sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);
  P[0]/=l; P[1]/=l; P[2]/=l; 

  // CALCULATE D COEFFICIENT
  P[3]=-Dist;

  return(P);
}


//--------------------------------------------------------------------------
// Calculates the plane equation coeffients P = [ A B C D ] for the plane
// equation: Ax+By+Cz+D=0 given an unnormalized normal N to the plane and
// a point pt on the plane. The dot product between the normalized normal
// and the point should equal the distance along the normal to the plane,
// thus equalling negative D.
//--------------------------------------------------------------------------
float* PlaneEquation(float P[4], const float N[3], const float pt[3])
{
  // COPY AND NORMALIZE NORMAL
  P[0]=N[0]; P[1]=N[1]; P[2]=N[2];
  float l = (float)sqrt(P[0]*P[0] + P[1]*P[1] + P[2]*P[2]);
  P[0]/=l; P[1]/=l; P[2]/=l; 

  // CALCULATE D COEFFICIENT (USING PT ON PLANE)
  P[3] = -(P[0]*pt[0] + P[1]*pt[1] + P[2]*pt[2]);

  return(P);
}

//--------------------------------------------------------------------------
// Transforms a plane by a 4x4 matrix (col-major order, assumes premult/col vect). 
// The given plane coefficients are altered. The normal (A,B,C) is normalized.
// see HOFF tech report: "4x4 matrix transformation of the implicit form of the plane"
//--------------------------------------------------------------------------
void XformPlane(const float M[16], float P[4])
{
  float NewP[4], L2, L;
  NewP[0] = M[0]*P[0] + M[4]*P[1] + M[8]*P[2]; 
  NewP[1] = M[1]*P[0] + M[5]*P[1] + M[9]*P[2]; 
  NewP[2] = M[2]*P[0] + M[6]*P[1] + M[10]*P[2]; 
       L2 = NewP[0]*NewP[0] + NewP[1]*NewP[1] + NewP[2]*NewP[2];
        L = (float)sqrt(L2);
  P[0] = NewP[0] / L;
  P[1] = NewP[1] / L;
  P[2] = NewP[2] / L;
  P[3] = P[3]*L2 - (NewP[0]*M[12] + NewP[1]*M[13] + NewP[2]*M[14]);
}     

//--------------------------------------------------------------------------
// return 1 if point is "outside" (normal side) of plane, 0 if "inside" or on
//--------------------------------------------------------------------------
int PlanePtOutTest(const float P[4], const float Pt[3])
{
  return( (P[0]*Pt[0] + P[1]*Pt[1] + P[2]*Pt[2]) > -P[3] );
}

//--------------------------------------------------------------------------
// returns -1 inside, 0 on, 1 outside (normal side)
//--------------------------------------------------------------------------
int PlanePtInOutTest(const float P[4], const float Pt[3])
{
  float DotProd = P[0]*Pt[0] + P[1]*Pt[1] + P[2]*Pt[2];
  if (DotProd < -P[3]) return(-1);
  if (DotProd > -P[3]) return(1);
  return(0);
}

//--------------------------------------------------------------------------
// Calculates the "signed" distance of a point from a plane (in units of
// the normal length, if not normalized). +Dist is on normal dir side of plane.
//--------------------------------------------------------------------------
float PlaneDistToPt(const float P[4], const float Pt[3])
{ 
  return( P[0]*Pt[0] + P[1]*Pt[1] + P[2]*Pt[2] + P[3] );
}

//--------------------------------------------------------------------------
// returns 1 if ray (Start,Dir) intersects plane, 0 if not
//--------------------------------------------------------------------------
int PlaneRayIsect(const float P[4], const float Start[3], const float Dir[3])
{
  float NdotDir = P[0]*Dir[0] + P[1]*Dir[1] + P[2]*Dir[2];
  float NdotStart = P[0]*Start[0] + P[1]*Start[1] + P[2]*Start[2];
  if (NdotDir==(float)0) return(0);
  float t = (-P[3] - NdotStart) / NdotDir;
  if (t>=0) return(1);
  return(0);
}

//--------------------------------------------------------------------------
// Also computes intersection point, if possible
//--------------------------------------------------------------------------
int PlaneRayIsect(const float P[4], const float Start[3], const float Dir[3],
                  float IsectPt[3])
{
  float NdotDir = P[0]*Dir[0] + P[1]*Dir[1] + P[2]*Dir[2];
  float NdotStart = P[0]*Start[0] + P[1]*Start[1] + P[2]*Start[2];
  if (NdotDir==(float)0) return(0);
  float t = (-P[3] - NdotStart) / NdotDir;
  if (t>=0) 
  {
    IsectPt[0] = Start[0] + Dir[0]*t;
    IsectPt[1] = Start[1] + Dir[1]*t;
    IsectPt[2] = Start[2] + Dir[2]*t;
    return(1);
  }
  return(0);
}

//--------------------------------------------------------------------------
// Intersects a line with a plane with normal (0,0,-1) with distance d
// from the origin.  The line is given by position Start and direction Dir.
//--------------------------------------------------------------------------
int ZPlaneLineIsect(float d, const float Start[3], const float Dir[3],
                    float IsectPt[3])
{
  float NdotDir = -Dir[2];
  if (NdotDir==(float)0) return(0);
  float t = (d + Start[2]) / NdotDir;
  IsectPt[0] = Start[0] + Dir[0]*t;
  IsectPt[1] = Start[1] + Dir[1]*t;
  IsectPt[2] = Start[2] + Dir[2]*t;
  return(1);
}

//--------------------------------------------------------------------------
// returns 1 if edge AB intersects plane, 0 if not
//--------------------------------------------------------------------------
int PlaneEdgeIsect(const float P[4], const float A[3], const float B[3])
{
  float Dir[3] = { B[0]-A[0], B[1]-A[1], B[2]-A[2] };
  float NdotDir = P[0]*Dir[0] + P[1]*Dir[1] + P[2]*Dir[2];
  float NdotA = P[0]*A[0] + P[1]*A[1] + P[2]*A[2];
  if (NdotDir==(float)0) return(0);
  float t = (-P[3] - NdotA) / NdotDir;
  if (t>=0 && t<=1) return(1);
  return(0);
}

//--------------------------------------------------------------------------
// Also computes intersection point, if possible
//--------------------------------------------------------------------------
int PlaneEdgeIsect(const float P[4], const float A[3], const float B[3],
                   float IsectPt[3])
{
  float Dir[3] = { B[0]-A[0], B[1]-A[1], B[2]-A[2] };
  float NdotDir = P[0]*Dir[0] + P[1]*Dir[1] + P[2]*Dir[2];
  float NdotA = P[0]*A[0] + P[1]*A[1] + P[2]*A[2];
  if (NdotDir==(float)0) return(0);
  float t = (-P[3] - NdotA) / NdotDir;
  if (t>=0 && t<=1)
  {
    IsectPt[0] = A[0] + Dir[0]*t;
    IsectPt[1] = A[1] + Dir[1]*t;
    IsectPt[2] = A[2] + Dir[2]*t;
    return(1);
  }
  return(0);
}

//---------------------------------------------------------------------------
// Box (m,M)/ Plane P overlap test.
// returns type of overlap: 1=OUT (side of normal dir), -1=IN, 0=Overlapping
// Finds the "closest" and "farthest" points from the plane with respect
// to the plane normal dir (the "extremes" of the aabb) and tests for overlap.
// m and M are the Min and Max extents of the AABB.
//---------------------------------------------------------------------------
int PlaneMinMaxBoxOverlap(const float P[4], const float m[3], const float M[3])
{
  #define SET(v,x,y,z) v[0]=x; v[1]=y; v[2]=z;

  // CALC EXTREME PTS (Neg,Pos) ALONG NORMAL AXIS (Pos in dir of norm, etc.)
  float Neg[3], Pos[3];
  if(P[0]>0)
    if(P[1]>0) { if(P[2]>0) { SET(Pos,M[0],M[1],M[2]); SET(Neg,m[0],m[1],m[2]); }
                       else { SET(Pos,M[0],M[1],m[2]); SET(Neg,m[0],m[1],M[2]); } }
          else { if(P[2]>0) { SET(Pos,M[0],m[1],M[2]); SET(Neg,m[0],M[1],m[2]); }
                       else { SET(Pos,M[0],m[1],m[2]); SET(Neg,m[0],M[1],M[2]); } }
  else
    if(P[1]>0) { if(P[2]>0) { SET(Pos,m[0],M[1],M[2]); SET(Neg,M[0],m[1],m[2]); }
                       else { SET(Pos,m[0],M[1],m[2]); SET(Neg,M[0],m[1],M[2]); } }
          else { if(P[2]>0) { SET(Pos,m[0],m[1],M[2]); SET(Neg,M[0],M[1],m[2]); }
                       else { SET(Pos,m[0],m[1],m[2]); SET(Neg,M[0],M[1],M[2]); } }

  // CHECK DISTANCE TO PLANE FROM EXTREMAL POINTS TO DETERMINE OVERLAP
  if (PlaneDistToPt(P,Neg) > 0) return(1);
  else if (PlaneDistToPt(P,Pos) <= 0) return(-1);
  return(0);
}

//---------------------------------------------------------------------------
// Edge/Planes intersection test. Returns 0 or 1, calcs In-Out "HitTimes"
// IsectPts can be calculated as:
//   InIsectPt = Start + Dir * InT      (if InT>0)
//  OutIsectPt = Start + Dir * OutT     (if OutT>0)
// Planes is defined as: float Planes[n][4] for n planes.
//---------------------------------------------------------------------------
int PlanesRayIsect(const float Planes[][4], int NumPlanes,
                   const float Start[3], const float Dir[3], 
                   float *InT, float *OutT)
{
  *InT=-99999, *OutT=99999;          // INIT INTERVAL T-VAL ENDPTS TO -/+ INFINITY
  float NdotDir, NdotStart;          // STORAGE FOR REPEATED CALCS NEEDED FOR NewT CALC
  float NewT;
  for (int i=0; i<NumPlanes; i++)    // CHECK INTERSECTION AGAINST EACH VF PLANE
  {
    NdotDir = Planes[i][0]*Dir[0] + Planes[i][1]*Dir[1] + Planes[i][2]*Dir[2];
    NdotStart = Planes[i][0]*Start[0] + Planes[i][1]*Start[1] + Planes[i][2]*Start[2];
    if (NdotDir == 0)                // CHECK IF RAY IS PARALLEL TO THE SLAB PLANES
    {
      if (NdotStart > -Planes[i][3]) return(0); // IF STARTS "OUTSIDE", NO INTERSECTION
    }
    else
    {
      NewT = (-Planes[i][3] - NdotStart) / NdotDir;      // FIND HIT "TIME" (DISTANCE)
      if (NdotDir < 0) { if (NewT > *InT) *InT=NewT; }   // IF "FRONTFACING", MUST BE NEW IN "TIME"
                  else { if (NewT < *OutT) *OutT=NewT; } // IF "BACKFACING", MUST BE NEW OUT "TIME"
    }
    if (*InT > *OutT) return(0);   // CHECK FOR EARLY EXITS (INTERSECTION INTERVAL "OUTSIDE")
  }

  // IF AT LEAST ONE THE Tvals ARE IN THE INTERVAL [0,1] WE HAVE INTERSECTION
  if (*InT>=0 || *OutT>=0) return(1);
  return(0);
}	

//--------------------------------------------------------------------------
// returns whether or not an edge AB intersects a set of planes. The hit 
// times are returned just as in the ray isect test.
//--------------------------------------------------------------------------
int PlanesEdgeIsect(const float Planes[][4], int NumPlanes,
                    const float A[3], const float B[3], 
                    float *InT, float *OutT)
{
  float Dir[3] = {B[0]-A[0],B[1]-A[1],B[2]-A[2]};
  if ( PlanesRayIsect(Planes,NumPlanes,A,Dir,InT,OutT) )
    if (*InT>=0 && *InT<=1) return(1);
    else if (*OutT>=0 && *OutT<=1) return(1);
  return(0);
}
