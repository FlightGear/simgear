//------------------------------------------------------------------------------
// File : SkyCloudParticle.hpp
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
 * @file SkyCloudParticle.hpp
 * 
 * Definition of a simple cloud particle class.
 */
#ifndef __SKYCLOUDPARTICLE_HPP__
#define __SKYCLOUDPARTICLE_HPP__

#include "vec3f.hpp"
#include "vec4f.hpp"
#include <vector>

//------------------------------------------------------------------------------
/**
 * @class SkyCloudParticle
 * @brief A class for particles that make up a cloud.
 * 
 * @todo <WRITE EXTENDED CLASS DESCRIPTION>
 */
class SkyCloudParticle
{
public:
  inline SkyCloudParticle();
  inline SkyCloudParticle(const Vec3f& pos, 
                          float rRadius, 
                          const Vec4f& baseColor, 
                          float rTransparency = 0);
  inline ~SkyCloudParticle();

  //! Returns the radius of the particle.
  float               GetRadius() const               { return _rRadius;        }
  //! Returns the transparency of the particle.
  float               GetTransparency() const         { return _rTransparency;  }
  //! Returns the position of the particle.
  const Vec3f&        GetPosition() const             { return _vecPosition;  }
  //! Returns the base color of the particle.  This is often used for ambient color.
  const Vec4f&        GetBaseColor() const            { return _vecBaseColor; }
  //! Returns the number of light contributions to this particle's color.
  unsigned int        GetNumLitColors() const         { return _vecLitColors.size();  }
  //! Returns the light contribution to the color of this particle from light @a index.
  inline const Vec4f& GetLitColor(unsigned int index) const;
  //! Returns the square distance from the sort position used for the operator< in sorts / splits.
  float               GetSquareSortDistance() const   { return _rSquareSortDistance; }

  //! Sets the radius of the particle.
  void                SetRadius(float rad)            { _rRadius      = rad;    }
  //! Returns the transparency of the particle.
  void                SetTransparency(float trans)    { _rTransparency = trans; }
  //! Sets the position of the particle.
  void                SetPosition(const Vec3f& pos)   { _vecPosition  = pos;  }
  //! Sets the base color of the particle.  This is often used for ambient color.
  void                SetBaseColor(const Vec4f& col)  { _vecBaseColor = col;  }
  //! Sets the light contribution to the color of this particle from light @a index.
  void                AddLitColor(const Vec4f& col)   { _vecLitColors.push_back(col); }
  //! Clears the list of light contributions.
  void                ClearLitColors()                { _vecLitColors.clear();        }
  //! Sets the square distance from the sort position used for the operator< in sorts.
  void                SetSquareSortDistance(float rSquareDistance) { _rSquareSortDistance = rSquareDistance; }

  //! This operator is used to sort particle arrays from nearest to farthes.
  bool operator<(const SkyCloudParticle& p) const
	{
    return (_rSquareSortDistance < p._rSquareSortDistance);
	}

  //! This operator is used to sort particle arrays from farthest to nearest.
  bool operator>(const SkyCloudParticle& p) const
  {
    return (_rSquareSortDistance > p._rSquareSortDistance);
  }
  
protected: 
  float               _rRadius;
  float               _rTransparency;
  Vec3f               _vecPosition;
  Vec4f               _vecBaseColor;
  std::vector<Vec4f>  _vecLitColors;
  Vec3f               _vecEye; 

  // for sorting particles during shading
  float               _rSquareSortDistance;
};

//------------------------------------------------------------------------------
// Function     	  : SkyCloudParticle::SkyCloudParticle
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloudParticle::SkyCloudParticle()
 * @brief Default constructor.
 */ 
inline SkyCloudParticle::SkyCloudParticle()
: _rRadius(0),
  _rTransparency(0),
  _vecPosition(0, 0, 0),
  _vecBaseColor(0, 0, 0, 1),
  _vecEye(0, 0, 0),
  _rSquareSortDistance(0)
{
  _vecLitColors.clear();
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloudParticle::SkyCloudParticle
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloudParticle::SkyCloudParticle(const Vec3f& pos, float rRadius, const Vec4f& baseColor, float rTransparency)
 * @brief Constructor.
 */ 
inline SkyCloudParticle::SkyCloudParticle(const Vec3f& pos, 
                                          float rRadius, 
                                          const Vec4f& baseColor,
                                          float rTransparency /* = 0 */)
: _rRadius(rRadius),
  _rTransparency(rTransparency),
  _vecPosition(pos),
  _vecBaseColor(baseColor),
  _vecEye(0, 0, 0),
  _rSquareSortDistance(0)
{
  _vecLitColors.clear();
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloudParticle::~SkyCloudParticle
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloudParticle::~SkyCloudParticle()
 * @brief Destructor.
 */ 
inline SkyCloudParticle::~SkyCloudParticle()
{
  _vecLitColors.clear();
}


//------------------------------------------------------------------------------
// Function     	  : Vec4f& SkyCloudParticle::GetLitColor
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn Vec4f& SkyCloudParticle::GetLitColor(unsigned int index) const
 * @brief Returns the lit color specified by index.
 * 
 * If the index is out of range, returns a zero vector.
 */ 
inline const Vec4f& SkyCloudParticle::GetLitColor(unsigned int index) const
{
  if (index <= _vecLitColors.size())
    return _vecLitColors[index];
  else
    return Vec4f::ZERO;
}

#endif //__SKYCLOUDPARTICLE_HPP__
