//------------------------------------------------------------------------------
// File : SkyRenderable.hpp
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
//------------------------------------------------------------------------------
// File : SkyRenderable.hpp
//------------------------------------------------------------------------------
// Sky  : Copyright 2002 Mark J. Harris and Andrew Zaferakis
//------------------------------------------------------------------------------
/**
 * @file SkyRenderable.hpp
 *
 * Abstract base class definition for SkyRenderable, a renderable object class.
 */ 
#ifndef __SKYRENDERABLE_HPP__
#define __SKYRENDERABLE_HPP__

#pragma warning( disable : 4786)

#include <string>

#include "SkyUtil.hpp"

// forward to reduce unnecessary dependencies
class SkyMinMaxBox;
class SkyRenderableInstance;
class Camera;

//------------------------------------------------------------------------------
/**
 * @class SkyRenderable
 * @brief An base class for renderable objects.
 * 
 * Each SkyRenderable object should know how to Display itself, however some
 * objects may not have a bounding volume that is useful (skybox, etc.)
 */
class SkyRenderable
{
public:
  //! Constructor
  SkyRenderable() {}
  //! Destructor
  virtual ~SkyRenderable() { }

  //------------------------------------------------------------------------------
  // Function     	  :                      SetName
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
  * @fn                      SetName(const std::string &name)
  * @brief Set a name for this renderable.
  */ 
  void                        SetName(const std::string &name) { _name = name; }

  //------------------------------------------------------------------------------
  // Function     	  :         GetName
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
  * @fn         GetName() const
  * @brief Get the name of this renderable.
  */ 
  const std::string&          GetName() const                  { return _name; }
  
  //------------------------------------------------------------------------------
  // Function     	  : Update
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Update(const Camera &cam, SkyRenderableInstance *pInstance)
   * @brief Update the state of the renderable.
   * 
   * This method is optional, as some renderables will need periodic updates
   * (i.e. for animation) and others will not.
   */ 
  virtual SKYRESULT           Update(const Camera &cam, SkyRenderableInstance *pInstance = NULL)
                              { return SKYRESULT_OK;  }
  
  //------------------------------------------------------------------------------
  // Function     	  : Display
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Display(const Camera &cam, SkyRenderableInstance *pInstance)
   * @brief Display the object.
   */ 
  virtual SKYRESULT           Display(const Camera &cam, SkyRenderableInstance *pInstance = NULL) = 0;

  //------------------------------------------------------------------------------
  // Function     	  : CopyBoundingVolume
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn CopyBoundingVolume() const
   * @brief Create a copy of the object's bounding volume, useful for collision, VFC, etc.
   */ 
  virtual SkyMinMaxBox*       CopyBoundingVolume() const = 0;// { return 0; }

protected:
  std::string         _name; // the name of this renderable.
};

#endif //__SKYRENDERABLE_HPP__
