//------------------------------------------------------------------------------
// File : SkyMinMaxBox.cpp
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
 * @file SkyMinMaxBox.cpp
 * 
 * Implementation of a bounding box class.  Modified from Wes Hunt's BoundingBox.
 */
#include "SkyMinMaxBox.hpp"
#include "camutils.hpp"
#include <GL/glut.h>
#include <float.h>


//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::SkyMinMaxBox
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMinMaxBox::SkyMinMaxBox()
 * @brief Constructor
 */ 
SkyMinMaxBox::SkyMinMaxBox()
{
  Clear();
}


//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::Clear
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMinMaxBox::Clear()
 * @brief Reset the min and max to floating point extremes.
 * 
 */ 
void SkyMinMaxBox::Clear()
{
	_min.x =  FLT_MAX;
	_min.y =  FLT_MAX;
	_min.z =  FLT_MAX;
	_max.x = -FLT_MAX;
	_max.y = -FLT_MAX;
	_max.z = -FLT_MAX;
}


//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::PointInBBox
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMinMaxBox::PointInBBox( const Vec3f &pt ) const
 * @brief Queries pt to see if it is inside the SkyMinMaxBox.
 * 
 */ 
bool SkyMinMaxBox::PointInBBox( const Vec3f &pt ) const
{
	if( (pt.x >= _min.x) && ( pt.x <= _max.x ) )
	{
		if( (pt.y >= _min.y) && ( pt.y <= _max.y ) )
		{
			if( (pt.z >= _min.z) && ( pt.z <= _max.z ) )
				return true;
		}
	}

	return false;
}


//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::AddPoint
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMinMaxBox::AddPoint( float x , float y , float z )
 * @brief Adds a point and adjusts bounds if necessary.             
 */ 
void SkyMinMaxBox::AddPoint( float x , float y , float z )
{
	if( x > _max.x )
		_max.x = x;
	if( x < _min.x )
		_min.x = x;
	
	if( y > _max.y )
		_max.y = y;
	if( y < _min.y )
		_min.y = y;

	if( z > _max.z )
		_max.z = z;
	if( z < _min.z )
		_min.z = z;

  // update the center and radius
  _UpdateSphere();
}


//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::AddPoint
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMinMaxBox::AddPoint( const Vec3f &pt )
 * @brief Adds a point and adjusts bounds if necessary.             
 */ 
void SkyMinMaxBox::AddPoint( const Vec3f &pt )
{
	if( pt.x > _max.x )
		_max.x = pt.x;
	if( pt.x < _min.x )
		_min.x = pt.x;
	
	if( pt.y > _max.y )
		_max.y = pt.y;
	if( pt.y < _min.y )
		_min.y = pt.y;

	if( pt.z > _max.z )
		_max.z = pt.z;
	if( pt.z < _min.z )
		_min.z = pt.z;

  // update the center and radius
  _UpdateSphere();
}


//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::ViewFrustumCull
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMinMaxBox::ViewFrustumCull( const Camera &cam, const Mat44f &mat )
 * @brief Returns true if bounding volume culled against cam.
 * 
 * This function must transform the object space min and max then adjust the new 
 * min and max box by expanding it.  Each of the 8 points must be tested.  This 
 * is faster then doing an xform of all the geometry points and finding a tight
 * fitting min max box, however this will be enlarged.
 */ 
bool SkyMinMaxBox::ViewFrustumCull( const Camera &cam, const Mat44f &mat )
{
  SkyMinMaxBox xBV;                // Xformed Bounding Volume
  Vec3f xMin   = mat * _min;      // Xformed _min
  Vec3f xMax   = mat * _max;      // Xformed _max
  Vec3f offset = _max - _min;     // Offset for sides of MinMaxBox
  Vec3f tmp;

  xBV.Clear(); // Clear the values first

  // First find the new minimum x,y,z
  // Find min + x
  tmp.Set(mat.M[0], mat.M[4], mat.M[8]);
  tmp *= offset.x;
  tmp += xMin;
  xBV.AddPoint(tmp);

  // Find min + y
  tmp.Set(mat.M[1], mat.M[5], mat.M[9]);
  tmp *= offset.y;
  tmp += xMin;
  xBV.AddPoint(tmp);

  // Find min + z
  tmp.Set(mat.M[3], mat.M[6], mat.M[10]);
  tmp *= offset.z;
  tmp += xMin;
  xBV.AddPoint(tmp);
  
  // Second find the new maximum x,y,z
  // Find max - x
  tmp.Set(mat.M[0], mat.M[4], mat.M[8]);
  tmp *= -offset.x;
  tmp += xMax;
  xBV.AddPoint(tmp);
  
  // Find max - y
  tmp.Set(mat.M[1], mat.M[5], mat.M[9]);
  tmp *= -offset.y;
  tmp += xMax;
  xBV.AddPoint(tmp);
  
  // Find max - z
  tmp.Set(mat.M[3], mat.M[6], mat.M[10]);
  tmp *= -offset.z;
  tmp += xMax;
  xBV.AddPoint(tmp);
  
  // Use the camera utility function that already exists for minmax boxes
  return VFC(&cam, xBV.GetMin(), xBV.GetMax());
}


//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::Transform
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMinMaxBox::Transform(const Mat44f& mat)
 * @brief @todo <WRITE BRIEF SkyMinMaxBox::Transform DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyMinMaxBox::Transform FUNCTION DOCUMENTATION>
 */ 
void SkyMinMaxBox::Transform(const Mat44f& mat)
{
  Vec3f verts[8];
  _CalcVerts(verts);
  Clear();
  for (int i = 0; i < 8; ++i)
  {
    AddPoint(mat * verts[i]);
  }
}

//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::_UpdateSphere
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyMinMaxBox::_UpdateSphere()
* @brief Updates the bounding sphere based on min and max.
*/ 
void SkyMinMaxBox::_UpdateSphere()
{
  _vecCenter =  _min;
  _vecCenter += _max;
  _vecCenter *= 0.5f;
  
  Vec3f rad  =  _max;
  rad        -= _vecCenter;
  _rRadius   =  rad.Length();
}



//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::Display
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMinMaxBox::Display() const
 * @brief @todo <WRITE BRIEF SkyMinMaxBox::Display DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyMinMaxBox::Display FUNCTION DOCUMENTATION>
 */ 
void SkyMinMaxBox::Display() const
{
  Vec3f V[8];
  _CalcVerts(V);
  
  glPushAttrib(GL_LINE_BIT);
  glLineWidth(1.0);
  
  glBegin(GL_LINE_LOOP);  // TOP FACE
  glVertex3fv(V[4]); glVertex3fv(V[5]); glVertex3fv(V[1]); glVertex3fv(V[0]);
  glEnd();
  glBegin(GL_LINE_LOOP);  // BOTTOM FACE
  glVertex3fv(V[3]); glVertex3fv(V[2]); glVertex3fv(V[6]); glVertex3fv(V[7]); 
  glEnd();
  glBegin(GL_LINE_LOOP);  // LEFT FACE
  glVertex3fv(V[1]); glVertex3fv(V[5]); glVertex3fv(V[6]); glVertex3fv(V[2]); 
  glEnd();
  glBegin(GL_LINE_LOOP);  // RIGHT FACE
  glVertex3fv(V[0]); glVertex3fv(V[3]); glVertex3fv(V[7]); glVertex3fv(V[4]); 
  glEnd();
  glBegin(GL_LINE_LOOP);  // NEAR FACE
  glVertex3fv(V[1]); glVertex3fv(V[2]); glVertex3fv(V[3]); glVertex3fv(V[0]); 
  glEnd();
  glBegin(GL_LINE_LOOP);  // FAR FACE
  glVertex3fv(V[4]); glVertex3fv(V[7]); glVertex3fv(V[6]); glVertex3fv(V[5]); 
  glEnd();
  
  glPopAttrib();
}

//-----------------------------------------------------------------------------
// Calculates the eight corner vertices of the MinMaxBox.
// V must be prealloced.
//       5---4
//      /   /|
//     1---0 |   VERTS : 0=RTN,1=LTN,2=LBN,3=RBN,4=RTF,5=LTF,6=LBF,7=RBF
//     |   | 7   (L,R, B,T, N,F) = (Left,Right, Bottom,Top, Near,Far)
//     |   |/
//     2---3
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function     	  : SkyMinMaxBox::_CalcVerts
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMinMaxBox::_CalcVerts(Vec3f pVerts[8]) const
 * @brief @todo <WRITE BRIEF SkyMinMaxBox::_CalcVerts DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyMinMaxBox::_CalcVerts FUNCTION DOCUMENTATION>
 */ 
void SkyMinMaxBox::_CalcVerts(Vec3f pVerts[8]) const
{
  pVerts[0].Set(_max); pVerts[4].Set(_max.x, _max.y, _min.z);
  pVerts[1].Set(_min.x, _max.y, _max.z); pVerts[5].Set(_min.x, _max.y, _min.z);
  pVerts[2].Set(_min.x, _min.y, _max.z); pVerts[6].Set(_min);
  pVerts[3].Set(_max.x, _min.y, _max.z); pVerts[7].Set(_max.x, _min.y, _min.z);
}
