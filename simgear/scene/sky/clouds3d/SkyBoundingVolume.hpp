//------------------------------------------------------------------------------
// File : SkyBoundingVolume.hpp
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
 * @file SkyBoundingVolume.hpp
 * 
 * Base class interface definition for a bounding volume.
 */
#ifndef __SKYBOUNDINGVOLUME_HPP__
#define __SKYBOUNDINGVOLUME_HPP__

#include <vec3f.hpp>
#include <mat44.hpp>

// forward to reduce unnecessary dependencies
class Camera;

//------------------------------------------------------------------------------
/**
 * @class SkyBoundingVolume
 * @brief An abstract base class for bounding volumes (AABB,OBB,Sphere,etc.)
 *
 * This base class maintains a center and a radius, so it is effectively a bounding
 * sphere.  Derived classes may represent other types of bounding volumes, but they 
 * should be sure to update the radius and center, because some objects will treat
 * all bounding volumes as spheres.
 *
 */
class SkyBoundingVolume
{
public:
  //! Constructor
  SkyBoundingVolume() : _vecCenter(0, 0, 0), _rRadius(0) {}
  //! Destructor
  virtual ~SkyBoundingVolume() {}


  //------------------------------------------------------------------------------
  // Function     	  : SetCenter
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn SetCenter(const Vec3f &center)
   * @brief Sets the center of the bounding volume.
   */ 
  virtual void SetCenter(const Vec3f &center)               { _vecCenter = center; }
  

  //------------------------------------------------------------------------------
  // Function     	  : SetRadius
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn SetRadius(float radius)
   * @brief Sets the radius of the bounding volume.
   */ 
  virtual void SetRadius(float radius)                      { _rRadius = radius;   }


  //------------------------------------------------------------------------------
  // Function     	  : Vec3f& GetCenter
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn Vec3f& GetCenter() const
   * @brief Returns the center of the bounding volume.
   */ 
  virtual const Vec3f& GetCenter() const                    { return _vecCenter;   }


  //------------------------------------------------------------------------------
  // Function     	  : GetRadius
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn GetRadius() const
   * @brief Returns the radius ofthe bounding volume.
   */ 
  virtual float GetRadius() const                           { return _rRadius;     }

  //------------------------------------------------------------------------------
  // Function     	  : ViewFrustumCull
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
   * @fn ViewFrustumCull( const Camera &cam, const Mat44f &mat )
   * @brief Cull a bounding volume.
   * 
   * Returns false if the bounding volume is entirely outside the camera's frustum,
   * true otherwise.
   */ 
  virtual bool ViewFrustumCull( const Camera &cam, const Mat44f &mat ) = 0;

  //------------------------------------------------------------------------------
  // Function     	  : AddPoint
  // Description	    : 
  //------------------------------------------------------------------------------
  /*
   * @fn AddPoint( const Vec3f &pt )
   * @brief Add a point to a bounding volume.
   * 
   * One way to create a bounding volume is to add points.  What should/could happen 
   * is that the BV will store an object space BV and then when a call to SetPosition 
   * is called, the stored values will be transformed and stored separately, so that 
   * the original values always exist.
   *
   */ 
  //virtual void AddPoint( const Vec3f &point )  = 0;
  
  //------------------------------------------------------------------------------
  // Function     	  : AddPoint
  // Description	    : 
  //------------------------------------------------------------------------------
  /*
   * @fn AddPoint( float x, float y, float z )
   * @brief Add a point to a bounding volume.
   * 
   * @see AddPoint(const Vec3f &pt)
   */ 
  //virtual void AddPoint( float x, float y, float z ) = 0;

  //------------------------------------------------------------------------------
  // Function     	  : Clear
  // Description	    : 
  //------------------------------------------------------------------------------
  /*
   * @fn Clear()
   * @brief Clear all data from the bounding volume.
   */ 
  //virtual void Clear() = 0;

protected:
  Vec3f        _vecCenter;
  float        _rRadius;
};

#endif //__SKYBOUNDINGVOLUME_HPP__
