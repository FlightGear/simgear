//------------------------------------------------------------------------------
// File : SkyMaterial.cpp
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
 * @file SkyMaterial.cpp
 * 
 * Implementation of class SkyMaterial, a meterial property object.
 */
#include "SkyMaterial.hpp"
#include "SkyContext.hpp"

//------------------------------------------------------------------------------
// Function     	  : SkyMaterial::SkyMaterial
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMaterial::SkyMaterial()
 * @brief Constructor.
 */ 
SkyMaterial::SkyMaterial()
: _iMaterialID(-1),  
  _vecDiffuse(Vec4f::ZERO),
  _vecSpecular(Vec4f::ZERO),
  _vecAmbient(Vec4f::ZERO),
  _vecEmissive(Vec4f::ZERO),
  _rSpecularPower(0),
  _bLighting(true),
  _eColorMaterialFace(GL_FRONT_AND_BACK),
  _eColorMaterialMode(GL_AMBIENT_AND_DIFFUSE),
  _bColorMaterial(false),
  _vecFogColor(Vec4f::ZERO),
  _eFogMode(GL_EXP),
  _bFog(false),
  _eDepthFunc(GL_LESS),
  _bDepthMask(true),
  _bDepthTest(true),
  _eAlphaFunc(GL_ALWAYS),
  _rAlphaRef(0),
  _bAlphaTest(false), 
  _eBlendSrcFactor(GL_ONE),
  _eBlendDstFactor(GL_ZERO),
  _bBlending(false),
  _bFaceCulling(false),
  _eFaceCullingMode(GL_BACK),
  _eTextureEnvMode(GL_MODULATE)
{
  _rFogParams[SKY_FOG_DENSITY]  = 1;
  _rFogParams[SKY_FOG_START]    = 0;
  _rFogParams[SKY_FOG_END]      = 1;
}


//------------------------------------------------------------------------------
// Function     	  : SkyMaterial::~SkyMaterial
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMaterial::~SkyMaterial()
 * @brief Destructor.
 */ 
SkyMaterial::~SkyMaterial()
{
}


//------------------------------------------------------------------------------
// Function     	  : SkyMaterial::SetFogParameter
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMaterial::SetFogParameter(GLenum eParameter, float rValue)
 */ 
SKYRESULT SkyMaterial::SetFogParameter(GLenum eParameter, float rValue)
{
  switch (eParameter)
  {
  case GL_FOG_DENSITY:
    _rFogParams[SKY_FOG_DENSITY] = rValue;
    break;
  case GL_FOG_START:
    _rFogParams[SKY_FOG_START] = rValue;
    break;
  case GL_FOG_END:
    _rFogParams[SKY_FOG_END] = rValue;
    break;
  default:
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyMaterial::SetFogParameter(): Invalid parameter.");
    break;
  }
  
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyMaterial::GetFogParameter
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMaterial::GetFogParameter(GLenum eParameter) const
 */ 
float SkyMaterial::GetFogParameter(GLenum eParameter) const
{
  switch (eParameter)
  {
  case GL_FOG_DENSITY:
    return _rFogParams[SKY_FOG_DENSITY];
    break;
  case GL_FOG_START:
    return _rFogParams[SKY_FOG_START];
    break;
  case GL_FOG_END:
    return _rFogParams[SKY_FOG_END];
    break;
  default:
    FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyMaterial::GetFogParameter(): Invalid parameter.");
    break;
  }
  
  return -1;
}


//------------------------------------------------------------------------------
// Function     	  : SkyMaterial::Activate
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMaterial::Activate()
 * @brief @todo <WRITE BRIEF SkyMaterial::SetMaterialPropertiesForDisplay DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyMaterial::SetMaterialPropertiesForDisplay FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkyMaterial::Activate()
{
  // Update the cached current material, and only pass values that have changed to the GL.
  
  SkyMaterial *pCurrentMaterial = GraphicsContext::InstancePtr()->GetCurrentMaterial();
  assert(NULL != pCurrentMaterial);

  // basic material properties
  if (pCurrentMaterial->GetDiffuse() != GetDiffuse())
  {
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &(GetDiffuse().x));
    pCurrentMaterial->SetDiffuse(GetDiffuse());
  }
  if (pCurrentMaterial->GetSpecular() != GetSpecular())
  {
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &(GetSpecular().x));
    pCurrentMaterial->SetSpecular(GetSpecular());
  }  
  if (pCurrentMaterial->GetAmbient() != GetAmbient())
  {
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &(GetAmbient().x));
    pCurrentMaterial->SetAmbient(GetAmbient());
  }
  if (pCurrentMaterial->GetEmissive() != GetEmissive())
  {
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &(GetEmissive().x));
    pCurrentMaterial->SetEmissive(GetEmissive());
  }
  if (pCurrentMaterial->GetSpecularPower() != GetSpecularPower())
  {
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, GetSpecularPower());
    pCurrentMaterial->SetSpecularPower(GetSpecularPower());
  } 

  // lighting
  if (pCurrentMaterial->IsLightingEnabled() != IsLightingEnabled())
  {
    if (IsLightingEnabled())
      glEnable(GL_LIGHTING);
    else
      glDisable(GL_LIGHTING);
    pCurrentMaterial->EnableLighting(IsLightingEnabled());
  }

  // color material (which material property tracks color calls)
  if (pCurrentMaterial->GetColorMaterialFace() != GetColorMaterialFace() ||
      pCurrentMaterial->GetColorMaterialMode() != GetColorMaterialMode())
  {
    glColorMaterial(GetColorMaterialFace(), GetColorMaterialMode());
    pCurrentMaterial->SetColorMaterialFace(GetColorMaterialFace());
    pCurrentMaterial->SetColorMaterialMode(GetColorMaterialMode());
  }
  if (pCurrentMaterial->IsColorMaterialEnabled() != IsColorMaterialEnabled())
  {
    if (IsColorMaterialEnabled()) 
      glEnable(GL_COLOR_MATERIAL);
    else
      glDisable(GL_COLOR_MATERIAL);
    pCurrentMaterial->EnableColorMaterial(IsColorMaterialEnabled());
  }

  // fog
  if (pCurrentMaterial->GetFogMode() != GetFogMode())
  {
    glFogf(GL_FOG_MODE, GetFogMode());
    pCurrentMaterial->SetFogMode(GetFogMode());
  }
  if (pCurrentMaterial->GetFogColor() != GetFogColor())
  {
    glFogfv(GL_FOG_COLOR, GetFogColor());
    pCurrentMaterial->SetFogColor(GetFogColor());
  }
  if (pCurrentMaterial->GetFogParameter(GL_FOG_DENSITY) != GetFogParameter(GL_FOG_DENSITY))
  {
    glFogf(GL_FOG_DENSITY, GetFogParameter(GL_FOG_DENSITY));
    pCurrentMaterial->SetFogParameter(GL_FOG_DENSITY, GetFogParameter(GL_FOG_DENSITY));
  }
  if (pCurrentMaterial->GetFogParameter(GL_FOG_START) != GetFogParameter(GL_FOG_START))
  {
    glFogf(GL_FOG_START, GetFogParameter(GL_FOG_START));
    pCurrentMaterial->SetFogParameter(GL_FOG_START, GetFogParameter(GL_FOG_START));
  }
  if (pCurrentMaterial->GetFogParameter(GL_FOG_END) != GetFogParameter(GL_FOG_END))
  {
    glFogf(GL_FOG_END, GetFogParameter(GL_FOG_END));
    pCurrentMaterial->SetFogParameter(GL_FOG_END, GetFogParameter(GL_FOG_END));
  }
  if (pCurrentMaterial->IsFogEnabled() != IsFogEnabled())
  {
    if (IsFogEnabled())
      glEnable(GL_FOG);
    else
      glDisable(GL_FOG);
    pCurrentMaterial->EnableFog(IsFogEnabled());
  }

  // depth test
  if (pCurrentMaterial->GetDepthFunc() != GetDepthFunc())
  {
    glDepthFunc(GetDepthFunc());
    pCurrentMaterial->SetDepthFunc(GetDepthFunc());
  }
  if (pCurrentMaterial->GetDepthMask() != GetDepthMask())
  {
    glDepthMask(GetDepthMask());
    pCurrentMaterial->SetDepthMask(GetDepthMask());
  }
  if (pCurrentMaterial->IsDepthTestEnabled() != IsDepthTestEnabled())
  {
    if (IsDepthTestEnabled()) 
      glEnable(GL_DEPTH_TEST);
    else 
      glDisable(GL_DEPTH_TEST);
    pCurrentMaterial->EnableDepthTest(IsDepthTestEnabled());
  }

  // alpha test
  if (pCurrentMaterial->GetAlphaFunc() != GetAlphaFunc() ||
      pCurrentMaterial->GetAlphaRef()  != GetAlphaRef())
  {
    glAlphaFunc(GetAlphaFunc(), GetAlphaRef());
    pCurrentMaterial->SetAlphaFunc(GetAlphaFunc());
    pCurrentMaterial->SetAlphaRef(GetAlphaRef());
  }
  if (pCurrentMaterial->IsAlphaTestEnabled() != IsAlphaTestEnabled())
  {
    if (IsAlphaTestEnabled()) 
      glEnable(GL_ALPHA_TEST);
    else
      glDisable(GL_ALPHA_TEST);
    pCurrentMaterial->EnableAlphaTest(IsAlphaTestEnabled());
  }

  // blending
  if (pCurrentMaterial->GetBlendingSourceFactor() != GetBlendingSourceFactor() ||
      pCurrentMaterial->GetBlendingDestFactor()   != GetBlendingDestFactor())
  {
    glBlendFunc(GetBlendingSourceFactor(), GetBlendingDestFactor());
    pCurrentMaterial->SetBlendFunc(GetBlendingSourceFactor(), GetBlendingDestFactor());
  }
  if (pCurrentMaterial->IsBlendingEnabled() != IsBlendingEnabled())
  {
    if (IsBlendingEnabled())
      glEnable(GL_BLEND);
    else
      glDisable(GL_BLEND);
    pCurrentMaterial->EnableBlending(IsBlendingEnabled());
  }

  if (pCurrentMaterial->GetFaceCullingMode() != GetFaceCullingMode())
  {
    glCullFace(GetFaceCullingMode());
    pCurrentMaterial->SetFaceCullingMode(GetFaceCullingMode());
  }
  if (pCurrentMaterial->IsFaceCullingEnabled() != IsFaceCullingEnabled())
  {
    if (IsFaceCullingEnabled())
      glEnable(GL_CULL_FACE);
    else
      glDisable(GL_CULL_FACE);
    pCurrentMaterial->EnableFaceCulling(IsFaceCullingEnabled());
  }

  // texturing
  FAIL_RETURN(_textureState.Activate());
  if (pCurrentMaterial->GetTextureApplicationMode() != GetTextureApplicationMode())
  {
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GetTextureApplicationMode());
    pCurrentMaterial->SetTextureApplicationMode(GetTextureApplicationMode());
  }

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyMaterial::Force
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyMaterial::Force()
 * @brief @todo <WRITE BRIEF SkyMaterial::SetMaterialPropertiesForDisplay DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyMaterial::SetMaterialPropertiesForDisplay FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkyMaterial::Force()
{
  // Update the cached current material, and only pass values that have changed to the GL.
  
  SkyMaterial *pCurrentMaterial = GraphicsContext::InstancePtr()->GetCurrentMaterial();
  assert(NULL != pCurrentMaterial);

  // basic material properties
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &(GetDiffuse().x));
  pCurrentMaterial->SetDiffuse(GetDiffuse());

  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &(GetSpecular().x));
  pCurrentMaterial->SetSpecular(GetSpecular());

  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &(GetAmbient().x));
  pCurrentMaterial->SetAmbient(GetAmbient());

  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, &(GetEmissive().x));
  pCurrentMaterial->SetEmissive(GetEmissive());

  glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, GetSpecularPower());
  pCurrentMaterial->SetSpecularPower(GetSpecularPower());

  // lighting
  if (IsLightingEnabled())
      glEnable(GL_LIGHTING);
  else
      glDisable(GL_LIGHTING);
  pCurrentMaterial->EnableLighting(IsLightingEnabled());

  // color material (which material property tracks color calls)
  glColorMaterial(GetColorMaterialFace(), GetColorMaterialMode());
  pCurrentMaterial->SetColorMaterialFace(GetColorMaterialFace());
  pCurrentMaterial->SetColorMaterialMode(GetColorMaterialMode());

  if (IsColorMaterialEnabled()) 
      glEnable(GL_COLOR_MATERIAL);
  else
      glDisable(GL_COLOR_MATERIAL);
  pCurrentMaterial->EnableColorMaterial(IsColorMaterialEnabled());

  // fog
  glFogf(GL_FOG_MODE, GetFogMode());
  pCurrentMaterial->SetFogMode(GetFogMode());

  glFogfv(GL_FOG_COLOR, GetFogColor());
  pCurrentMaterial->SetFogColor(GetFogColor());

  glFogf(GL_FOG_DENSITY, GetFogParameter(GL_FOG_DENSITY));
  pCurrentMaterial->SetFogParameter(GL_FOG_DENSITY, GetFogParameter(GL_FOG_DENSITY));

  glFogf(GL_FOG_START, GetFogParameter(GL_FOG_START));
  pCurrentMaterial->SetFogParameter(GL_FOG_START, GetFogParameter(GL_FOG_START));
  glFogf(GL_FOG_END, GetFogParameter(GL_FOG_END));
  pCurrentMaterial->SetFogParameter(GL_FOG_END, GetFogParameter(GL_FOG_END));

  if (IsFogEnabled())
      glEnable(GL_FOG);
  else
      glDisable(GL_FOG);
  pCurrentMaterial->EnableFog(IsFogEnabled());

  // depth test
  glDepthFunc(GetDepthFunc());
  pCurrentMaterial->SetDepthFunc(GetDepthFunc());

  glDepthMask(GetDepthMask());
  pCurrentMaterial->SetDepthMask(GetDepthMask());

  if (IsDepthTestEnabled()) 
      glEnable(GL_DEPTH_TEST);
  else 
      glDisable(GL_DEPTH_TEST);
  pCurrentMaterial->EnableDepthTest(IsDepthTestEnabled());

  // alpha test
  glAlphaFunc(GetAlphaFunc(), GetAlphaRef());
  pCurrentMaterial->SetAlphaFunc(GetAlphaFunc());
  pCurrentMaterial->SetAlphaRef(GetAlphaRef());

  if (IsAlphaTestEnabled()) 
      glEnable(GL_ALPHA_TEST);
  else
      glDisable(GL_ALPHA_TEST);

  // blending
  glBlendFunc(GetBlendingSourceFactor(), GetBlendingDestFactor());
  pCurrentMaterial->SetBlendFunc(GetBlendingSourceFactor(), GetBlendingDestFactor());

  if (IsBlendingEnabled())
      glEnable(GL_BLEND);
  else
      glDisable(GL_BLEND);
  pCurrentMaterial->EnableBlending(IsBlendingEnabled());

  glCullFace(GetFaceCullingMode());
  pCurrentMaterial->SetFaceCullingMode(GetFaceCullingMode());

  if (IsFaceCullingEnabled())
      glEnable(GL_CULL_FACE);
  else
      glDisable(GL_CULL_FACE);
  pCurrentMaterial->EnableFaceCulling(IsFaceCullingEnabled());

  // texturing
  FAIL_RETURN(_textureState.Force());
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GetTextureApplicationMode());
  pCurrentMaterial->SetTextureApplicationMode(GetTextureApplicationMode());

  return SKYRESULT_OK;
}


