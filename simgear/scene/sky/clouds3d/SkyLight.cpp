//------------------------------------------------------------------------------
// File : SkyLight.cpp
//------------------------------------------------------------------------------
// SkyWorks : Copyright 2002 Mark J. Harris and
//						The University of North Carolina at Chapel Hill
//------------------------------------------------------------------------------
// Permission to use, copy, modify, distribute and sell this software and its 
// documentation for any purpose is hereby granted without fee, provided that 
// the above copyright notice appear in all copies and that both that copyright 
// notice and this permission notice appear in supporting documentation. 
// Binaries may be compiled with this software without any royalties or 
// restrictions. 
//
// The author(s) and The University of North Carolina at Chapel Hill make no 
// representations about the suitability of this software for any purpose. 
// It is provided "as is" without express or implied warranty.
/**
 * @file SkyLight.cpp
 * 
 * Implementation of a class that maintains the state and operation of a light.
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include GLUT_H

#ifdef WIN32
# ifdef _MSC_VER
#  pragma warning( disable : 4786)
# endif
# include "extgl.h"
#endif

#include "SkyLight.hpp"
#include "SkyMaterial.hpp"
#include "mat44.hpp"

SkyMaterial* SkyLight::s_pMaterial = NULL;

//------------------------------------------------------------------------------
// Function     	  : SkyLight::SkyLight
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyLight::SkyLight(SkyLightType eType)
* @brief @todo <WRITE BRIEF SkyLight::SkyLight DOCUMENTATION>
* 
* @todo <WRITE EXTENDED SkyLight::SkyLight FUNCTION DOCUMENTATION>
*/ 
SkyLight::SkyLight(SkyLightType eType)
: _bEnabled(true),
_bDirty(true),
_iLastGLID(-1),
_eType(eType),
_vecPosition(0, 0, 1, 1),
_vecDirection(0, 0, -1, 0),
_vecDiffuse(1, 1, 1, 1),
_vecAmbient(0, 0, 0, 0),
_vecSpecular(1, 1, 1, 1),
_vecAttenuation(1, 0, 0),
_rSpotExponent(0),
_rSpotCutoff(180)  
{
  if (!s_pMaterial)
  {
    s_pMaterial = new SkyMaterial;
    s_pMaterial->SetColorMaterialMode(GL_DIFFUSE);
    s_pMaterial->EnableColorMaterial(true);
    s_pMaterial->EnableLighting(false);
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkyLight::~SkyLight
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyLight::~SkyLight()
* @brief @todo <WRITE BRIEF SkyLight::~SkyLight DOCUMENTATION>
* 
* @todo <WRITE EXTENDED SkyLight::~SkyLight FUNCTION DOCUMENTATION>
*/ 
SkyLight::~SkyLight()
{
}

//------------------------------------------------------------------------------
// Function     	  : SkyLight::Display
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyLight::Display() const
* @brief Displays a wireframe representation of the light.
* 
* This is useful for debugging.
*/ 
void SkyLight::Display() const
{
  s_pMaterial->Activate();
  //if (_bEnabled)
    //glColor3fv(&(_vecDiffuse.x));
  //else
    glColor3f(0, 0, 0);

  switch(_eType)
  {
  case SKY_LIGHT_POINT:
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    {
      glTranslatef(_vecPosition.x, _vecPosition.y, _vecPosition.z);
      glutWireSphere(4, 8, 8);
    }
    glPopMatrix();
    break;
  case SKY_LIGHT_DIRECTIONAL:
    {
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      {
        Mat44f mat;
        Vec3f vecPos(_vecPosition.x, _vecPosition.y, _vecPosition.z);
        Vec3f vecDir(_vecDirection.x, _vecDirection.y, _vecDirection.z);
        Vec3f vecUp(0, 1, 0);
        if (fabs(vecDir * vecUp) - 1 < 1e-6) // check that the view and up directions are not parallel.
          vecUp.Set(1, 0, 0);
        
        mat.invLookAt(vecPos, vecPos + 10 * vecDir, vecUp);
        
        glPushMatrix();
        {
          glTranslatef(-50 * vecDir.x, -50 * vecDir.y, -50 * vecDir.z);
          glMultMatrixf(mat);
          glutWireCone(10, 10, 4, 1);
        }
        glPopMatrix(); 
        
        glMultMatrixf(mat); 
        GLUquadric *pQuadric = gluNewQuadric();
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        gluCylinder(pQuadric, 4, 4, 50, 4, 4);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      }
      glPopMatrix();
    }
    break;
  case SKY_LIGHT_SPOT:
    {
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      {
        Mat44f mat;
        Vec3f vecUp = Vec3f(0, 1, 0);
        Vec3f vecPos(_vecPosition.x, _vecPosition.y, _vecPosition.z);
        Vec3f vecDir(_vecDirection.x, _vecDirection.y, _vecDirection.z);
        if (_vecDirection == vecUp)
          vecUp.Set(1, 0, 0);
        mat.invLookAt(vecPos + 50 * vecDir, vecPos + 51 * vecDir, vecUp);

        glMultMatrixf(mat);   
        float rAlpha= acos(pow(10, (-12 / _rSpotExponent)));
        //glutWireCone(50 * tan(SKYDEGREESTORADS * rAlpha), 50, 16, 8);
        glutWireCone(50 * tan(SKYDEGREESTORADS * _rSpotCutoff), 50, 16, 8);
      }
      glPopMatrix();
    }
    break;
  default:
    break;
  }  
}

//------------------------------------------------------------------------------
// Function     	  : SkyLight::Activate
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyLight::Activate(int iLightID)
 * @brief @todo <WRITE BRIEF SkyLight::Activate DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyLight::Activate FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkyLight::Activate(int iLightID)
{
  glPushMatrix();
  // set the position every frame
  if (SKY_LIGHT_DIRECTIONAL != _eType)
    glLightfv(GL_LIGHT0 + iLightID, GL_POSITION,              &(_vecPosition.x));
  else
    glLightfv(GL_LIGHT0 + iLightID, GL_POSITION,              &(_vecDirection.x));
  
  if (SKY_LIGHT_SPOT == _eType)
    glLightfv(GL_LIGHT0 + iLightID, GL_SPOT_DIRECTION,      &(_vecDirection.x));

  // set other light properties only when they change.
  if (_bDirty || iLightID != _iLastGLID)
  {     
    glLightfv(GL_LIGHT0 + iLightID,   GL_DIFFUSE,               &(_vecDiffuse.x));
    glLightfv(GL_LIGHT0 + iLightID,   GL_AMBIENT,               &(_vecAmbient.x));
    glLightfv(GL_LIGHT0 + iLightID,   GL_SPECULAR,              &(_vecSpecular.x));
    glLightf(GL_LIGHT0 + iLightID,    GL_CONSTANT_ATTENUATION,  _vecAttenuation.x);
    glLightf(GL_LIGHT0 + iLightID,    GL_LINEAR_ATTENUATION,    _vecAttenuation.y);
    glLightf(GL_LIGHT0 + iLightID,    GL_QUADRATIC_ATTENUATION, _vecAttenuation.z);
    
    if (SKY_LIGHT_SPOT == _eType)
    {
      glLightf(GL_LIGHT0 + iLightID,  GL_SPOT_CUTOFF,         _rSpotCutoff);
      glLightf(GL_LIGHT0 + iLightID,  GL_SPOT_EXPONENT,       _rSpotExponent);
    }
    else
    {
      glLightf(GL_LIGHT0 + iLightID,  GL_SPOT_CUTOFF,         180);
      glLightf(GL_LIGHT0 + iLightID,  GL_SPOT_EXPONENT,       0);
    }
 
    if (_bEnabled)
      glEnable(GL_LIGHT0 + iLightID);
    else
    {
      glDisable(GL_LIGHT0 + iLightID);
    }
    
    _iLastGLID = iLightID;
    _bDirty = false;    
  }
  glPopMatrix();
 
  return SKYRESULT_OK;
}
