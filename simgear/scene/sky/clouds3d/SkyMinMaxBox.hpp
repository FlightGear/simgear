//------------------------------------------------------------------------------
// File : SkyMinMaxBox.hpp
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
* @file SkyMinMaxBox.hpp
* 
* Interface definition for a min-max bounding box class for bounding volume hierarchies.
*/
#ifndef __SKYMINMAXBOX_HPP__
#define __SKYMINMAXBOX_HPP__

#include "SkyBoundingVolume.hpp"

//------------------------------------------------------------------------------
/**
* @class SkyMinMaxBox
* @brief An AABB class that can be used in bounding volume hierarchies.
* 
* @todo <WRITE EXTENDED CLASS DESCRIPTION>
*/
class SkyMinMaxBox : public SkyBoundingVolume
{
public:
  SkyMinMaxBox();
  //! Destructor
  virtual ~SkyMinMaxBox() {}
  
  void  AddPoint( const Vec3f &pt );
  void  AddPoint( float x , float y , float z );
  
  //! Expand this box to contain @a box.
  void  Union(const SkyMinMaxBox& box)           { AddPoint(box.GetMin()); AddPoint(box.GetMax()); }

  //! Returns the minimum corner of the bounding box.
  const Vec3f &GetMin() const                   { return _min; }
  //! Returns the maximum corner of the bounding box.
  const Vec3f &GetMax() const                   { return _max; }
  
  //! Sets the minimum corner of the bounding box.
  void  SetMin(const Vec3f &min)                { _min = min; _UpdateSphere(); }
  //! Sets the maximum corner of the bounding box.
  void  SetMax(const Vec3f &max)                { _max = max; _UpdateSphere(); }
  
  //! Returns the X width of the bounding box.
  float GetWidthInX() const                     { return _max.x - _min.x;}
  //! Returns the Y width of the bounding box.
  float GetWidthInY() const                     { return _max.y - _min.y;}
  //! Returns the Z width of the bounding box.
  float GetWidthInZ() const                     { return _max.z - _min.z;}
  
  bool  PointInBBox( const Vec3f &pt ) const;
  
  bool  ViewFrustumCull( const Camera &cam, const Mat44f &mat );

  void  Transform(const Mat44f& mat);
  
  // Reset the bounding box
  void  Clear();
  
  void  Display() const;
  
protected:
  void _UpdateSphere();
  void _CalcVerts(Vec3f pVerts[8]) const;

private:
	Vec3f _min;   // Original object space BV
	Vec3f _max;
};

#endif //__SKYMINMAXBOX_HPP__