//------------------------------------------------------------------------------
// File : minmaxbox.cpp
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
// minmaxbox.cpp : min-max box routines
//=========================================================================

//-----------------------------------------------------------------------------
// Calculates the eight corner vertices of the MinMaxBox.
// V must be prealloced.
//       5---4
//      /   /|
//     1---0 |   VERTS : 0=RTN,1=LTN,2=LBN,3=RBN,4=RTF,5=LTF,6=LBF,7=RBF
//     |   | 7   (L,R, B,T, N,F) = (Left,Right, Bottom,Top, Near,Far)
//     |   |/
//     2---3
//-----------------------------------------------------------------------------
void GetMinMaxBoxVerts(const float Min[3], const float Max[3], float V[8][3])
{
  #define SET(v,x,y,z) v[0]=x; v[1]=y; v[2]=z;
  SET(V[0],Max[0],Max[1],Max[2]); SET(V[4],Max[0],Max[1],Min[2]);
  SET(V[1],Min[0],Max[1],Max[2]); SET(V[5],Min[0],Max[1],Min[2]);
  SET(V[2],Min[0],Min[1],Max[2]); SET(V[6],Min[0],Min[1],Min[2]);
  SET(V[3],Max[0],Min[1],Max[2]); SET(V[7],Max[0],Min[1],Min[2]);
}

//--------------------------------------------------------------------------
// Ray-MinMaxBox intersection test. Returns 0 or 1, calcs In-Out "HitTimes"
// IsectPts can be calculated as:
//   InIsectPt = Start + Dir * InT      (if InT>0)
//  OutIsectPt = Start + Dir * OutT     (if OutT>0)
//--------------------------------------------------------------------------
int RayMinMaxBoxIsect(const float Start[3], const float Dir[3], 
                      const float Min[3], const float Max[3],
                      float *InT, float *OutT)
{
  *InT=-99999, *OutT=99999;    // INIT INTERVAL T-VAL ENDPTS TO -/+ "INFINITY"
  float NewInT, NewOutT;     // STORAGE FOR NEW T VALUES

  // X-SLAB (PARALLEL PLANES PERPENDICULAR TO X-AXIS) INTERSECTION (Xaxis is Normal)
  if (Dir[0] == 0)            // CHECK IF RAY IS PARALLEL TO THE SLAB PLANES
  { if ((Start[0] < Min[0]) || (Start[0] > Max[0])) return(0); }
  else
  {
    NewInT = (Min[0]-Start[0])/Dir[0];  // CALC Tval ENTERING MIN PLANE
    NewOutT = (Max[0]-Start[0])/Dir[0]; // CALC Tval ENTERING MAX PLANE
    if (NewOutT>NewInT) { if (NewInT>*InT) *InT=NewInT;  if (NewOutT<*OutT) *OutT=NewOutT; }
                   else { if (NewOutT>*InT) *InT=NewOutT;  if (NewInT<*OutT) *OutT=NewInT; }
    if (*InT>*OutT) return(0);
  }

  // Y-SLAB (PARALLEL PLANES PERPENDICULAR TO Y-AXIS) INTERSECTION (Yaxis is Normal)
  if (Dir[1] == 0)                // CHECK IF RAY IS PARALLEL TO THE SLAB PLANES
  { if ((Start[1] < Min[1]) || (Start[1] > Max[1])) return(0); }
  else
  {
    NewInT = (Min[1]-Start[1])/Dir[1];  // CALC Tval ENTERING MIN PLANE
    NewOutT = (Max[1] - Start[1])/Dir[1]; // CALC Tval ENTERING MAX PLANE
    if (NewOutT>NewInT) { if (NewInT>*InT) *InT=NewInT;  if (NewOutT<*OutT) *OutT=NewOutT; }
                   else { if (NewOutT>*InT) *InT=NewOutT;  if (NewInT<*OutT) *OutT=NewInT; }
    if (*InT>*OutT) return(0);
  }

  // Z-SLAB (PARALLEL PLANES PERPENDICULAR TO Z-AXIS) INTERSECTION (Zaxis is Normal)
  if (Dir[2] == 0)                // CHECK IF RAY IS PARALLEL TO THE SLAB PLANES
  { if ((Start[2] < Min[2]) || (Start[2] > Max[2])) return(0); }
  else
  {
    NewInT = (Min[2]-Start[2])/Dir[2];  // CALC Tval ENTERING MIN PLANE
    NewOutT = (Max[2]-Start[2])/Dir[2]; // CALC Tval ENTERING MAX PLANE
    if (NewOutT>NewInT) { if (NewInT>*InT) *InT=NewInT;  if (NewOutT<*OutT) *OutT=NewOutT; }
                   else { if (NewOutT>*InT) *InT=NewOutT;  if (NewInT<*OutT) *OutT=NewInT; }
    if (*InT>*OutT) return(0);
  }

  // CHECK IF INTERSECTIONS ARE "AT OR BEYOND" THE START OF THE RAY
  if (*InT>=0 || *OutT>=0) return(1);
  return(0);
}

//--------------------------------------------------------------------------
// returns whether or not an edge intersects a MinMaxBox. The hit times
// are returned just as in the ray isect test.
//--------------------------------------------------------------------------
int EdgeMinMaxBoxIsect(const float A[3], const float B[3], 
                       const float Min[3], const float Max[3],
                       float *InT, float *OutT)
{
  float Dir[3] = {B[0]-A[0],B[1]-A[1],B[2]-A[2]};
  if ( RayMinMaxBoxIsect(A,Dir,Min,Max,InT,OutT) )
    if (*InT>=0 && *InT<=1) return(1);
    else if (*OutT>=0 && *OutT<=1) return(1);
  return(0);
}
