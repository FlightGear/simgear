//------------------------------------------------------------------------------
// File : tri.hpp
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
// tri.hpp : triangle defs and intersection routines.
//============================================================================

int PtInTri2D(float Px, float Py, 
              float Ax, float Ay, float Bx, float By, float Cx, float Cy);

int PtInTri3D(const float P[3], float N[3], 
              const float A[3], const float B[3], const float C[3]);

int EdgeTriIsect(const float U[3], const float V[3],
                 const float A[3], const float B[3], const float C[3],
                 float IsectPt[3]);
