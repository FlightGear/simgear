//------------------------------------------------------------------------------
// File : SkyTextureManager.hpp
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
 * @file SkyTextureManager.hpp
 * 
 * Definition of a manager that keeps track of texture locations and sharing of texture files.                                          |
 */
#ifndef SKYTEXTUREMANAGER_HPP
#define SKYTEXTUREMANAGER_HPP

#pragma warning( disable : 4786)

#include "SkySingleton.hpp"
#include "SkyTexture.hpp"
#include <string>
#include <list>
#include <map>

using namespace std;

// forward declaration for singleton
class SkyTextureManager;

//! A singleton of the SkyTextureManager.  Can only create the TextureManager with TextureManager::Instantiate();
typedef SkySingleton<SkyTextureManager> TextureManager;

//------------------------------------------------------------------------------
/**
 * @class SkyTextureManager
 * @brief A resource manager for textures.
 * 
 * This manager allows textures to be shared.  It keeps a mapping of 
 * filenames to texture objects, and makes it easy to use the same texture
 * for multiple objects without the objects having to be aware of the 
 * sharing.  Supports cube map textures, and 2D textures.  Can also be used
 * to "clone textures", which creates unmanaged texture objects from files 
 * that are not kept in the mapping, and thus are not shared.
 */
class SkyTextureManager
{
public: // types
  typedef list<string> StringList;

public: // methods
  //.-------------------------------------------------------------------------.
  //|  Paths to texture directories
  //.-------------------------------------------------------------------------.
  void              AddPath(const string& path);
  //! Return the list of texture paths that will be searched by Get2DTexture() and Get3DTexture().
  const StringList& GetPaths() const                    { return _texturePaths;  }
  //! Clear the list of texture paths that will be searched by Get2DTexture() and Get3DTexture().
  void              ClearPaths()                        { _texturePaths.clear(); }
  
  //.-------------------------------------------------------------------------.
  //|  Texture loading
  //.-------------------------------------------------------------------------.
  SKYRESULT         Get2DTexture(         const string  &filename, 
                                          SkyTexture&   texture,
                                          bool          bMipmap             = false); 
  SKYRESULT         Get3DTexture(         const string  &filename, 
                                          SkyTexture&   texture,
                                          unsigned int  iDepth,
                                          bool          bMipmap             = false,
                                          bool          bLoadFromSliceFiles = false);
  SKYRESULT         GetCubeMapTexture(    const string  &filename, 
                                          SkyTexture&   texture,
                                          bool          bMipmap             = false);

  //.-------------------------------------------------------------------------.
  //|  Texture cloning: create a duplicate texture object: not added to set!
  //.-------------------------------------------------------------------------.
  SKYRESULT         Clone2DTexture(       const string  &filename, 
                                          SkyTexture&   texture,
                                          bool          bMipmap             = false);
  SKYRESULT         Clone3DTexture(       const string  &filename, 
                                          SkyTexture&   texture,
                                          unsigned int  iDepth,
                                          bool          bMipmap             = false,
                                          bool          bLoadFromSliceFiles = false );
  SKYRESULT         CloneCubeMapTexture(  const string  &filename, 
                                          SkyTexture&   texture,
                                          bool          bMipmap             = false);

  //.-------------------------------------------------------------------------.
  //|  Texture Object Creation: not added to the texture set (no filename!)
  //.-------------------------------------------------------------------------.
  inline SKYRESULT  Create2DTextureObject(SkyTexture    &texture, 
                                          unsigned int  iWidth, 
                                          unsigned int  iHeight,
                                          unsigned int  iFormat,
                                          unsigned char *pData);
  inline SKYRESULT  Create3DTextureObject(SkyTexture    &texture, 
                                          unsigned int  iWidth, 
                                          unsigned int  iHeight,
                                          unsigned int  iDepth,
                                          unsigned int  iFormat,
                                          unsigned char *pData);

  //.-------------------------------------------------------------------------.
  //|  Texture Object Destruction: use this because texture objects are structs
  //|  that use shallow copies!
  //.-------------------------------------------------------------------------.
  static void       DestroyTextureObject( SkyTexture &texture);

  
protected:
  SkyTextureManager(bool bSlice3DTextures = false);
  ~SkyTextureManager();

  SKYRESULT  _Create2DTextureObject(      SkyTexture    &texture, 
                                          unsigned int  iWidth, 
                                          unsigned int  iHeight,
                                          unsigned int  iFormat,
                                          unsigned char *pData);
  SKYRESULT  _Create3DTextureObject(      SkyTexture    &texture, 
                                          unsigned int  iWidth, 
                                          unsigned int  iHeight,
                                          unsigned int  iDepth,
                                          unsigned int  iFormat,
                                          unsigned char *pData);
private:
  typedef list<SkyTexture>        TextureList;
  typedef map<string, SkyTexture> TextureSet;
  typedef TextureSet::iterator    TextureIterator;
  
  //.-------------------------------------------------------------------------.
  //|  Data
  //.-------------------------------------------------------------------------.
  
  // paths searched for textures specified by filename.
  StringList  _texturePaths;
  
  // cached textures
  TextureSet  _textures;        // loaded textures
  // textures created directly, not loaded from file and cached.
  TextureList _uncachedTextures;
  
  // if this is true, then 3D textures will be represented as a set of 2D slices.
  static bool s_bSlice3DTextures;
};


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::Create2DTextureObject
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::Create2DTextureObject(SkyTexture &texture, unsigned int iWidth, unsigned int iHeight, unsigned int iFormat, unsigned char *pData)
 * @brief Creates a 2D texture.
 * 
 * Creates an OpenGL texture object and returns its ID and dimensions in a SkyTexture structure.
 * This texture will be deleted by the texture manager at shutdown.
 *
 */ 
inline SKYRESULT SkyTextureManager::Create2DTextureObject(SkyTexture &texture, 
                                                          unsigned int iWidth, 
                                                          unsigned int iHeight, 
                                                          unsigned int iFormat, 
                                                          unsigned char *pData)
{
  SKYRESULT retval = _Create2DTextureObject(texture, iWidth, iHeight, iFormat, pData);
  if SKYSUCCEEDED(retval)
    _uncachedTextures.push_back(texture);
  return retval;
}
  

//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::Create3DTextureObject
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::Create3DTextureObject(SkyTexture &texture, unsigned int iWidth, unsigned int iHeight, unsigned int iDepth, unsigned int iFormat, unsigned char *pData)
 * @brief Creates a 3D texture.
 * 
 * Creates an OpenGL texture object and returns its ID and dimensions in a SkyTexture structure.
 * This texture will be deleted by the texture manager at shutdown, and should not be destroyed 
 * by the user.
 *
 */ 
inline SKYRESULT SkyTextureManager::Create3DTextureObject(SkyTexture &texture, 
                                                          unsigned int iWidth, 
                                                          unsigned int iHeight, 
                                                          unsigned int iDepth, 
                                                          unsigned int iFormat, 
                                                          unsigned char *pData)
{
  SKYRESULT retval = _Create3DTextureObject(texture, iWidth, iHeight, iDepth, iFormat, pData);
  if SKYSUCCEEDED(retval)
    _uncachedTextures.push_back(texture);
  return retval;
}

#endif //QGLVUTEXTUREMANAGER_HPP
