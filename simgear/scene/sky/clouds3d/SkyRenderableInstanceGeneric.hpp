//------------------------------------------------------------------------------
// File : SkyRenderableInstanceGeneric.hpp
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
 * @file SkyRenderableInstanceGeneric.hpp
 * 
 * Interface for a basic implementation of SkyRenderableInstance
 */
#ifndef __SKYRENDERABLEINSTANCEGENERIC_HPP__
#define __SKYRENDERABLEINSTANCEGENERIC_HPP__

#include "SkyRenderableInstance.hpp"

// forward to reduce unnecessary dependencies
class SkyRenderable;
class SkyMinMaxBox;

//------------------------------------------------------------------------------
/**
 * @class SkyRenderableInstanceGeneric
 * @brief A generic renderable instance
 * 
 * The SkyRenderableInstanceGeneric is a basic implementation of the base class.
 * For view frustum culling, the function ViewFrustumCull should be called once
 * per frame, at which point a flag is set if the object is culled or not.  It
 * is possible that the object is then queried multiple times if it is culled or
 * not by various other objects before being displayed, that is why the flag is
 * stored.
 */
class SkyRenderableInstanceGeneric : public SkyRenderableInstance
{
public:
  SkyRenderableInstanceGeneric(SkyRenderable *pObject);
  SkyRenderableInstanceGeneric(SkyRenderable *pObject, 
                               const Vec3f   &position, 
                               const Mat33f  &rotation, 
                               const float   scale);
  virtual ~SkyRenderableInstanceGeneric();

  // Setters / Getters

  virtual void SetRenderable(SkyRenderable *pRenderable );
                                                      
  //! Returns a pointer to the renderable that this instance represents.
  virtual SkyRenderable* GetRenderable() const     { return _pObj;       }

  virtual SKYRESULT      Display();

  // Test / Set / Get
  virtual bool           ViewFrustumCull( const Camera &cam );

  
  virtual SkyMinMaxBox*  GetBoundingVolume() const { return _pBV;        }

protected:

protected:
  SkyRenderable     *_pObj;         // Pointer to the renderable object
  SkyMinMaxBox      *_pBV;          // Pointer to bounding volume
};

#endif //__SKYRENDERABLEINSTANCEGENERIC_HPP__