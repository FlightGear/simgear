//------------------------------------------------------------------------------
// File : SkyTextureManager.cpp
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
 * @file SkyTextureManager.cpp
 * 
 * Implementation of a manager that keeps track of texture resource locations and sharing.
 */

#pragma warning( disable : 4786)

#include "SkyTextureManager.hpp"
#include "SkyContext.hpp"
//#include "glvu.hpp"
//#include "ppm.hpp"
//#include "tga.hpp"
//#include "fileutils.hpp"

bool SkyTextureManager::s_bSlice3DTextures = false;

//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::SkyTextureManager
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::SkyTextureManager(bool bSlice3DTextures)
 * @brief Constructor.
 * 
 */ 
SkyTextureManager::SkyTextureManager(bool bSlice3DTextures /* = false */)
{
  s_bSlice3DTextures = bSlice3DTextures;

  // this should be put somewhere more safe -- like done once in the functions that actually
  // use these extensions.
  /*GraphicsContext::InstancePtr()->InitializeExtensions("GL_ARB_texture_cube_map "
                                                         "GL_VERSION_1_2");*/
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::~SkyTextureManager
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::~SkyTextureManager()
 * @brief destructor.
 * 
 * @todo <WRITE EXTENDED SkyTextureManager::~SkyTextureManager FUNCTION DOCUMENTATION>
 */ 
SkyTextureManager::~SkyTextureManager()
{
  _texturePaths.clear();
  for ( TextureIterator iter = _textures.begin(); 
        iter != _textures.end(); 
        ++iter)
  {
    DestroyTextureObject(iter->second);
  }
  _textures.clear();

  for ( TextureList::iterator uncachedIter = _uncachedTextures.begin(); 
        uncachedIter != _uncachedTextures.end();
        ++uncachedIter)
  {
    DestroyTextureObject(*uncachedIter);
  }
  _uncachedTextures.clear();
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::AddPath
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::AddPath(const string &path)
 * @brief Adds a texture path to the list of active search paths. 
 * 
 */ 
void  SkyTextureManager::AddPath(const string &path)
{
  _texturePaths.push_back(path);
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::Get2DTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::Get2DTexture(const string& filename, SkyTexture& texture, bool bMipmap)
 * @brief Returns a 2D texture object from the texture set.
 * 
 * If the texture is already loaded, it is returned.  If it is not, the texture is loaded from 
 * file, added to the texture set, and returned.
 * 
 * If the image cannot be loaded, returns an error.  Otherwise returns success.
 */ 
SKYRESULT SkyTextureManager::Get2DTexture(const string& filename, 
                                          SkyTexture& texture,
                                          bool bMipmap /* = false */)
{
  TextureIterator iter = _textures.find(filename);
  
  if (iter != _textures.end())
  { // the texture is already loaded, just return it.
    texture = iter->second;
  }
  else
  { // the texture is being requested for the first time, load and return it
    FAIL_RETURN(Clone2DTexture(filename, texture, bMipmap));

    _textures.insert(make_pair(filename, texture));  
  }

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::Get3DTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::Get3DTexture(const string& filename, SkyTexture& texture, unsigned int iDepth, bool bMipmap, bool bLoadFromSliceFiles)
 * @brief Returns a 3D texture object from the texture set.
 * 
 * If the texture is already loaded, it is returned.  If it is not, the texture is loaded from 
 * file, added to the texture set, and returned.  If the image cannot be loaded, returns an error.  
 * Otherwise returns success. 
 *
 * For 3D textures, this simply loads a 2D image file, and duplicates it across each slice.  The 
 * parameter iDepth must be set in order to use a 2D texture image for a 3D texture. 
 */ 
SKYRESULT SkyTextureManager::Get3DTexture(const string& filename, 
                                       SkyTexture& texture,
                                       unsigned int iDepth,
                                       bool bMipmap /* = false */,
                                       bool bLoadFromSliceFiles /* = false */)
{
  TextureIterator iter = _textures.find(filename);
  
  if (iter != _textures.end())
  { // the texture is already loaded, just return it.
    texture = iter->second;
  }
  else
  { // the texture is being requested for the first time, load and return it
    FAIL_RETURN(Clone3DTexture(filename, texture, iDepth, bMipmap, bLoadFromSliceFiles));

    _textures.insert(make_pair(filename, texture));  
  }

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : roundPowerOf2
// Description	    : 
//------------------------------------------------------------------------------
static int roundPowerOf2(int n)
{
  int m;
  for (m = 1; m < n; m *= 2);

  // m >= n
  if (m - n <= n - m/2) 
    return m;
  else
    return m/2;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::Clone2DTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::Clone2DTexture( const string &filename, SkyTexture& texture, bool bMipmap)
 * @brief Returns a 2D texture object.
 * 
 * Ignores texture set.  This always loads the file, if it exists, and creates and returns the texture.
 *
 * If the image cannot be loaded, returns an error.  Otherwise returns success.
 */ 
SKYRESULT SkyTextureManager::Clone2DTexture(const string &filename, 
                                            SkyTexture& texture,
                                            bool bMipmap /* = false */)
{
  string pathFilename;
  unsigned char *pImageData = NULL;
  int iWidth       = 0;
  int iHeight      = 0;
  int iChannels    = 0;

  enum ImageType
  {
    IMAGE_PPM,
    IMAGE_TGA
  };
  ImageType eType;
/****
  // first get the image type from its extension.
  if (filename.find(".tga") != string.npos || filename.find(".TGA") != string.npos)
    eType = IMAGE_TGA;
  else if (filename.find(".ppm") != string.npos || filename.find(".PPM") != string.npos)
    eType = IMAGE_PPM;
  else 
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyTextureManager error: invalid image format");
  ****/
  // first try the filename sent in in case it includes a path.
  //if (FileUtils::FileExists(filename.c_str()))
  //{
 // printf("Filename is %s\n",  filename.c_str() );
  //eType = IMAGE_TGA;
  /*  switch (eType)
    {
    case IMAGE_PPM:
      LoadPPM(filename.c_str(), pImageData, iWidth, iHeight);
      iChannels = 3;
      break;
    case IMAGE_TGA:      
      LoadTGA(filename.c_str(), pImageData, iWidth, iHeight, iChannels);
      break;
    default:
      break;
    }*/
    
  //}
  
  if (!pImageData) // image not found in current directory.  Check the paths...
  {
    
    for ( StringList::iterator iter = _texturePaths.begin(); 
          iter != _texturePaths.end(); 
          ++iter)

    { // loop over all texture paths, looking for the filename
      // get just the filename without path.
      int iPos = filename.find_last_of("/");
      if (iPos == filename.npos)
        iPos = filename.find_last_of("/");
      
      // tack on the paths from the texture path list.
      if (iPos != filename.npos)
        pathFilename = (*iter) + "/" +  filename.substr(iPos+1);
      else
        pathFilename = (*iter) + "/" +  filename;
        
      //if (FileUtils::FileExists(pathFilename.c_str()))
      //{
       /* switch (eType)
        {
        case IMAGE_PPM:
          LoadPPM(pathFilename.c_str(), pImageData, iWidth, iHeight);
          break;
        case IMAGE_TGA:
          LoadTGA(pathFilename.c_str(), pImageData, iWidth, iHeight, iChannels);
          break;
        default:
          break;
        }*/
        
        //if (pImageData)
          //break;
      //}
    }
  }

  if (!pImageData)
  {
    char buffer[256];
    sprintf(buffer, "SkyTextureManager::Clone2DTexture(): Could not load image. %s.\n", filename.c_str());
    FAIL_RETURN_MSG(SKYRESULT_OK, buffer);
  }

  // make sure it is power of 2 resolution.
  int iNewWidth  = roundPowerOf2(iWidth);
  int iNewHeight = roundPowerOf2(iHeight);
  int iMaxsize;
  glGetIntegerv( GL_MAX_TEXTURE_SIZE, &iMaxsize );
  if (iNewWidth > iMaxsize) 
  {
    iNewWidth = iMaxsize;
  }
  if (iNewHeight> iMaxsize) 
  {
    iNewHeight = iMaxsize;
  }

  GLenum eFormat = (4 == iChannels) ? GL_RGBA : GL_RGB;

  if (iNewWidth != iWidth || iNewHeight != iHeight)
  {
    unsigned char *pScaledImageData = new unsigned char[iChannels * iNewWidth * iNewHeight];  
    gluScaleImage(eFormat, iWidth, iHeight, GL_UNSIGNED_BYTE, pImageData, 
                  iNewWidth, iNewHeight, GL_UNSIGNED_BYTE, pScaledImageData);
    SAFE_DELETE_ARRAY(pImageData);
    pImageData = pScaledImageData;
  }
  
  _Create2DTextureObject( texture, iNewWidth, iNewHeight, eFormat, pImageData);
  
  SAFE_DELETE_ARRAY(pImageData);
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::Clone3DTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::Clone3DTexture( const string &filename, SkyTexture& texture, unsigned int iDepth, bool bMipmap, bool bLoadFromSliceFiles)
 * @brief Returns a 3D texture object.
 * 
 * Ignores texture set.  This always loads the file, if it exists, and creates and returns the texture.
 * If the image cannot be loaded, returns an error.  Otherwise returns success.
 */ 
SKYRESULT SkyTextureManager::Clone3DTexture(const string &filename, 
                                            SkyTexture& texture,
                                            unsigned int iDepth,
                                            bool bMipmap /* = false */,
                                            bool bLoadFromSliceFiles /* = false */)
{
  string pathFilename;
  /*QImage  image;
  QDir    dir;
  unsigned char *pBits = NULL;
  
  if (!bLoadFromSliceFiles)
  {
    // first try the filename sent in in case it includes a path.
    if (image.load(filename))
    {
      image = QGLWidget::convertToGLFormat(image);
    }
    else
    {
      image.reset();
      
      for ( QStringList::Iterator iter = _texturePaths.begin(); 
      iter != _texturePaths.end(); 
      iter++)
      { // loop over all texture paths, looking for the filename
        pathFilename = (*iter) + "\\" +  filename;
        
        if (image.load(pathFilename))
        {
          image = QGLWidget::convertToGLFormat(image);
          break;
        }
        else
          image.reset();
      }
    }
    
    if (image.isNull())
    {
      qWarning("SkyTextureManager::GetTexture(): Could not load image "
        "%s.\n", filename);
      return false;
    }
    
    // make sure it is power of 2 resolutions.
    int iWidth  = roundPowerOf2(image.width());
    int iHeight = roundPowerOf2(image.height());
    int iMaxsize;
    if (s_bSlice3DTextures)
      glGetIntegerv( GL_MAX_TEXTURE_SIZE, &iMaxsize );
    else
      glGetIntegerv( GL_MAX_3D_TEXTURE_SIZE, &iMaxsize );
    if (iWidth > iMaxsize) 
    {
      iWidth = iMaxsize;
    }
    if (iHeight> iMaxsize) 
    {
      iHeight = iMaxsize;
    }
    
    if (iWidth != image.width() || iHeight != image.height())
      image = image.smoothScale(iWidth, iHeight);
    
    // first build an array of repeated 2D textures...
    QImage inverted(image.mirror());
    pBits = new unsigned char[image.numBytes() * iDepth];
    unsigned int iSliceSize = image.numBytes();
    int bInverted = false;
    int iInvertedCount = 8;
    for (unsigned int iSlice = 0; iSlice < iDepth; ++iSlice)
    { 
      memcpy(&(pBits[iSlice * iSliceSize]), 
        (bInverted) ? inverted.bits() : image.bits(), 
        image.numBytes());
      if (--iInvertedCount <= 0) 
      {
        iInvertedCount = 8;
        bInverted = !bInverted;
      }
    }
  }
  else /// Load from a set of files matching the file pattern
  {
    QFileInfo fi(filename);
    fi.refresh();
    
    QString baseFilename = fi.baseName();
    int truncPos = baseFilename.find(QRegExp("[0-9]"));
    if (truncPos >= 0)
      baseFilename.truncate(truncPos);

    dir.setFilter(QDir::Files);
    dir.setNameFilter(baseFilename + "*." + fi.extension());
    dir.setSorting(QDir::Name);
    QStringList files = dir.entryList();

    bool bFound = true;
    if (files.count() < iDepth)
    {
      bFound = false;
      for ( QStringList::Iterator iter = _texturePaths.begin(); 
          iter != _texturePaths.end(); 
          iter++)
      { 
        dir.setCurrent(*iter);
        files = dir.entryList();
        if (files.count() >= iDepth)
        {
          bFound = true;
          break;
        }
      }
    }
    if (!bFound)
    {
      qWarning("SkyTextureManager::Clone3DTexture: ERROR: could not find %d files matching "
               "%s", iDepth, filename.latin1());
      return false;
    }
    else
    {
      unsigned int iSlice = 0;
      unsigned int  iSliceSize = 0;
      for ( QStringList::Iterator iter = files.begin(); 
            iter != files.end() && iSlice < iDepth; 
            iter++)
      {
        if (image.load(*iter))
        {
          image = QGLWidget::convertToGLFormat(image);
          // make sure it is power of 2 resolution.
          int iWidth  = roundPowerOf2(image.width());
          int iHeight = roundPowerOf2(image.height());
          int iMaxsize;
          if (s_bSlice3DTextures)
            glGetIntegerv( GL_MAX_TEXTURE_SIZE, &iMaxsize );
          else
            glGetIntegerv( GL_MAX_3D_TEXTURE_SIZE, &iMaxsize );
          if (iWidth > iMaxsize) 
          {
            iWidth = iMaxsize;
          }
          if (iHeight> iMaxsize) 
          {
            iHeight = iMaxsize;
          }
          
          if (iWidth != image.width() || iHeight != image.height())
            image = image.smoothScale(iWidth, iHeight);
          if (0 == iSlice)
          {
            pBits       = new unsigned char[image.numBytes() * iDepth];
            iSliceSize  = image.numBytes();
          }
          memcpy(&(pBits[iSlice * iSliceSize]), image.bits(), image.numBytes());
          ++iSlice;
        }
        else
        {
          qWarning("SkyTextureManager::Clone3DTexture: ERROR: could not find %d files matching "
               "%s", iDepth, filename);
          return false;
        }
      }
      
    }
  }

  _Create3DTextureObject(texture, 
                        image.width(), 
                        image.height(),
                        iDepth,
                        GL_RGBA, 
                        pBits);
*/
  return SKYRESULT_FAIL;
}



//! 
/*!  */

//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::GetCubeMapTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::GetCubeMapTexture(const string& filename, SkyTexture& texture, bool bMipmap)
 * @brief Returns a cube map texture object from the texture set
 * 
 * If the texture is already loaded, it is returned.  If it is not, the texture is loaded from file, 
 * added to the texture set, and returned.  If any of the 6 images cannot be loaded, returns an error.  
 * Otherwise returns success.
 */ 
SKYRESULT SkyTextureManager::GetCubeMapTexture( const string& filename, 
                                                SkyTexture&   texture,
                                                bool          bMipmap)
{
  TextureIterator iter = _textures.find(filename);
  
  if (iter != _textures.end())
  { // the texture is already loaded, just return it.
    texture = iter->second;
  }
  else
  { // the texture is being requested for the first time, load and return it
    if (!CloneCubeMapTexture(filename, texture, bMipmap))
      return false;

    _textures.insert(make_pair(filename, texture));  
  }

  return true;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::CloneCubeMapTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::CloneCubeMapTexture(const string& filename, SkyTexture& texture, bool bMipmap)
 * @brief Returns a cube map texture object.
 * 
 * Ignores the texture set.  This always loads the cube map texture, if all 6 face images exist, 
 * creates the texture, and returns it.  If any of the 6 images cannot be loaded, returns an error.  
 * Otherwise returns success.
 */ 
SKYRESULT SkyTextureManager::CloneCubeMapTexture( const string& filename, 
                                                  SkyTexture&   texture,
                                                  bool          bMipmap)
{
  string pathFilename;
  /*QImage  images[6];

  GLenum faces [] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
                      GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
                      GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
                      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
                      GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
                      GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB };
  char* faceNames[] = {"posx", "negx", "posy", "negy", "posz", "negz" };

  for ( QStringList::Iterator iter = _texturePaths.begin(); 
        iter != _texturePaths.end(); 
        iter++)
  { // loop over all texture paths, looking for the filename
    for (int i = 0; i < 6; i++)
    {
      char buffer[FILENAME_MAX];
      sprintf(buffer, filename.ascii(), faceNames[i]);
      pathFilename = (*iter) + "\\" + buffer;

      if (images[i].load(pathFilename))
      {
        images[i] = QGLWidget::convertToGLFormat(images[i]);
      }
      else
        images[i].reset();
    }
  }
    
  for (int i = 0; i < 6; i++)
  {
    if (images[i].isNull())
    {
      char buffer[FILENAME_MAX];
      sprintf(buffer, filename.ascii(), faceNames[i]);
      qWarning("SkyTextureManager::GetTexture(): Could not load image "
                "%s.\n", buffer);
      return false;
    }
  }
    
  glGenTextures(1, &(texture.iTextureID));
  texture.iWidth  = images[0].width();
  texture.iHeight = images[0].height();
  
  // create and bind a cubemap texture object
  glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, texture.iTextureID);
  
  // enable automipmap generation if needed.
  glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_GENERATE_MIPMAP_SGIS, bMipmap);
  if (bMipmap)
    glTexParameterf(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  else
    glTexParameterf(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  for (i = 0; i < 6; i++)
  {
    glTexImage2D(faces[i], 
                 0, 
                 GL_RGBA8, 
                 images[i].width(), 
                 images[i].height(), 
                 0, 
                 GL_RGBA, 
                 GL_UNSIGNED_BYTE, 
                 images[i].bits());
  }*/

  return true;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::_Create2DTextureObject
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::_Create2DTextureObject(SkyTexture &texture, unsigned int iWidth, unsigned int iHeight, unsigned int iFormat, unsigned char *pData)
 * @brief Creates a 2D texture.
 * 
 * Creates an OpenGL texture object and returns its ID and dimensions in a SkyTexture structure.
 */ 
SKYRESULT SkyTextureManager::_Create2DTextureObject(SkyTexture &texture,
                                                    unsigned int iWidth, 
                                                    unsigned int iHeight,
                                                    unsigned int iFormat,
                                                    unsigned char *pData)
{ 
  bool bNew = false;
  unsigned int iTextureID;
  if (!texture.GetID())
  {
    glGenTextures(1, &iTextureID);
    texture.SetID(iTextureID);
    bNew = true;
  }
  texture.SetWidth(iWidth);
  texture.SetHeight(iHeight);

  glBindTexture(GL_TEXTURE_2D, texture.GetID());
    
  if (bNew)
  {
    unsigned int iInternalFormat;
    switch (iFormat)
    {
    case GL_LUMINANCE:
      iInternalFormat = GL_LUMINANCE;
      break;
    case GL_LUMINANCE_ALPHA:
      iInternalFormat = GL_LUMINANCE_ALPHA;
      break;
    default:
      iInternalFormat = GL_RGBA8;
      break;
    }

    glTexImage2D( GL_TEXTURE_2D, 
                  0, 
                  iInternalFormat, 
                  iWidth, iHeight,
                  0, 
                  iFormat,
                  GL_UNSIGNED_BYTE, 
                  pData);
  }
  else
  {
    glTexSubImage2D(GL_TEXTURE_2D,
                    0, 0, 0,
                    iWidth, iHeight, 
                    iFormat, 
                    GL_UNSIGNED_BYTE, 
                    pData);
  }
  // set default filtering.
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::_Create3DTextureObject
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::_Create3DTextureObject(SkyTexture &texture, unsigned int iWidth, unsigned int iHeight, unsigned int iDepth, unsigned int iFormat, unsigned char *pData)
 * @brief Creates a 3D texture
 * 
 * Creates an OpenGL 3D texture object (or a set of 2D slices) and returns its ID and dimensions 
 * in a SkyTexture structure.
 */ 
SKYRESULT SkyTextureManager::_Create3DTextureObject(SkyTexture &texture,
                                                    unsigned int iWidth, 
                                                    unsigned int iHeight,
                                                    unsigned int iDepth,
                                                    unsigned int iFormat,
                                                    unsigned char *pData)
{ 
/*  bool bNew = false;
  if (s_bSlice3DTextures) // create one 2D texture per slice!
  {
    if (!texture.pSliceIDs)
    {
      texture.pSliceIDs = new unsigned int[iDepth];
      glGenTextures(iDepth, texture.pSliceIDs);
      bNew = true;
    }
  }
  else if (!texture.iTextureID)
  {
    glGenTextures(1, &(texture.iTextureID));
    bNew = true;
  }
   
  texture.iWidth    = iWidth;
  texture.iHeight   = iHeight;
  texture.iDepth    = iDepth;
  texture.bSliced3D = s_bSlice3DTextures;

  if (!s_bSlice3DTextures)
  {
    glBindTexture(GL_TEXTURE_3D, texture.iTextureID);
    
    if (bNew)
    {
      unsigned int iInternalFormat;
      switch (iFormat)
      {
      case GL_LUMINANCE:
        iInternalFormat = GL_LUMINANCE;
        break;
      case GL_LUMINANCE_ALPHA:
        iInternalFormat = GL_LUMINANCE_ALPHA;
        break;
      default:
        iInternalFormat = GL_RGBA;
        break;
      }

      glTexImage3D( GL_TEXTURE_3D, 
                    0, 
                    iInternalFormat, 
                    iWidth, iHeight, iDepth,
                    0, 
                    iFormat,
                    GL_UNSIGNED_BYTE, 
                    pData);
    }
    else
    {
      glTexSubImage3D(GL_TEXTURE_3D,
                      0, 0, 0, 0,
                      iWidth, iHeight, iDepth,
                      iFormat, 
                      GL_UNSIGNED_BYTE, 
                      pData);
    }
    // set default filtering.
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  else
  {
    unsigned int iInternalFormat = 0;
    unsigned int iBytesPerPixel  = 0;
    switch (iFormat)
    {
    case GL_LUMINANCE:
      iInternalFormat = GL_LUMINANCE;
      iBytesPerPixel  = 1;
      break;
    case GL_LUMINANCE_ALPHA:
      iInternalFormat = GL_LUMINANCE_ALPHA;
      iBytesPerPixel  = 2;
      break;
    case GL_RGBA:
    default:
      iInternalFormat = GL_RGBA;
      iBytesPerPixel  = 4;
      break;
    }

    unsigned int iSliceSize = iWidth * iHeight * iBytesPerPixel;

    // create iDepth 2D texture slices...
    for (unsigned int iSlice = 0; iSlice < iDepth; ++iSlice)
    {
      glBindTexture(GL_TEXTURE_2D, texture.pSliceIDs[iSlice]);

      if (bNew)
      {
        glTexImage2D( GL_TEXTURE_2D, 
                      0, 
                      iInternalFormat, 
                      iWidth, iHeight,
                      0, 
                      iFormat,
                      GL_UNSIGNED_BYTE, 
                      (pData + iSlice * iSliceSize));   
      }
      else
      {
        glTexSubImage2D(GL_TEXTURE_2D,
                        0, 0, 0,
                        iWidth, iHeight,
                        iFormat, 
                        GL_UNSIGNED_BYTE, 
                        (pData + iSlice * iSliceSize));
      }
      // set default filtering.
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
  } 
  GLVU::CheckForGLError("SkyTextureManager::_Create3DTextureObject()");

  return SKYRESULT_OK;*/
  return SKYRESULT_FAIL;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTextureManager::DestroyTextureObject
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyTextureManager::DestroyTextureObject(SkyTexture &texture)
 * @brief destroys a SkyTexture object.
 * 
 * Deletes the data as well as the OpenGL texture ID(s).
 */ 
void SkyTextureManager::DestroyTextureObject(SkyTexture &texture)
{ /*
  if (texture.GetID)
    glDeleteTextures(1, &(texture.iTextureID));
  if (texture.bSliced3D && texture.pSliceIDs)
  {
    glDeleteTextures(texture.iDepth, texture.pSliceIDs);
    delete [] texture.pSliceIDs;
  } */
}
