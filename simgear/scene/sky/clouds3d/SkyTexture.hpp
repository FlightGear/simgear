//------------------------------------------------------------------------------
// File : SkyTexture.hpp
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
 * @file SkyTexture.hpp
 * 
 * Interface definition for class SkyTexture, a texture class.
 */
#ifndef __SKYTEXTURE_HPP__
#define __SKYTEXTURE_HPP__

// #pragma warning( disable : 4786 )

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <simgear/compiler.h>

#include SG_GL_H

#define __glext_h_
#define __GLEXT_H_
#define __glext_h_
#define __GLEXT_H_

//------------------------------------------------------------------------------
/**
 * @class SkyTexture
 * @brief A basic texture class.
 * 
 * @todo <WRITE EXTENDED CLASS DESCRIPTION>
 */
class SkyTexture
{
public:
  //! Default Constructor.
  SkyTexture() : _iID(0), _iWidth(0), _iHeight(0) {}
  //! Constructor.
  SkyTexture(unsigned int iWidth, unsigned int iHeight, unsigned int iTextureID)
    : _iID(iTextureID), _iWidth(iWidth), _iHeight(iHeight) {}
  //! Destructor.
  ~SkyTexture() {}

  //! Sets the texture width in texels.
  void              SetWidth(unsigned int iWidth)   { _iWidth  = iWidth;      }
  //! Sets the texture height in texels.
  void              SetHeight(unsigned int iHeight) { _iHeight = iHeight;     }
  //! Sets the texture ID as created by OpenGL.
  void              SetID(unsigned int iTextureID)  { _iID     = iTextureID;  }
                    
  //! Returns the texture width in texels.
  unsigned int      GetWidth() const                { return _iWidth;         }
  //! Returns the texture height in texels.
  unsigned int      GetHeight() const               { return _iHeight;        }
  //! Returns the texture ID as created by OpenGL.
  unsigned int      GetID() const                   { return _iID;            }

  inline SKYRESULT  Destroy();

protected:
  unsigned int _iID;
  unsigned int _iWidth;
  unsigned int _iHeight;
};


//------------------------------------------------------------------------------
// Function     	  : SkyTexture::Destroy
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTexture::Destroy()
 * @brief Destroys the OpenGL texture object represented by this SkyTexture object.
 * 
 * Fails if the GL texture has not been created (i.e. its ID is zero).
 */ 
inline SKYRESULT SkyTexture::Destroy()
{
  if (0 == _iID)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, 
                    "SkyTexture::Destroy(): Error: attempt to destroy unallocated texture.");
  }
  else
  {
    glDeleteTextures(1, &_iID);
    _iID = 0;
  }
  return SKYRESULT_OK;
}

#endif //__SKYTEXTURE_HPP__
