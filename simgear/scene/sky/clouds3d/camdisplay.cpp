//------------------------------------------------------------------------------
// File : camdisplay.cpp
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
// camdisplay.cpp
//============================================================================

#include <GL/glut.h>
#include "camera.hpp"

//----------------------------------------------------------------------------
// OPENGL CAMERA FRUSTUM DRAWING ROUTINES
//----------------------------------------------------------------------------
void Camera::Display() const
{
  // CALC EIGHT CORNERS OF FRUSTUM (NEAR PTS AND FAR PTS)
  Vec3f V[8];
  CalcVerts(V);


  // DRAW THE FRUSTUM IN WIREFRAME
  glBegin(GL_LINE_LOOP);  // TOP FACE
    glVertex3fv(&(V[4].x)); glVertex3fv(&(V[5].x)); 
    glVertex3fv(&(V[1].x)); glVertex3fv(&(V[0].x));
  glEnd();
  glBegin(GL_LINE_LOOP);  // BOTTOM FACE
    glVertex3fv(&(V[3].x)); glVertex3fv(&(V[2].x));
    glVertex3fv(&(V[6].x)); glVertex3fv(&(V[7].x)); 
  glEnd();
  glBegin(GL_LINE_LOOP);  // LEFT FACE
    glVertex3fv(&(V[1].x)); glVertex3fv(&(V[5].x));
    glVertex3fv(&(V[6].x)); glVertex3fv(&(V[2].x)); 
  glEnd();
  glBegin(GL_LINE_LOOP);  // RIGHT FACE
    glVertex3fv(&(V[0].x)); glVertex3fv(&(V[3].x));
    glVertex3fv(&(V[7].x)); glVertex3fv(&(V[4].x)); 
  glEnd();
  glBegin(GL_LINE_LOOP);  // NEAR FACE
    glVertex3fv(&(V[1].x)); glVertex3fv(&(V[2].x));
    glVertex3fv(&(V[3].x)); glVertex3fv(&(V[0].x)); 
  glEnd();
  glBegin(GL_LINE_LOOP);  // FAR FACE
    glVertex3fv(&(V[4].x)); glVertex3fv(&(V[7].x));
    glVertex3fv(&(V[6].x)); glVertex3fv(&(V[5].x)); 
  glEnd();

  // DRAW PROJECTOR LINES FROM EYE TO CORNERS OF VIEWPLANE WINDOW
  glBegin(GL_LINES);
    glVertex3fv(&(Orig.x)); glVertex3fv(&(V[1].x));
    glVertex3fv(&(Orig.x)); glVertex3fv(&(V[2].x));
    glVertex3fv(&(Orig.x)); glVertex3fv(&(V[3].x));
    glVertex3fv(&(Orig.x)); glVertex3fv(&(V[0].x));
  glEnd();

}
void Camera::DisplayInGreen() const
{
  //draws the camera in unlit green lines, then restores the GL state
  glPushAttrib(GL_LIGHTING_BIT);
  glDisable(GL_LIGHTING);
  glPushAttrib(GL_LINE_BIT);
  glLineWidth(1.0);
  glColor3f(0,1,0);

  Display();

  glPopAttrib();
  glPopAttrib();
  
}
