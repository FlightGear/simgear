//------------------------------------------------------------------------------
// File : SkyRenderableInstanceGroup.hpp
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
 * @file SkyRenderableInstanceGroup.hpp
 * 
 * Interface definition for class SkyRenderableInstanceGroup, an instance that groups
 * other instances.
 */
#ifndef __SKYRENDERABLEINSTANCEGROUP_HPP__
#define __SKYRENDERABLEINSTANCEGROUP_HPP__

#include "SkyRenderableInstance.hpp"
#include "SkyMinMaxBox.hpp"

//------------------------------------------------------------------------------
/**
 * @class SkyRenderableInstanceGroup
 * @brief A renderable instance that groups other instances.
 * 
 * This class provides a very basic way to implement static hierarchies of objects.
 * It is not meant to be a full scene graph -- 
 */
class SkyRenderableInstanceGroup : public SkyRenderableInstance
{
public:
	SkyRenderableInstanceGroup();
	virtual ~SkyRenderableInstanceGroup();

  //! Update all sub-instances.
  virtual SKYRESULT Update(const Camera &cam);
  //! Render all sub-instances.
  virtual SKYRESULT Display();

  //! Returns true if and only if the bounding volume of this instance lies entirely outside @a cam.  
  virtual bool      ViewFrustumCull(const Camera &cam);
  
  //! Adds an instance to the group that this instance represents.
  void              AddSubInstance(SkyRenderableInstance *pInstance, bool bTransparent);

protected:
  InstanceArray _opaqueSubInstances;
  InstanceArray _transparentSubInstances;
  
  SkyMinMaxBox  *_pObjectSpaceBV; // Pointer to bounding volume in object space
};


#endif //__SKYRENDERABLEINSTANCEGROUP_HPP__
