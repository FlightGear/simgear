//------------------------------------------------------------------------------
// File : minmaxbox.hpp
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
// minmaxbox.hpp : min-max box routines
//=========================================================================

void GetMinMaxBoxVerts(const float Min[3], const float Max[3], float V[8][3]);

int RayMinMaxBoxIsect(const float Start[3], const float Dir[3], 
                      const float Min[3], const float Max[3],
                      float *InT, float *OutT);
int EdgeMinMaxBoxIsect(const float A[3], const float B[3], 
                       const float Min[3], const float Max[3],
                       float *InT, float *OutT);

