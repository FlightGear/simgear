//------------------------------------------------------------------------------
// File : SkyLight.hpp
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
 * @file SkyLight.hpp
 * 
 * Definition of a class that maintains the state and operation of a light.
 */
#ifndef __SKYLIGHT_HPP__
#define __SKYLIGHT_HPP__

#include "vec3f.hpp"
#include "vec4f.hpp"
#include "SkyUtil.hpp"

class SkyMaterial;

class SkyLight
{
public: // types
  enum SkyLightType
  {
    SKY_LIGHT_POINT,
    SKY_LIGHT_DIRECTIONAL,
    SKY_LIGHT_SPOT,
    SKY_LIGHT_NUM_TYPES
  };

public: // methods
  SkyLight(SkyLightType eType);
  virtual ~SkyLight();

  // for visualization of light positions / directions.
  void         Display() const;

  bool         GetEnabled() const               { return _bEnabled;          }
  SkyLightType GetType() const                  { return _eType;             }
  const float* GetPosition() const              { return _vecPosition;       }
  const float* GetDirection() const             { return _vecDirection;      }
  const float* GetDiffuse() const               { return _vecDiffuse;        }
  const float* GetAmbient() const               { return _vecAmbient;        }
  const float* GetSpecular() const              { return _vecSpecular;       }
  const float* GetAttenuation() const           { return _vecAttenuation;    }
  float        GetSpotExponent() const          { return _rSpotExponent;     }
  float        GetSpotCutoff() const            { return _rSpotCutoff;       }

  void         Enable(bool bEnable)             { _bEnabled       = bEnable; _bDirty = true; }
  void         SetType(const SkyLightType& t)   { _eType          = t;       _bDirty = true; }
  void         SetPosition(const float pos[3])  
  { _vecPosition.Set(pos[0], pos[1], pos[2], (_eType != SKY_LIGHT_DIRECTIONAL)); _bDirty = true; }
  
  void         SetDirection(const float dir[3]) { _vecDirection.Set(dir[0], dir[1], dir[2], 0); _bDirty = true; }
  void         SetDiffuse(const float color[4]) { _vecDiffuse.Set(color);    _bDirty = true; }
  void         SetAmbient(const float color[4]) { _vecAmbient.Set(color);    _bDirty = true; }
  void         SetSpecular(const float color[4]){ _vecSpecular.Set(color);   _bDirty = true; }
  void         SetAttenuation(float rConstant, float rLinear, float rQuadratic) 
  { _vecAttenuation.Set(rConstant, rLinear, rQuadratic);        _bDirty = true; }
  
  void         SetSpotExponent(float rExp)      { _rSpotExponent  = rExp;    _bDirty = true; }
  void         SetSpotCutoff(float rCutoff)     { _rSpotCutoff    = rCutoff; _bDirty = true; }
    
  SKYRESULT    Activate(int iLightID);

protected: // data
  bool  _bEnabled;
  bool  _bDirty;

  int   _iLastGLID;

  SkyLightType _eType;

  Vec4f _vecPosition;
  Vec4f _vecDirection;
  Vec4f _vecDiffuse;
  Vec4f _vecAmbient;
  Vec4f _vecSpecular;
  Vec3f _vecAttenuation;  // constant, linear, and quadratic attenuation factors.
  float _rSpotExponent;
  float _rSpotCutoff;

  static SkyMaterial *s_pMaterial;  // used for rendering the lights during debugging
};

#endif //__SKYLIGHT_HPP__