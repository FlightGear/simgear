//------------------------------------------------------------------------------
// File : mat44test.cpp
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

//----------------------------------------------------------------------------
// mat44test  -- test compilation of all the functions in mat44 library
//----------------------------------------------------------------------------
// $Id$
//----------------------------------------------------------------------------
#include <iostream.h>
#include "vec3f.hpp"
#include "mat44.hpp"

int main(int argc, char *argv[])
{
  cout << "This is not a comprehensive correctness test. " << endl
       << "It merely tests whether all the functions in mat44 will compile." <<endl;

  Mat44f m;
  float m2[16];
  Vec3f v(1.0, 1.0, 1.0);
  Vec3f up(0.0, 1.0, 0.0);

  m.Identity();
  m.Transpose();
  m.Translate(1.0, 1.0, 1.0);
  m.Translate(v);
  m.invTranslate(v);
  m.Scale(1.0, 1.0, 1.0);
  m.Scale(v);
  m.invScale(1.0, 1.0, 1.0);
  m.invScale(v);
  m.Rotate(90, v);
  m.invRotate(90, v);
  m.Frustum(-1.0, 1.0, -1.0, 1.0, 0.01, 20);
  m.invFrustum(-1.0, 1.0, -1.0, 1.0, 0.01, 20);
  m.Perspective(60, 1.4, 0.01, 20);
  m.invPerspective(60, 1.4, 0.01, 20);
  m.Viewport(200, 200);
  m.invViewport(200, 200);
  m.LookAt(Vec3f::ZERO, v, up);
  m.invLookAt(Vec3f::ZERO, v, up);
  m.Viewport2(200, 200);
  m.invViewport2(200, 200);
  m.Print();
  m.Transpose();
  m.Print();
  // These two should be transpose of one another
  m.CopyInto(m2);

  cout << "Ok there is this one test to make sure Transpose() is correct.\n"
    "The above two matricies should be the transpose of one another.\n" << endl;
}
