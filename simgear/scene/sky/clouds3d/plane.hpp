//------------------------------------------------------------------------------
// File : plane.hpp
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
// plane.hpp
//=========================================================================

// PLANE EQUATION COEFFICIENT GENERATION FUNCTIONS
float* PlaneEquation(float P[4], 
                     const float p0[3], const float p1[3], const float p2[3]);
float* PlaneEquation(float P[4], const float N[3], float Dist);
float* PlaneEquation(float P[4], const float N[3], const float pt[3]);

int PlanePtOutTest(const float P[4], const float Pt[3]);
int PlanePtInOutTest(const float P[4], const float Pt[3]);
float PlaneDistToPt(const float P[4], const float Pt[3]);

int PlaneRayIsect(const float P[4], const float Start[3], const float Dir[3]);
int PlaneRayIsect(const float P[4], const float Start[3], const float Dir[3], 
                  float IsectPt[3]);
int ZPlaneLineIsect(float d, const float Start[3], const float Dir[3], 
                    float IsectPt[3]);
int PlaneEdgeIsect(const float P[4], const float A[3], const float B[3]);
int PlaneEdgeIsect(const float P[4], const float A[3], const float B[3],
                   float IsectPt[3]);

void XformPlane(const float M[16], float P[4]);

int PlaneMinMaxBoxOverlap(const float P[4], const float m[3], 
                          const float M[3]);

int PlanesRayIsect(const float Planes[][4], int NumPlanes,
                   const float Start[3], const float Dir[3], 
                   float *InT, float *OutT);
int PlanesEdgeIsect(const float Planes[][4], int NumPlanes,
                    const float A[3], const float B[3], 
                    float *InT, float *OutT);
