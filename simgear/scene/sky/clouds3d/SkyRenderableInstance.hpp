//------------------------------------------------------------------------------
// File : SkyRenderableInstance.hpp
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
 * @file SkyRenderableInstance.hpp
 * 
 * Interface definition for SkyRenderableInstance, an instance of a renderable object.
 */
#ifndef __SKYRENDERABLEINSTANCE_HPP__
#define __SKYRENDERABLEINSTANCE_HPP__

#include <vector>
#include <mat33.hpp>
#include <mat44.hpp>
#include "SkyUtil.hpp"

// forward to reduce unnecessary dependencies
class Camera;
class SkyMinMaxBox;

// forward so we can make the following typedefs easily visible in the header.
// rather than buried under the class definition.
class SkyRenderableInstance;

//! A dynamic array of SkyRenderableInstance pointers.
typedef std::vector<SkyRenderableInstance*>       InstanceArray;
//! An instance array iterator.
typedef InstanceArray::iterator                   InstanceIterator;

//------------------------------------------------------------------------------
/**
* @class SkyRenderableInstance
* @brief An instance of a SkyRenderable object.
* 
* An instance contains a pointer to a SkyRenderable object.  The
* instance contains attributes such as position, orientation,
* scale, etc. that vary between instances.
*/
class SkyRenderableInstance
{
public:
  //! Constructor.
  SkyRenderableInstance() 
  : _bCulled(false), _bAlive(true), _vecPosition(0, 0, 0), _rScale(1), _rSquareSortDistance(0)
  { 
    _matRotation.Identity(); _matInvRotation.Identity(); 
  }

  //! Constructor.
  SkyRenderableInstance(const Vec3f   &position, 
                        const Mat33f  &rotation, 
                        const float   scale)
  : _bCulled(false), _bAlive(true), _vecPosition(position), 
    _matRotation(rotation), _rScale(scale), _rSquareSortDistance(0)
  {
    _matInvRotation = _matRotation;
    _matInvRotation.Transpose();
  }

  //! Destructor
  virtual ~SkyRenderableInstance() {}
   
  // Setters / Getters                                                      
  //! Set the world space position of the instance.
  virtual void SetPosition(const Vec3f  &position)     { _vecPosition = position; }
  //! Set the world space rotation of the instance.
  virtual void SetRotation(const Mat33f &rotation)     { _matRotation    = rotation;  
                                                         _matInvRotation = rotation; 
                                                         _matInvRotation.Transpose(); }
  //! Set the world space scale of the instance.
  virtual void SetScale(     const float  &scale)      { _rScale    = scale;  }
  
  //! Returns the world space position of the instance.
  virtual const Vec3f&   GetPosition() const           { return _vecPosition; }
  //! Returns the world space rotation matrix of the instance.
  virtual const Mat33f&  GetRotation() const           { return _matRotation; }
  //!  Returns the inverse of the world space rotation matrix of the instance.
  virtual const Mat33f&  GetInverseRotation() const    { return _matInvRotation; }
  //! Returns the world space scale of the instance.
  virtual float          GetScale() const              { return _rScale;      }
  
  //! Update the instance based on the given camera, @a cam.
  virtual SKYRESULT Update(const Camera &cam)          { return SKYRESULT_OK; }
  //! Render the instance.
  virtual SKYRESULT Display()                          { return SKYRESULT_OK; }
  
  //! Returns the transform matrix from model space to world space.
  inline virtual void  GetModelToWorldTransform(Mat44f &mat) const;
  
  //! Returns the transform matrix from world space to model space.

  inline virtual void  GetWorldToModelTransform(Mat44f &mat) const;

  //! Returns the object-space bounding volume for this instance, or NULL if none is available.
  virtual SkyMinMaxBox* GetBoundingVolume() const      { return NULL;         }
   
  //! Returns true if and only if the bounding volume of this instance lies entirely outside @a cam.  
  virtual bool  ViewFrustumCull(const Camera &cam)     { return false;        }
  //! Returns true if the instance was culled.
  virtual bool  IsCulled()                             { return _bCulled;     }
  //! Sets the culled state of the instance.
  virtual void  SetCulled(bool bCulled)                { _bCulled = bCulled;  }
  
  //! Returns true if the instance is currently active.
  virtual bool  IsAlive()                              { return _bAlive;      }
  //! Activates or deactivates the instance.
  virtual void  SetIsAlive(bool bAlive)                { _bAlive = bAlive;    }

  //! Sets the distance of this object from the sort position.  Used to sort instances.
  virtual void  SetSquareSortDistance(float rSqrDist)  { _rSquareSortDistance = rSqrDist; }
  //! Returns the distance of this object from the sort position. (Set with SetSquareSortDistance())
  virtual float GetSquareSortDistace() const           { return _rSquareSortDistance;     }
  
  //! This operator is used to sort instance arrays. 
  bool operator<(const SkyRenderableInstance& instance) const
  {
    return (_rSquareSortDistance > instance._rSquareSortDistance);
  }

protected:
  bool          _bCulled;        // Culled flag
  bool          _bAlive;         // Alive object flag  

  Vec3f         _vecPosition;    // Position
  Mat33f        _matRotation;    // Rotation
  Mat33f        _matInvRotation; // inverse rotation
  float         _rScale;         // Scale

  // for sorting particles during shading
  float         _rSquareSortDistance;
};

//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstance::GetModelToWorldTransform
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstance::GetModelToWorldTransform(Mat44f &mat) const
 * @brief Returns the 4x4 transformation matrix from world to model space.
 */ 
inline void SkyRenderableInstance::GetModelToWorldTransform(Mat44f &mat) const
{
  mat[0]  = _matRotation.M[0]; mat[4]  = _matRotation.M[3]; 
  mat[8]  = _matRotation.M[6]; mat[12] = 0;
  mat[1]  = _matRotation.M[1]; mat[5]  = _matRotation.M[4]; 
  mat[9]  = _matRotation.M[7]; mat[13] = 0;
  mat[2]  = _matRotation.M[2]; mat[6]  = _matRotation.M[5]; 
  mat[10] = _matRotation.M[8]; mat[14] = 0;
  mat[3]  = 0; mat[7] = 0; mat[11] = 0; mat[15] = 0;

  // Scale the matrix (we don't want to scale translation or mat[15] which is 1)
  if (_rScale != 1)
    mat *= _rScale;

  // Set the translation and w coordinate after the potential scaling
  mat[12] = _vecPosition.x; mat[13] = _vecPosition.y; mat[14] = _vecPosition.z;    
  mat[15] = 1;
}


//------------------------------------------------------------------------------
// Function     	  : Mat44f& SkyRenderableInstance::GetWorldToModelTransform
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstance::GetWorldToModelTransform(Mat44f &mat) const
 * @brief Returns the 4x4 transformation matrix from world to model space.
 */ 
inline void SkyRenderableInstance::GetWorldToModelTransform(Mat44f &mat) const
{
  mat[0]  = _matRotation.M[0]; mat[4]  = _matRotation.M[1]; 
  mat[8]  = _matRotation.M[2]; mat[12] = 0;
  mat[1]  = _matRotation.M[3]; mat[5]  = _matRotation.M[4]; 
  mat[9]  = _matRotation.M[5]; mat[13] = 0;
  mat[2]  = _matRotation.M[6]; mat[6]  = _matRotation.M[7]; 
  mat[10] = _matRotation.M[8]; mat[14] = 0;
  mat[3]  = 0; mat[7] = 0; mat[11] = 0; mat[15] = 0;

  // Scale the matrix (we don't want to scale translation or mat[15] which is 1)
  if (_rScale != 1)
    mat *= (1 / _rScale);

  // Set the translation and w coordinate after the potential scaling
  mat[12] = -_vecPosition.x; mat[13] = -_vecPosition.y; mat[14] = -_vecPosition.z;    
  mat[15] = 1;
}

#endif //__SKYRENDERABLEINSTANCE_HPP__
