//------------------------------------------------------------------------------
// File : SkyTextureState.hpp
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
* @file SkyTextureState.hpp
* 
* Interface Definition for class SkyTextureState, which encapsulates OpenGL texture state.
*/
#ifndef __SKYTEXTURESTATE_HPP__
#define __SKYTEXTURESTATE_HPP__

#include "SkyUtil.hpp"
#include "SkyTexture.hpp"
#include "SkyContext.hpp"
#include <map>

//------------------------------------------------------------------------------
/**
* @class SkyTextureState
* @brief A wrapper for texture unit state.
* 
* @todo <WRITE EXTENDED CLASS DESCRIPTION>
*/
class SkyTextureState
{
public: // methods
	SkyTextureState();
	~SkyTextureState();

  SKYRESULT Activate();

  SKYRESULT SetTexture(unsigned int iTextureUnit, GLenum eTarget, SkyTexture& texture);
  SKYRESULT SetTexture(unsigned int iTextureUnit, GLenum eTarget, unsigned int iTextureID);
  SKYRESULT EnableTexture(unsigned int iTextureUnit, bool bEnable);
  SKYRESULT SetTextureParameter(unsigned int  iTextureUnit, 
                                GLenum        eParameter, 
                                GLenum        eMode);

  inline GLenum       GetActiveTarget(unsigned int iTextureUnit) const;
  inline unsigned int GetTextureID(unsigned int iTextureUnit) const;
  inline bool         IsTextureEnabled(unsigned int iTextureUnit) const;
  inline GLenum       GetTextureParameter(unsigned int iTextureUnit, GLenum eParameter) const;

protected: // datatypes    
  struct TexState
  {
    TexState() : eActiveTarget(GL_TEXTURE_2D), iBoundTexture(0), bEnabled(false)
    {
      // set state to GL defaults.
      int i;
      for (i = 0; i < SKY_TEXCOORD_COUNT; ++i) { eWrapMode[i] = GL_REPEAT; }
      eFilterMode[SKY_FILTER_MIN] = GL_NEAREST_MIPMAP_LINEAR;
      eFilterMode[SKY_FILTER_MAG] = GL_LINEAR;
    }

    enum TexCoord
    {
      SKY_TEXCOORD_S,
      SKY_TEXCOORD_T,
      SKY_TEXCOORD_R,
      SKY_TEXCOORD_COUNT
    };
    
    enum TexFilter
    {
      SKY_FILTER_MIN,
      SKY_FILTER_MAG,
      SKY_FILTER_COUNT
    };
    
    GLenum        eActiveTarget;
    unsigned int  iBoundTexture; 
    bool          bEnabled;
    GLenum        eWrapMode[SKY_TEXCOORD_COUNT];       
    GLenum        eFilterMode[SKY_FILTER_COUNT];  
  };

protected: // data

  TexState        *_pTextureUnitState;  // one per texture unit
    
  static unsigned int  s_iNumTextureUnits;
};


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::GetActiveTarget
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureState::GetActiveTarget(unsigned int iTextureUnit) const
 * @brief Returns the active texture target for the specified texture unit.
 * 
 * If an invalid texture unit is specifed, returns GL_NONE.
 */ 
inline GLenum SkyTextureState::GetActiveTarget(unsigned int iTextureUnit) const
{
  if (iTextureUnit >= s_iNumTextureUnits)
  {
    SkyTrace("SkyTextureState::GetActiveTexture(): Invalid texture unit.");
    return GL_NONE;
  }
  return _pTextureUnitState[iTextureUnit].eActiveTarget;
}


//------------------------------------------------------------------------------
// Function     	  : int SkyTextureState::GetTextureID
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn int SkyTextureState::GetTextureID(unsigned int iTextureUnit) const
* @brief Returns the texture ID associated with the specified texture unit.
* 
* If an invalid texture unit is specifed, returns GL_NONE.
*/ 
inline unsigned int SkyTextureState::GetTextureID(unsigned int iTextureUnit) const
{
  if (iTextureUnit >= s_iNumTextureUnits)
  {
    SkyTrace("SkyTextureState::GetTextureID(): Invalid texture unit.");
    return GL_NONE;
  }
  return _pTextureUnitState[iTextureUnit].iBoundTexture;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::IsTextureEnabled
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyTextureState::IsTextureEnabled(unsigned int iTextureUnit) const
* @brief Returns the status (enabled or disabled) of the specified texture unit.
* 
* If an invalid texture unit is specifed, returns false.
*/ 
inline bool SkyTextureState::IsTextureEnabled(unsigned int iTextureUnit) const
{
  if (iTextureUnit >= s_iNumTextureUnits)
  {
    SkyTrace("SkyTextureState::IsTextureEnabled(): Invalid texture unit.");
    return false;
  }
  return _pTextureUnitState[iTextureUnit].bEnabled;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureState::GetTextureParameter
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureState::GetTextureParameter(unsigned int iTextureUnit, GLenum eParamter) const
 * @brief Returns the current value of @eParameter on the specified texture unit.
 * 
 * If an invalid texture unit or parameter is specified, returns GL_NONE.
 */ 
inline GLenum SkyTextureState::GetTextureParameter(unsigned int iTextureUnit, GLenum eParameter) const
{
  if (iTextureUnit >= s_iNumTextureUnits)
  {
    SkyTrace("SkyTextureState::GetTextureParamter(): Invalid texture unit.");
    return GL_NONE;
  }

  switch (eParameter)
  {
  case GL_TEXTURE_WRAP_S:
    return _pTextureUnitState[iTextureUnit].eWrapMode[TexState::SKY_TEXCOORD_S];
    break;
  case GL_TEXTURE_WRAP_T:
    return _pTextureUnitState[iTextureUnit].eWrapMode[TexState::SKY_TEXCOORD_T];
    break;
  case GL_TEXTURE_WRAP_R:
    return _pTextureUnitState[iTextureUnit].eWrapMode[TexState::SKY_TEXCOORD_R];
    break;
  case GL_TEXTURE_MIN_FILTER:
    return _pTextureUnitState[iTextureUnit].eFilterMode[TexState::SKY_FILTER_MIN];
    break;
  case GL_TEXTURE_MAG_FILTER:
    return _pTextureUnitState[iTextureUnit].eFilterMode[TexState::SKY_FILTER_MAG];
    break;
  default:
    SkyTrace("SkyTExtureState::SetTextureParameter(): Invalid parameter.");
    break;
  }
  return GL_NONE;
}

#endif //__SKYTEXTURESTATE_HPP__
