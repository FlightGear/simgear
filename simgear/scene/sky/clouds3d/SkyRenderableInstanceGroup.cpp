//------------------------------------------------------------------------------
// File : SkyRenderableInstanceGroup.cpp
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
 * @file SkyRenderableInstanceGroup.cpp
 * 
 * Implementation of class SkyRenderableInstanceGroup, an instance that groups
 * other instances.
 */

#include <GL/glut.h>
#include "SkyRenderableInstanceGroup.hpp"
#include "SkySceneManager.hpp"


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGroup::SkyRenderableInstanceGroup
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGroup::SkyRenderableInstanceGroup()
 * @brief Constructor.
 */ 
SkyRenderableInstanceGroup::SkyRenderableInstanceGroup()
: SkyRenderableInstance(),
  _pObjectSpaceBV(NULL) 
{
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGroup::~SkyRenderableInstanceGroup
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGroup::~SkyRenderableInstanceGroup()
 * @brief Destructor.
 */ 
SkyRenderableInstanceGroup::~SkyRenderableInstanceGroup()
{
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGroup::Update
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGroup::Update(const Camera &cam)
 * @brief Processes any per frame updates the instance requires.
 * 
 * This method simply calls the SkyRenderableInstance::Update() method of each of
 * its sub-instances.
 */ 
SKYRESULT SkyRenderableInstanceGroup::Update(const Camera &cam)
{
  InstanceIterator ii;
  for (ii = _opaqueSubInstances.begin(); ii != _opaqueSubInstances.end(); ++ii)
  {
    FAIL_RETURN((*ii)->Update(cam));
  }
  for (ii = _transparentSubInstances.begin(); ii != _transparentSubInstances.end(); ++ii)
  {
    FAIL_RETURN((*ii)->Update(cam));
  }

  SkySceneManager::SortInstances(_transparentSubInstances, cam.Orig);
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGroup::Display
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGroup::Display()
 * @brief Displays all sub-instances of this instance.
 * 
 * The object-to-world transform of this instance group will be applied to all sub-instances before
 * their own object-to-world transforms are applied.
 */ 
SKYRESULT SkyRenderableInstanceGroup::Display()
{
  // Get and set the world space transformation
  Mat44f mat;
  GetModelToWorldTransform(mat);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMultMatrixf(mat.M);

  InstanceIterator ii;
  /***
  for (ii = _opaqueSubInstances.begin(); ii != _opaqueSubInstances.end(); ++ii)
  {
    FAIL_RETURN((*ii)->Display());
  }

  for (ii = _transparentSubInstances.begin(); ii != _transparentSubInstances.end(); ++ii)
  {
    FAIL_RETURN((*ii)->Display());
  }
***/
  _pObjectSpaceBV->Display();

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGroup::ViewFrustumCull
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGroup::ViewFrustumCull(const Camera& cam)
 * @brief @todo <WRITE BRIEF SkyRenderableInstanceGroup::ViewFrustumCull DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyRenderableInstanceGroup::ViewFrustumCull FUNCTION DOCUMENTATION>
 */ 
bool SkyRenderableInstanceGroup::ViewFrustumCull(const Camera& cam)
{
  Mat44f xform;
  GetModelToWorldTransform(xform);
  _bCulled = (_pObjectSpaceBV == NULL) ? false : _pObjectSpaceBV->ViewFrustumCull(cam, xform);
  return _bCulled;
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceGroup::AddSubInstance
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceGroup::AddSubInstance(SkyRenderableInstance *pInstance, bool bTransparent)
 * @brief Adds a sub-instance to the group.
 */ 
void SkyRenderableInstanceGroup::AddSubInstance(SkyRenderableInstance *pInstance, bool bTransparent)
{
  if (!bTransparent)
    _opaqueSubInstances.push_back(pInstance);
  else
    _transparentSubInstances.push_back(pInstance);

  // update the bounds...
  Mat44f xform;
  pInstance->GetModelToWorldTransform(xform);

  SkyMinMaxBox *pBV = pInstance->GetBoundingVolume();
  if (pBV)
  {
    Vec3f min = pInstance->GetBoundingVolume()->GetMin();
    Vec3f max = pInstance->GetBoundingVolume()->GetMax();

    if (!_pObjectSpaceBV)
      _pObjectSpaceBV = new SkyMinMaxBox;

    _pObjectSpaceBV->AddPoint(xform * min);
    _pObjectSpaceBV->AddPoint(xform * max);
  } 
}
