
//------------------------------------------------------------------------------
// File : SkyTextureState.cpp
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
// It is provided "as is" without express or 
// implied warranty.
/**
 * @file SkyTextureState.cpp
 * 
 * Implementation of class SkyTextureState, which encapsulates OpenGL texture state.
 */
#include "SkyTextureState.hpp"
//#include "glvu.hpp"

//------------------------------------------------------------------------------
// Static initializations.
//------------------------------------------------------------------------------
unsigned int SkyTextureState::s_iNumTextureUnits = 0;


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::SkyTextureState
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyTextureState::SkyTextureState()
* @brief Constructor.
*/ 
SkyTextureState::SkyTextureState()
{
  if (0 == s_iNumTextureUnits)
  {
    int iNumTextureUnits = 0;
#ifdef GL_ARB_multitexture
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &iNumTextureUnits);
    if (iNumTextureUnits > 0)
      s_iNumTextureUnits = iNumTextureUnits;
    else
      s_iNumTextureUnits = 1;
#endif
  }
  
  _pTextureUnitState = new TexState[s_iNumTextureUnits];
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::~SkyTextureState
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyTextureState::~SkyTextureState()
* @brief Destructor.
*/ 
SkyTextureState::~SkyTextureState()
{
  SAFE_DELETE(_pTextureUnitState);
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::Activate
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureState::Activate()
 * @brief @todo <WRITE BRIEF SkyTextureState::Activate DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyTextureState::Activate FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkyTextureState::Activate()
{
  SkyTextureState *pCurrent = GraphicsContext::InstancePtr()->GetCurrentTextureState();
  assert(NULL != pCurrent);
  //GLVU::CheckForGLError("SkyTextureState::Activate(8)");
  for (unsigned int i = 0; i < s_iNumTextureUnits; ++i)
  {
#ifdef GL_ARB_multitexture
    if (s_iNumTextureUnits > 1)
      glActiveTextureARB(GL_TEXTURE0_ARB + i);
#endif
    bool bEnabled = IsTextureEnabled(i);
    if (pCurrent->IsTextureEnabled(i) != bEnabled)
    {
      FAIL_RETURN(pCurrent->EnableTexture(i, bEnabled));
      //GLVU::CheckForGLError("SkyTextureState::Activate(7)");
      if (bEnabled)
        glEnable(GetActiveTarget(i));
      else
        glDisable(GetActiveTarget(i));
    }
    //GLVU::CheckForGLError("SkyTextureState::Activate(6)");
    if (bEnabled)
    {
      GLenum eTarget    = GetActiveTarget(i);
      unsigned int iID  = GetTextureID(i);
      if ((pCurrent->GetActiveTarget(i) != eTarget) ||
          (pCurrent->GetTextureID(i)    != iID))
      {
        FAIL_RETURN(pCurrent->SetTexture(i, eTarget, iID));
        glBindTexture(eTarget, iID);
      }
      //GLVU::CheckForGLError("SkyTextureState::Activate(5)");
      GLenum paramValue = GetTextureParameter(i, GL_TEXTURE_WRAP_S);
      if (pCurrent->GetTextureParameter(i, GL_TEXTURE_WRAP_S) != paramValue) 
      {
        FAIL_RETURN(pCurrent->SetTextureParameter(i, GL_TEXTURE_WRAP_S, paramValue));
        glTexParameteri(eTarget, GL_TEXTURE_WRAP_S, paramValue);
      }
      //GLVU::CheckForGLError("SkyTextureState::Activate(4)");
      paramValue = GetTextureParameter(i, GL_TEXTURE_WRAP_T);
      if (pCurrent->GetTextureParameter(i, GL_TEXTURE_WRAP_T) != paramValue) 
      {
        FAIL_RETURN(pCurrent->SetTextureParameter(i, GL_TEXTURE_WRAP_T, paramValue));
        glTexParameteri(eTarget, GL_TEXTURE_WRAP_T, paramValue);
      }
      //GLVU::CheckForGLError("SkyTextureState::Activate(3)");
      paramValue = GetTextureParameter(i, GL_TEXTURE_WRAP_R);
      if (pCurrent->GetTextureParameter(i, GL_TEXTURE_WRAP_R) != paramValue) 
      {
        FAIL_RETURN(pCurrent->SetTextureParameter(i, GL_TEXTURE_WRAP_R, paramValue));
        glTexParameteri(eTarget, GL_TEXTURE_WRAP_R, paramValue);
      }
      //GLVU::CheckForGLError("SkyTextureState::Activate(2)");
      paramValue = GetTextureParameter(i, GL_TEXTURE_MIN_FILTER);
      if (pCurrent->GetTextureParameter(i, GL_TEXTURE_MIN_FILTER) != paramValue) 
      {
        FAIL_RETURN(pCurrent->SetTextureParameter(i, GL_TEXTURE_MIN_FILTER, paramValue));
        glTexParameteri(eTarget, GL_TEXTURE_MIN_FILTER, paramValue);
      }
       //GLVU::CheckForGLError("SkyTextureState::Activate(1)");
      paramValue = GetTextureParameter(i, GL_TEXTURE_MAG_FILTER);
      if (pCurrent->GetTextureParameter(i, GL_TEXTURE_MAG_FILTER) != paramValue) 
      {
        FAIL_RETURN(pCurrent->SetTextureParameter(i, GL_TEXTURE_MAG_FILTER, paramValue));
        glTexParameteri(eTarget, GL_TEXTURE_MIN_FILTER, paramValue);
      }
      //GLVU::CheckForGLError("SkyTextureState::Activate()");
    }
#ifdef GL_ARB_multitexture
    if (s_iNumTextureUnits > 1)
      glActiveTextureARB(GL_TEXTURE0_ARB);
#endif
  }
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::SetTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureState::SetTexture(unsigned int iTextureUnit, GLenum eTarget, SkyTexture&  texture)
 * @brief @todo <WRITE BRIEF SkyTextureState::BindTexture DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyTextureState::BindTexture FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkyTextureState::SetTexture(unsigned int iTextureUnit, 
                                      GLenum       eTarget, 
                                      SkyTexture&  texture)
{
  if (iTextureUnit >= s_iNumTextureUnits)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyTextureState::BindTexture(): Invalid texture unit.");
  }
  
  _pTextureUnitState[iTextureUnit].eActiveTarget = eTarget;
  _pTextureUnitState[iTextureUnit].iBoundTexture = texture.GetID();
  
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::SetTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureState::SetTexture(unsigned int iTextureUnit, GLenum eTarget, unsigned int iTextureID)
 * @brief @todo <WRITE BRIEF SkyTextureState::SetTexture DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyTextureState::SetTexture FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkyTextureState::SetTexture(unsigned int iTextureUnit, 
                                      GLenum       eTarget, 
                                      unsigned int iTextureID)
{
  if (iTextureUnit >= s_iNumTextureUnits)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyTextureState::BindTexture(): Invalid texture unit.");
  }
  
  _pTextureUnitState[iTextureUnit].eActiveTarget = eTarget;
  _pTextureUnitState[iTextureUnit].iBoundTexture = iTextureID;
  
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::EnableTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureState::EnableTexture(unsigned int iTextureUnit, bool bEnable)
 * @brief @todo <WRITE BRIEF SkyTextureState::EnableTexture DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyTextureState::EnableTexture FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkyTextureState::EnableTexture(unsigned int iTextureUnit, bool bEnable)
{
  if (iTextureUnit >= s_iNumTextureUnits)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyTextureState::EnableTexture(): Invalid texture unit.");
  }  
  
  _pTextureUnitState[iTextureUnit].bEnabled = bEnable;
  
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::SetTextureParameter
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureState::SetTextureParameter(unsigned int iTextureUnit, GLenum eParameter, GLenum eMode)
 * @brief @todo <WRITE BRIEF SkyTextureState::SetTextureParameter DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyTextureState::SetTextureParameter FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkyTextureState::SetTextureParameter(unsigned int iTextureUnit, 
                                               GLenum       eParameter, 
                                               GLenum       eMode)
{
  if (iTextureUnit >= s_iNumTextureUnits)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyTextureState::SetTextureParameter(): Invalid texture unit.");
  }  

  switch (eParameter)
  {
  case GL_TEXTURE_WRAP_S:
    _pTextureUnitState[iTextureUnit].eWrapMode[TexState::SKY_TEXCOORD_S] = eMode;
    break;
  case GL_TEXTURE_WRAP_T:
    _pTextureUnitState[iTextureUnit].eWrapMode[TexState::SKY_TEXCOORD_T] = eMode;
    break;
  case GL_TEXTURE_WRAP_R:
    _pTextureUnitState[iTextureUnit].eWrapMode[TexState::SKY_TEXCOORD_R] = eMode;
    break;
  case GL_TEXTURE_MIN_FILTER:
    _pTextureUnitState[iTextureUnit].eFilterMode[TexState::SKY_FILTER_MIN] = eMode;
    break;
  case GL_TEXTURE_MAG_FILTER:
    _pTextureUnitState[iTextureUnit].eFilterMode[TexState::SKY_FILTER_MAG] = eMode;
    break;
  default:
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyTExtureState::SetTextureParameter(): Invalid parameter.");
    break;
  }
  
  return SKYRESULT_OK;
}

