//------------------------------------------------------------------------------
// File : SkyMaterial.hpp
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
 * @file SkyMaterial.hpp
 * 
 * Interface definition for class SkyMaterial, a meterial property object.
 */
#ifndef __SKYMATERIAL_HPP__
#define __SKYMATERIAL_HPP__

// #pragma warning( disable : 4786)

#include "vec4f.hpp"
#include "SkyUtil.hpp"
#include "SkyTextureManager.hpp"
#include "SkyTextureState.hpp"
#include <GL/glut.h>

// forward
class SkyRenderable;


//------------------------------------------------------------------------------
/**
 * @class SkyMaterial
 * @brief A class for organizing and caching material state.
 * 
 * This class handles setting and querying material state.  By calling the Activate()
 * method, the material's state can be made current in OpenGL.  The material will not
 * set states that are currently active in the current OpenGL context.
 */
class SkyMaterial
{
public:
  SkyMaterial();
  ~SkyMaterial();
  
  SKYRESULT     Activate();
  SKYRESULT     Force();

  // Getters for basic material properties

  //! Returns the material identifier.
  int           GetMaterialID() const                  { return _iMaterialID;     }
  //! Returns the material diffuse color.
  const Vec4f&  GetDiffuse() const                     { return _vecDiffuse;      }
  //! Returns the material specular color.
  const Vec4f&  GetSpecular() const                    { return _vecSpecular;     }
  //! Returns the material ambient color.
  const Vec4f&  GetAmbient() const                     { return _vecAmbient;      }
  //! Returns the material emissive color.
  const Vec4f&  GetEmissive() const                    { return _vecEmissive;     }
  //! Returns the material specular power (shininess).
  const float   GetSpecularPower() const               { return _rSpecularPower;  }

  // lighting
  //! Returns true if lighting is enabled for this material.
  bool          IsLightingEnabled() const              { return _bLighting;       }

  // color material (which material property tracks color calls)
  //! Returns the face for which color material tracking is enabled.
  GLenum        GetColorMaterialFace() const           { return _eColorMaterialFace; }
  //! Returns the color material tracking mode.
  GLenum        GetColorMaterialMode() const           { return _eColorMaterialMode; }
  //! Returns true if color material tracking is enabled.
  bool          IsColorMaterialEnabled() const         { return _bColorMaterial;     }  

  //! Returns the fog density or start / end distance.
  float         GetFogParameter(GLenum eParameter) const;
  //! Returns the fog mode (exponential, linear, etc.)
  GLenum        GetFogMode() const                     { return _eFogMode;        }
  //! Returns the fog color.
  const Vec4f&  GetFogColor() const                    { return _vecFogColor;     }
  //! Returns true if fog is enabled for this material.
  bool          IsFogEnabled() const                   { return _bFog;            }

  // texturing
  //! Returns the active texture target for texture unit @a iTextureUnit.
  GLenum        GetActiveTarget(unsigned int iTextureUnit) const 
                { return _textureState.GetActiveTarget(iTextureUnit);   }
  //! Returns the bound texture ID for texture unit @a iTextureUnit.
  unsigned int  GetTextureID(unsigned int iTextureUnit) const    
                { return _textureState.GetTextureID(iTextureUnit);      }
  //! Returns true if texturing is enabled for texture unit @a iTextureUnit.
  bool          IsTextureEnabled(unsigned int iTextureUnit) const
                { return _textureState.IsTextureEnabled(iTextureUnit);  }
  //! Returns the value of the texture parameter @a eParameter for texture unit @a iTextureUnit.
  GLenum        GetTextureParameter(unsigned int iTextureUnit, GLenum eParameter) const
                { return _textureState.GetTextureParameter(iTextureUnit, eParameter); }
  //! Returns the texture application mode of the texture environment.
  GLenum        GetTextureApplicationMode() const      { return _eTextureEnvMode; }

  //! Returns a reference to the texture state object owned by this materal.
  SkyTextureState& GetTextureState()                   { return _textureState;    }

  // depth test
  //! Returns true if depth testing is enabled for this material.
  bool          IsDepthTestEnabled() const             { return _bDepthTest;      }
  //! Returns the depth test function for this material.
  GLenum        GetDepthFunc() const                   { return _eDepthFunc;      }
  //! Returns true if depth writes are enabled for this material, false if not.
  bool          GetDepthMask() const                   { return _bDepthMask;      }
  
  // alpha test
  //! Returns true if alpha testing is enabled for this material.
  bool          IsAlphaTestEnabled() const             { return _bAlphaTest;      }
  //! Returns the alpha test function for this material.
  GLenum        GetAlphaFunc() const                   { return _eAlphaFunc;      }
  //! Returns the reference value for alpha comparison.
  float         GetAlphaRef() const                    { return _rAlphaRef;       }
  
  // blending
  //! Returns true if blending is enabled for this material.
  bool          IsBlendingEnabled() const              { return _bBlending;       }
  //! Returns the source blending factor for this material.
  GLenum        GetBlendingSourceFactor() const        { return _eBlendSrcFactor; }
  //! Returns the destination blending factor for this material.
  GLenum        GetBlendingDestFactor() const          { return _eBlendDstFactor; }
    
  //! Returns true if face culling enabled for this material.
  bool          IsFaceCullingEnabled() const           { return _bFaceCulling;    }
  //! Returns which faces are culled -- front-facing or back-facing.
  GLenum        GetFaceCullingMode() const             { return _eFaceCullingMode; }

  // Setters for basic material properties

  //! Sets the material identifier.
  void          SetMaterialID(int ID)                  { _iMaterialID = ID;       }  
  //! Sets the diffuse material color.
  void          SetDiffuse( const Vec4f& d)            { _vecDiffuse = d;         }
  //! Sets the specular material color.
  void          SetSpecular(const Vec4f& d)            { _vecSpecular = d;        }
  //! Sets the ambient material color.
  void          SetAmbient( const Vec4f& d)            { _vecAmbient = d;         }
  //! Sets the emissive material color.
  void          SetEmissive(const Vec4f& d)            { _vecEmissive = d;        }
  //! Sets the material specular power (shininess).
  void          SetSpecularPower(float power)          { _rSpecularPower = power; }

  // lighting
  //! Enables / Disables lighting for this material.
  void          EnableLighting(bool bEnable)           { _bLighting = bEnable;    }

  // color material (which material property tracks color calls)
  //! Sets which faces (front or back) track color calls.
  void          SetColorMaterialFace(GLenum eFace)     { _eColorMaterialFace = eFace; }
  //! Sets which material property tracks color calls.
  void          SetColorMaterialMode(GLenum eMode)     { _eColorMaterialMode = eMode; }
  //! Enables / Disables material color tracking for this material.
  void          EnableColorMaterial(bool bEnable)      { _bColorMaterial = bEnable;   }

  //! Sets the fog density or start / end distance.
  SKYRESULT     SetFogParameter(GLenum eParameter, float rValue);
  //! Sets the fog mode (exponential, linear, etc.)
  void          SetFogMode(GLenum eMode)               { _eFogMode = eMode;           }
  //! Sets the fog color.
  void          SetFogColor(const Vec4f& color)        { _vecFogColor = color;        }
  //! Enables / Disables fog for this material.
  void          EnableFog(bool bEnable)                { _bFog = bEnable;             }
  
  // texturing
  //! Sets the bound texture and texture target for texture unit @a iTextureUnit.
  SKYRESULT     SetTexture(unsigned int iTextureUnit, GLenum eTarget, SkyTexture& texture)
                { return _textureState.SetTexture(iTextureUnit, eTarget, texture);    }
  //! Sets the bound texture and texture target for texture unit @a iTextureUnit.
  SKYRESULT     SetTexture(unsigned int iTextureUnit, GLenum eTarget, unsigned int iTextureID)
                { return _textureState.SetTexture(iTextureUnit, eTarget, iTextureID); }
  //! Enables / Disables texture unit @a iTextureUnit for this material.
  SKYRESULT     EnableTexture(unsigned int iTextureUnit, bool bEnable)
                { return _textureState.EnableTexture(iTextureUnit, bEnable);          }
  //! Sets the value of the texture parameter @a eParameter for texture unit @a iTextureUnit.
  SKYRESULT     SetTextureParameter(unsigned int  iTextureUnit, GLenum eParameter, GLenum eMode)
                { return _textureState.SetTextureParameter(iTextureUnit, eParameter, eMode); }

  //! Sets the texture application mode of the texture environment.
  void          SetTextureApplicationMode(GLenum eMode){ _eTextureEnvMode = eMode;}

  // depth test
  //! Enables / Disables depth test for this material.
  void          EnableDepthTest(bool bEnable)          { _bDepthTest = bEnable;   }
  //! Sets the depth test function (greater, less than, equal, etc.).
  void          SetDepthFunc(GLenum  eDepthFunc)       { _eDepthFunc = eDepthFunc;}
  //! If @a bDepthMask is true, then depth writes are enabled, otherwise they are not.
  void          SetDepthMask(bool    bDepthMask)       { _bDepthMask = bDepthMask;}
  
  // alpha test
  //! Enables / Disables alpha test for this material.
  void          EnableAlphaTest(bool bEnable)          { _bAlphaTest = bEnable;   }
  //! Sets the alpha test function (greater, less than, equal, etc.).
  void          SetAlphaFunc(GLenum  eAlphaFunc)       { _eAlphaFunc = eAlphaFunc;}
  //! Sets the reference value against which fragment alpha values are compared.
  void          SetAlphaRef(float    rAlphaRef)        { _rAlphaRef  = rAlphaRef; }
  
  // blending
  //! Enables / Disables blending for this material.
  void          EnableBlending(bool bEnable)           { _bBlending  = bEnable;   }
  //! Sets the source and destination blending factors for this material.
  void          SetBlendFunc(GLenum eSrcFactor, GLenum eDstFactor) 
                { _eBlendSrcFactor = eSrcFactor; _eBlendDstFactor = eDstFactor;   }

  //! Enables / Disables face culling for this material.
  void          EnableFaceCulling(bool bEnable)        { _bFaceCulling = bEnable; }
  //! Sets which faces will be culled -- front facing or back facing.
  void          SetFaceCullingMode(GLenum eMode)       { _eFaceCullingMode = eMode; }

protected:
  int           _iMaterialID;  

  Vec4f         _vecDiffuse;
  Vec4f         _vecSpecular;
  Vec4f         _vecAmbient;
  Vec4f         _vecEmissive;
  
  float         _rSpecularPower;

  bool          _bLighting;

  GLenum        _eColorMaterialFace;
  GLenum        _eColorMaterialMode;
  bool          _bColorMaterial;

  enum SkyFogParams
  {
    SKY_FOG_DENSITY,
    SKY_FOG_START,
    SKY_FOG_END,
    SKY_FOG_NUM_PARAMS
  };

  Vec4f         _vecFogColor;
  GLenum        _eFogMode;
  float         _rFogParams[SKY_FOG_NUM_PARAMS];
  bool          _bFog;

  GLenum        _eDepthFunc;
  bool          _bDepthMask;
  bool          _bDepthTest;

  GLenum        _eAlphaFunc;
  float         _rAlphaRef;
  bool          _bAlphaTest;

  GLenum        _eBlendSrcFactor;
  GLenum        _eBlendDstFactor;
  bool          _bBlending;

  bool          _bFaceCulling;
  GLenum        _eFaceCullingMode;

  SkyTextureState _textureState;
  GLenum        _eTextureEnvMode;
};

#endif //__SKYMATERIAL_HPP__
