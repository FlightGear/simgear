//------------------------------------------------------------------------------
// File : vec3fv.hpp
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
// vec3fv.hpp
//============================================================================

void Set3fv(float v[3], float x, float y, float z);
void Copy3fv(float A[3], const float B[3]); // A=B

void ScalarMult3fv(float c[3], const float a[3], float s);
void ScalarDiv3fv(float v[3], float s);
void Add3fv(float c[3], const float a[3], const float b[3]); // c = a + b
void Subtract3fv(float c[3], const float a[3], const float b[3]); // c = a - b
void Negate3fv(float a[3], const float b[3]);  // a = -b

float Length3fv(const float v[3]);
void Normalize3fv(float v[3]);
float DotProd3fv(const float a[3], const float b[3]);
void CrossProd3fv(float* C, const float* A, const float* B); // C = A X B
