//------------------------------------------------------------------------------
// File : SkyDynamicTextureManager.hpp
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
 * @file SkyDynamicTextureManager.hpp
 * 
 * Interface definition of a repository for check out and check in of dynamic textures.
 */
#ifndef __SKYDYNAMICTEXTUREMANAGER_HPP__
#define __SKYDYNAMICTEXTUREMANAGER_HPP__
  
// warning for truncation of template name for browse info
#pragma warning( disable : 4786)

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

#include <map>

#include <simgear/compiler.h>

#include SG_GL_H

#include "SkyUtil.hpp"
#include "SkySingleton.hpp"

using namespace std;

class SkyTexture;
class SkyDynamicTextureManager;

//! Dynamic Texture Manager Singleton declaration.
/*! The DynamicTextureManager must be created by calling DynamicTextureManager::Instantiate(). */
typedef SkySingleton<SkyDynamicTextureManager> DynamicTextureManager;

//------------------------------------------------------------------------------
/**
 * @class SkyDynamicTextureManager
 * @brief A repository that allows check-out and check-in from a pool of dynamic textures.
 * 
 * When an object needs a dynamic texture, it checks it out using CheckOutTexture(), passing
 * the resolution of the texture it needs.  When the object is done with the texture, it
 * calls CheckInTexture().  New dynamic textures can be allocated by calling CreateDynamicTexture,
 * but these textures will be unmanaged.
 */
class SkyDynamicTextureManager
{
public:
  SkyTexture* CheckOutTexture(unsigned int iWidth, unsigned int iHeight);
  void        CheckInTexture(SkyTexture* pTexture);

  SkyTexture* CreateDynamicTexture(unsigned int iWidth, unsigned int iHeight);
  
protected: // methods
  SkyDynamicTextureManager(); // these are protected because it is a singleton.
  ~SkyDynamicTextureManager();

protected: // datatypes
  typedef multimap<unsigned int, SkyTexture*>    TextureSubset;
  typedef multimap<unsigned int, TextureSubset*> TextureSet;
  
protected: // data
  TextureSet      _availableTexturePool;
  TextureSubset   _checkedOutTexturePool;

  unsigned int    _iAvailableSizeCounts[11][11];

  unsigned int    _iNumTextureBytesUsed;
  unsigned int    _iNumTextureBytesCheckedOut;
  unsigned int    _iNumTextureBytesCheckedIn;
};

#endif //__SKYDYNAMICTEXTUREMANAGER_HPP__
