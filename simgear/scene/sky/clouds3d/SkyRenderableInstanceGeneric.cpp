//------------------------------------------------------------------------------
// File : SkyRenderableInstanceGeneric.cpp
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
 * @file SkyRenderableInstanceGeneric.cpp
 * 
 * A basic implementation of SkyRenderableInstance
 */

#include "glvu.hpp"
#include "SkyUtil.hpp"
#include "SkyRenderable.hpp"
#include "SkyMinMaxBox.hpp"
#include "SkyRenderableInstanceGeneric.hpp"

//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGeneric::SkyRenderableInstanceGeneric
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGeneric::SkyRenderableInstanceGeneric(SkyRenderable *object)
 * @brief Constructor, store the renderable and set the position to identity
 */ 
SkyRenderableInstanceGeneric::SkyRenderableInstanceGeneric(SkyRenderable *pObject)
: SkyRenderableInstance(),
  _pObj(pObject)
{
  _pBV = pObject->CopyBoundingVolume();
}

//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGeneric::SkyRenderableInstanceGeneric
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGeneric::SkyRenderableInstanceGeneric(SkyRenderable *object, const Vec3f &position, const Mat33f &rotation, const float scale)
 * @brief Constructor, stores the instance information given
 */ 
SkyRenderableInstanceGeneric::SkyRenderableInstanceGeneric(SkyRenderable *pObject, 
                                                           const Vec3f   &position, 
                                                           const Mat33f  &rotation, 
                                                           const float   scale)
: SkyRenderableInstance(position, rotation, scale),
  _pObj(pObject)
{
  _pBV = pObject->CopyBoundingVolume();
}

//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGeneric::~SkyRenderableInstanceGeneric
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGeneric::~SkyRenderableInstanceGeneric()
 * @brief Destructor
 */ 
SkyRenderableInstanceGeneric::~SkyRenderableInstanceGeneric()
{
  _pObj = NULL;
  SAFE_DELETE(_pBV);
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGeneric::SetRenderable
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGeneric::SetRenderable(SkyRenderable *pRenderable)
 * @brief Set the renderable for this instance.
 */ 
void SkyRenderableInstanceGeneric::SetRenderable(SkyRenderable *pRenderable) 
{ 
  _pObj = pRenderable; 
  SAFE_DELETE(_pBV);
  _pBV  = pRenderable->CopyBoundingVolume();
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGeneric::Display
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGeneric::Display()
 * @brief Displays the instance by calling the renderable's display function
 */ 
SKYRESULT SkyRenderableInstanceGeneric::Display()
{
  // Get and set the world space transformation
  Mat44f mat;
  GetModelToWorldTransform(mat);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMultMatrixf(mat.M);
 
  //FAIL_RETURN_MSG(_pObj->Display(*(GLVU::GetCurrent()->GetCurrentCam()), this), 
    //              "SkyRenderableInstanceGeneric:Display(), error returned from object's display");

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGeneric::ViewFrustumCull
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGeneric::ViewFrustumCull(const Camera &cam)
 * @brief View frustum cull the object given its world position
 */ 
bool SkyRenderableInstanceGeneric::ViewFrustumCull(const Camera &cam)
{
  Mat44f xform;
  GetModelToWorldTransform(xform);
  _bCulled = (_pBV == NULL) ? false : _pBV->ViewFrustumCull(cam, xform);
  return _bCulled;
}
