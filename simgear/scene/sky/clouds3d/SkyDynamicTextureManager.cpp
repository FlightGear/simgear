//------------------------------------------------------------------------------
// File : SkyDynamicTextureManager.cpp
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
 * @file SkyDynamicTextureManager.cpp
 * 
 * Implementation of a repository for check out and check in of dynamic textures.
 */

#pragma warning( disable : 4786 )

#include "SkyDynamicTextureManager.hpp"
#include "SkyTexture.hpp"
#include "SkyContext.hpp"

#pragma warning( disable : 4786 )

//! Set this to 1 to print lots of dynamic texture usage messages.
#define SKYDYNTEXTURE_VERBOSE        0
//! The maximum number of textures of each resolution to allow in the checked in pool.
#define SKYDYNTEXTURE_TEXCACHE_LIMIT 32

//------------------------------------------------------------------------------
// Function     	  : SkyDynamicTextureManager::SkyDynamicTextureManager
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyDynamicTextureManager::SkyDynamicTextureManager()
 * @brief Constructor.
 */ 
SkyDynamicTextureManager::SkyDynamicTextureManager()
#ifdef SKYDYNTEXTURE_VERBOSE
: _iNumTextureBytesUsed(0),
  _iNumTextureBytesCheckedIn(0),
  _iNumTextureBytesCheckedOut(0)
#endif
{
  for (int i = 0; i < 11; ++i)
    for (int j = 0; j < 11; ++j)
      _iAvailableSizeCounts[i][j] = 0;
}


//------------------------------------------------------------------------------
// Function     	  : SkyDynamicTextureManager::~SkyDynamicTextureManager
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyDynamicTextureManager::~SkyDynamicTextureManager()
 * @brief Destructor.
 */ 
SkyDynamicTextureManager::~SkyDynamicTextureManager()
{
  for ( TextureSet::iterator subset = _availableTexturePool.begin();
        subset != _availableTexturePool.end();
        ++subset )
  { // iterate over texture subsets.
    for ( TextureSubset::iterator texture = (*subset).second->begin(); 
          texture != (*subset).second->end(); 
          ++texture )
    {
      texture->second->Destroy();
      delete texture->second;
    }
    subset->second->clear();
  }
  _availableTexturePool.clear();

  for ( TextureSubset::iterator texture = _checkedOutTexturePool.begin();
        texture != _checkedOutTexturePool.end();
        ++texture )
  {
    texture->second->Destroy();
    delete texture->second;
  }
  _checkedOutTexturePool.clear();
}


//------------------------------------------------------------------------------
// Function     	  : SkyDynamicTextureManager::CheckOutTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyDynamicTextureManager::CheckOutTexture(unsigned int iWidth, unsigned int iHeight)
 * @brief Returns a texture from the available pool, creating a new one if necessary.
 * 
 * Thi texture returned by this method is checked out: it will be maintained in a 
 * checked out pool until it is checked in with CheckInTexture().  The texture is owned 
 * by the SkyDynamicTextureManager -- it should not be deleted by another object.  Checked out
 * textures can be modified (copied, or rendered to, etc.), but should not be reallocated
 * or resized.  All checked out textures will be deleted when the SkyDynamicTextureManager
 * is destroyed, so this manager should be destroyed only after rendering ceases.
 */ 
SkyTexture* SkyDynamicTextureManager::CheckOutTexture(unsigned int iWidth, 
                                                      unsigned int iHeight)
{
  int iWidthLog, iHeightLog;
  iWidthLog = SkyGetLogBaseTwo(iWidth);
  iHeightLog = SkyGetLogBaseTwo(iHeight);

  // first see if a texture of this resolution is available:
  // find the subset of textures with width = iWidth, if it exists.
  TextureSet::iterator subset = _availableTexturePool.find(iWidth);
  if (subset != _availableTexturePool.end())
  { // found the iWidth subset
    // now find a texture with height = iHeight:
    TextureSubset::iterator texture = (*subset).second->find(iHeight);
    if (texture != (*subset).second->end())
    { //  found one!
      // extract the texture
      SkyTexture *pTexture = (*texture).second;
      (*texture).second = NULL;
      // first remove it from this set.
      (*subset).second->erase(texture);
      // now add it to the checked out texture set.
      _checkedOutTexturePool.insert(TextureSubset::value_type(pTexture->GetID(), pTexture));

      // update checked out/in amount.
#if SKYDYNTEXTURE_VERBOSE
      _iNumTextureBytesCheckedIn  -= iWidth * iHeight * 4;
      _iNumTextureBytesCheckedOut += iWidth * iHeight * 4;
      printf("CHECKOUT: %d x %d\n", iWidth, iHeight);
#endif
      _iAvailableSizeCounts[iWidthLog][iHeightLog]--;
      // we're now free to give this texture to the user
      return pTexture;
    }
    else
    { // we didn't find an iWidth x iHeight texture, although the iWidth subset exists
      // create a new texture of the appropriate dimensions and return it.
      SkyTexture *pNewTexture = CreateDynamicTexture(iWidth, iHeight);
      _checkedOutTexturePool.insert(TextureSubset::value_type(pNewTexture->GetID(), 
                                                              pNewTexture));
#if SKYDYNTEXTURE_VERBOSE
      _iNumTextureBytesCheckedOut += iWidth * iHeight * 4;
#endif

      return pNewTexture;
    }
  }
  else
  { // we don't yet have a subset for iWidth textures.  Create one.
    TextureSubset *pSubset = new TextureSubset;
    _availableTexturePool.insert(TextureSet::value_type(iWidth, pSubset));
    // now create a new texture of the appropriate dimensions and return it.
    SkyTexture *pNewTexture = CreateDynamicTexture(iWidth, iHeight);
    _checkedOutTexturePool.insert(TextureSubset::value_type(pNewTexture->GetID(), pNewTexture));
   
#if SKYDYNTEXTURE_VERBOSE
    _iNumTextureBytesCheckedOut += iWidth * iHeight * 4;
#endif
    return pNewTexture;
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkyDynamicTextureManager::CheckInTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyDynamicTextureManager::CheckInTexture(SkyTexture *pTexture)
 * @brief Returns a checked-out texture to the available pool.
 * 
 * This method removes the checked out texture from the checked out pool if it is
 * checked out, and then checks it in to the available pool.
 */ 
void SkyDynamicTextureManager::CheckInTexture(SkyTexture *pTexture)
{
  // first see if the texture is in the checked out pool.
  TextureSubset::iterator coTexture = _checkedOutTexturePool.find(pTexture->GetID());
  if (coTexture != _checkedOutTexturePool.end())
  { // if it is there, remove it.
    _checkedOutTexturePool.erase(coTexture);
    _iNumTextureBytesCheckedOut -= pTexture->GetWidth() * pTexture->GetHeight() * 4;
  }

  // Don't cache too many unused textures.
  int iWidthLog, iHeightLog;
  iWidthLog = SkyGetLogBaseTwo(pTexture->GetWidth());
  iHeightLog = SkyGetLogBaseTwo(pTexture->GetHeight());
  if (_iAvailableSizeCounts[iWidthLog][iHeightLog] >= SKYDYNTEXTURE_TEXCACHE_LIMIT)
  {
#if SKYDYNTEXTURE_VERBOSE
    _iNumTextureBytesUsed -= pTexture->GetWidth() * pTexture->GetHeight() * 4;
    printf("%dx%d texture DESTROYED.\n\t Total memory used: %d bytes.\n", 
             pTexture->GetWidth(), pTexture->GetHeight(), _iNumTextureBytesUsed);
#endif

    pTexture->Destroy();
    SAFE_DELETE(pTexture);
    return;
  }

  // now check the texture into the available pool.
  // find the width subset:
  TextureSet::iterator subset = _availableTexturePool.find(pTexture->GetWidth());
  if (subset != _availableTexturePool.end())
  { // the subset exists.  Add the texture to it
    (*subset).second->insert(TextureSubset::value_type(pTexture->GetHeight(), pTexture));
    _iNumTextureBytesCheckedIn += pTexture->GetWidth() * pTexture->GetHeight() * 4;

    _iAvailableSizeCounts[iWidthLog][iHeightLog]++;
  }
  else
  { // subset not found.  Create it.
    TextureSubset *pSubset = new TextureSubset;
    // insert the texture.
    pSubset->insert(TextureSubset::value_type(pTexture->GetHeight(), pTexture));
    // insert the subset into the available pool
    _availableTexturePool.insert(TextureSet::value_type(pTexture->GetWidth(), pSubset));

#if SKYDYNTEXTURE_VERBOSE
    _iNumTextureBytesCheckedIn += pTexture->GetWidth() * pTexture->GetHeight() * 4;
    _iAvailableSizeCounts[iWidthLog][iHeightLog]++;
#endif
  }

  pTexture = NULL;
}


//------------------------------------------------------------------------------
// Function     	  : SkyDynamicTextureManager::CreateDynamicTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyDynamicTextureManager::CreateDynamicTexture(unsigned int iWidth, unsigned int iHeight)
 * @brief Allocate a new dynamic texture object of the given resolution.
 * 
 * This method is used by CheckOutTexture() when it can't find an available texture of 
 * the requested resolution.  It can also be called externally, but will result in an 
 * unmanaged texture unless the new texture is subsequently checked in using CheckInTexture().
 */ 
SkyTexture* SkyDynamicTextureManager::CreateDynamicTexture(unsigned int iWidth, unsigned int iHeight)
{
  unsigned int iID;
  glGenTextures(1, &iID);
  SkyTexture *pNewTexture = new SkyTexture(iWidth, iHeight, iID);
  glBindTexture(GL_TEXTURE_2D, pNewTexture->GetID());
  // set default filtering.
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // create an empty buffer
  unsigned char *pData = new unsigned char[iWidth * iHeight * 4];

  // allocate the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, iWidth, iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData );

  delete [] pData;

  // update the used texture bytes...
  _iNumTextureBytesUsed += iWidth * iHeight * 4;
#if SKYDYNTEXTURE_VERBOSE
  printf("New %dx%d texture created.\n\t Total memory used: %d bytes\n", iWidth, iHeight, _iNumTextureBytesUsed);
  printf("\tTotal memory checked in: %d\n", _iNumTextureBytesCheckedIn);
  printf("\tTotal memory checked out: %d\n", _iNumTextureBytesCheckedOut);
#endif

  return pNewTexture;
}
