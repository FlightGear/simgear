//------------------------------------------------------------------------------
// File : SkyCloud.cpp
//------------------------------------------------------------------------------
// SkyWorks : Adapted from skyworks program writen by Mark J. Harris and
//						The University of North Carolina at Chapel Hill
//					: by J. Wojnaroski Sep 2002
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
 * @file SkyCloud.cpp
 * 
 * Implementation of class SkyCloud.
 */

// warning for truncation of template name for browse info
#pragma warning( disable : 4786)

#include <plib/ul.h>

#include "SkyCloud.hpp"
#include "SkyRenderableInstance.hpp" 
#include "SkyContext.hpp"
#include "SkyMaterial.hpp"
#include "SkyLight.hpp"
#include "SkyTextureManager.hpp"
#include "SkySceneManager.hpp"
#include <algorithm>

//! The version used for cloud archive files.
#define CLOUD_ARCHIVE_VERSION 0.1f

//------------------------------------------------------------------------------
// Static initialization
//------------------------------------------------------------------------------
SkyMaterial*  SkyCloud::s_pMaterial         = NULL;
SkyMaterial*  SkyCloud::s_pShadeMaterial    = NULL;
unsigned int  SkyCloud::s_iShadeResolution  = 32;
float         SkyCloud::s_rAlbedo           = 0.9f;
float         SkyCloud::s_rExtinction       = 80.0f;
float         SkyCloud::s_rTransparency     = exp(-s_rExtinction);
float         SkyCloud::s_rScatterFactor    = s_rAlbedo * s_rExtinction * SKY_INV_4PI;
float         SkyCloud::s_rSortAngleErrorTolerance = 0.8f;
float         SkyCloud::s_rSortSquareDistanceTolerance = 100;

//------------------------------------------------------------------------------
// Function     	  : SkyCloud::SkyCloud
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::SkyCloud()
 * @brief Constructor.
 */ 
SkyCloud::SkyCloud()
: SkyRenderable(),
  _bUsePhaseFunction(true),
  _vecLastSortViewDir(Vec3f::ZERO),
  _vecLastSortCamPos(Vec3f::ZERO)
{
  if (!s_pShadeMaterial)
  {
    s_pShadeMaterial = new SkyMaterial;
    s_pShadeMaterial->SetAmbient(Vec4f(0.1f, 0.1f, 0.1f, 1));
    s_pShadeMaterial->EnableDepthTest(false);
    s_pShadeMaterial->SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    s_pShadeMaterial->EnableBlending(true);
    s_pShadeMaterial->SetAlphaFunc(GL_GREATER);
    s_pShadeMaterial->SetAlphaRef(0);
    s_pShadeMaterial->EnableAlphaTest(true);
    s_pShadeMaterial->SetColorMaterialMode(GL_DIFFUSE);
    s_pShadeMaterial->EnableColorMaterial(true);
    s_pShadeMaterial->EnableLighting(false);
    s_pShadeMaterial->SetTextureApplicationMode(GL_MODULATE);
  }
  if (!s_pMaterial)
  {
    s_pMaterial = new SkyMaterial;
    s_pMaterial->SetAmbient(Vec4f(0.3f, 0.3f, 0.3f, 1));
    s_pMaterial->SetDepthMask(false);
    s_pMaterial->SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    s_pMaterial->EnableBlending(true);
    s_pMaterial->SetAlphaFunc(GL_GREATER);
    s_pMaterial->SetAlphaRef(0);
    s_pMaterial->EnableAlphaTest(true);
    s_pMaterial->SetColorMaterialMode(GL_DIFFUSE);
    s_pMaterial->EnableColorMaterial(true);
    s_pMaterial->EnableLighting(false);
    s_pMaterial->SetTextureApplicationMode(GL_MODULATE);
    _CreateSplatTexture(32); // will assign the texture to both static materials
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::~SkyCloud
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::~SkyCloud()
 * @brief Destructor.
 */ 
SkyCloud::~SkyCloud()
{
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::Update
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyCloud::Update(const Camera &cam, SkyRenderableInstance* pInstance)
* @brief Currently does nothing.
*/ 
SKYRESULT SkyCloud::Update(const Camera &cam, SkyRenderableInstance* pInstance)
{
  return SKYRESULT_OK;
}



//------------------------------------------------------------------------------
// Function     	  : DrawQuad
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn DrawQuad(Vec3f pos, Vec3f x, Vec3f y, Vec4f color)
 * @brief Draw a quad.
 */ 
inline void DrawQuad(Vec3f pos, Vec3f x, Vec3f y, Vec4f color)
{
  glColor4fv(&(color.x));
  Vec3f left  = pos;  left   -= y; 
  Vec3f right = left; right  += x; 
  left  -= x;
  glTexCoord2f(0, 0); glVertex3fv(&(left.x));
  glTexCoord2f(1, 0); glVertex3fv(&(right.x));
  left  += y;  left  += y;
  right += y;  right += y;
  glTexCoord2f(1, 1); glVertex3fv(&(right.x));
  glTexCoord2f(0, 1); glVertex3fv(&(left.x));
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::Display
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::Display(const Camera &camera, SkyRenderableInstance *pInstance)
 * @brief Renders the cloud.
 * 
 * The cloud is rendered by splatting the particles from back to front with respect
 * to @a camera.  Since instances of clouds each have their own particles, which 
 * are pre-transformed into world space, @a pInstance is not used.
 *
 * An alternative method is to store the particles untransformed, and transform the 
 * camera and light into cloud space for rendering.  This is more complicated, 
 * and not as straightforward.  Since I have to store the particles with each instance
 * anyway, I decided to pre-transform them instead.
 */ 
SKYRESULT SkyCloud::Display(const Camera &camera, SkyRenderableInstance *pInstance)
{
  // copy the current camera
  Camera cam(camera);
  
  // This cosine computation, along with the if() below, are an optimization.  The goal
  // is to avoid sorting when it will make no visual difference.  This will be true when the 
  // cloud particles are almost sorted for the current viewpoint.  This is the case most of the 
  // time, since the viewpoint does not move very far in a single frame.  Each time we sort,
  // we cache the current view direction.  Then, each time the cloud is displayed, if the 
  // current view direction is very close to the current view direction (dot product is nearly 1)
  // then we do not resort the particles.
  float rCosAngleSinceLastSort = 
      _vecLastSortViewDir * cam.ViewDir(); // dot product

  float rSquareDistanceSinceLastSort = 
    (cam.Orig - _vecLastSortCamPos).LengthSqr();

  if (rCosAngleSinceLastSort < s_rSortAngleErrorTolerance || 
      rSquareDistanceSinceLastSort > s_rSortSquareDistanceTolerance)
  {
    // compute the sort position for particles.
    // don't just use the camera position -- if it is too far away from the cloud, then
    // precision limitations may cause the STL sort to hang.  Instead, put the sort position
    // just outside the bounding sphere of the cloud in the direction of the camera.
    _vecSortPos = -cam.ViewDir();
    _vecSortPos *= (1.1 * _boundingBox.GetRadius());
    _vecSortPos += _boundingBox.GetCenter();
    
    // sort the particles from back to front wrt the camera position.
    _SortParticles(cam.ViewDir(), _vecSortPos, SKY_CLOUD_SORT_TOWARD);

    //_vecLastSortViewDir = GLVU::GetCurrent()->GetCurrentCam()->ViewDir();
    //_vecLastSortCamPos = GLVU::GetCurrent()->GetCurrentCam()->Orig;
    _vecLastSortViewDir = cam.ViewDir();
    _vecLastSortCamPos = cam.Orig;
  } 

  // set the material state / properties that clouds use for rendering:
  // Enables blending, with blend func (ONE, ONE_MINUS_SRC_ALPHA).
  // Enables alpha test to discard completely transparent fragments.
  // Disables depth test.
  // Enables texturing, with modulation, and the texture set to the shared splat texture.
  s_pMaterial->Activate();
  
  Vec4f color;
  Vec3f eyeDir;

  // Draw the particles using immediate mode.
  glBegin(GL_QUADS);

  int i = 0;
  for (ParticleIterator iter = _particles.begin(); iter != _particles.end(); iter++)
  {
    i++;
    SkyCloudParticle *p = *iter;

    // Start with ambient light
    color = p->GetBaseColor();
    
    if (_bUsePhaseFunction) // use the phase function for anisotropic scattering.
    { 
      eyeDir = cam.Orig;
      eyeDir -= p->GetPosition();     
      eyeDir.Normalize();
      float pf;
          
      // add the color contribution to this particle from each light source, modulated by
      // the phase function.  See _PhaseFunction() documentation for details.
      for (int i = 0; i < p->GetNumLitColors(); i++)
      {
        pf = _PhaseFunction(_lightDirections[i], eyeDir);
        // expand this to avoid temporary vector creation in the inner loop
        color.x += p->GetLitColor(i).x * pf;
        color.y += p->GetLitColor(i).y * pf;
        color.z += p->GetLitColor(i).z * pf;
      }
    }
    else  // just use isotropic scattering instead.
    {   
      for (int i = 0; i < (*iter)->GetNumLitColors(); ++i)
      {
        color += p->GetLitColor(i);
      }
    }
    
    // Set the transparency independently of the colors
    color.w = 1 - s_rTransparency;
    
    // draw the particle as a textured billboard.
    DrawQuad((*iter)->GetPosition(), cam.X * p->GetRadius(), cam.Y * p->GetRadius(), color);
  }
  glEnd();
 
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::DisplaySplit
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::DisplaySplit(const Camera &camera, const Vec3f &vecSplitPoint, bool bBackHalf, SkyRenderableInstance *pInstance)
 * @brief The same as Display(), except it displays only the particles in front of or behind the split point.
 * 
 * This is used to render clouds into two impostor images for displaying clouds that contain objects.
 *
 * @see SkyRenderableInstanceCloud
 */ 
SKYRESULT SkyCloud::DisplaySplit(const Camera           &camera, 
                                 const Vec3f            &vecSplitPoint,
                                 bool                   bBackHalf,
                                 SkyRenderableInstance  *pInstance /* = NULL */)
{
  // copy the current camera
  Camera cam(camera);

  Vec3f vecCloudSpaceSplit = vecSplitPoint;

  if (bBackHalf) // only sort when rendering the back half.  Reuse sort for front half.
  {
    // compute the sort position for particles.
    // don't just use the camera position -- if it is too far away from the cloud, then
    // precision limitations may cause the STL sort to hang.  Instead, put the sort position
    // just outside the bounding sphere of the cloud in the direction of the camera.
    _vecSortPos = -cam.ViewDir();
    _vecSortPos *= (1.1 * _boundingBox.GetRadius());
    _vecSortPos += _boundingBox.GetCenter();
    
    // sort the particles from back to front wrt the camera position.
    _SortParticles(cam.ViewDir(), _vecSortPos, SKY_CLOUD_SORT_TOWARD);

    // we can't use the view direction optimization when the cloud is split, or we get a lot
    // of popping of objects in and out of cloud cover.  For consistency, though, we need to update
    // the cached sort direction, since we just sorted the particles.
    // _vecLastSortViewDir = GLVU::GetCurrent()->GetCurrentCam()->ViewDir(); 
    // _vecLastSortCamPos = GLVU::GetCurrent()->GetCurrentCam()->Orig;

    // compute the split distance.
    vecCloudSpaceSplit  -= _vecSortPos;
    _rSplitDistance = vecCloudSpaceSplit * cam.ViewDir();
  }

  // set the material state / properties that clouds use for rendering:
  // Enables blending, with blend func (ONE, ONE_MINUS_SRC_ALPHA).
  // Enables alpha test to discard completely transparent fragments.
  // Disables depth test.
  // Enables texturing, with modulation, and the texture set to the shared splat texture.
  s_pMaterial->Activate();
  
  Vec4f color;
  Vec3f eyeDir;

  // Draw the particles using immediate mode.
  glBegin(GL_QUADS);

  // if bBackHalf is false, then we just continue where we left off.  If it is true, we
  // reset the iterator to the beginning of the sorted list.
  static ParticleIterator iter;
  if (bBackHalf) 
    iter = _particles.begin();
  
  // iterate over the particles and render them.
  for (; iter != _particles.end(); ++iter)
  {
    SkyCloudParticle *p = *iter;

    if (bBackHalf && (p->GetSquareSortDistance() < _rSplitDistance))
      break;

    // Start with ambient light
    color = p->GetBaseColor();
    
    if (_bUsePhaseFunction) // use the phase function for anisotropic scattering.
    {
      eyeDir = cam.Orig;
      eyeDir -= p->GetPosition();
      eyeDir.Normalize();
      float pf;
          
      // add the color contribution to this particle from each light source, modulated by
      // the phase function.  See _PhaseFunction() documentation for details.
      for (int i = 0; i < p->GetNumLitColors(); i++)
      {
        pf = _PhaseFunction(_lightDirections[i], eyeDir);
        // expand this to avoid temporary vector creation in the inner loop
        color.x += p->GetLitColor(i).x * pf;
        color.y += p->GetLitColor(i).y * pf;
        color.z += p->GetLitColor(i).z * pf;
      }
    }
    else  // just use isotropic scattering instead.
    {      
      for (int i = 0; i < p->GetNumLitColors(); ++i)
      {
        color += p->GetLitColor(i);
      }
    }

    // set the transparency independently of the colors.
    color.w = 1 - s_rTransparency;
    
    // draw the particle as a textured billboard.
    DrawQuad((*iter)->GetPosition(), cam.X * p->GetRadius(), cam.Y * p->GetRadius(), color);
  }
  glEnd();
  
  return SKYRESULT_OK;
}

//------------------------------------------------------------------------------
// Function     	  : SkyCloud::Illuminate
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::Illuminate(SkyLight *pLight, SkyRenderableInstance* pInstance, bool bReset)
 * @brief Compute the illumination of the cloud by the lightsource @a pLight
 * 
 * This method uses graphics hardware to compute multiple forward scattering at each cloud
 * in the cloud of light from the directional light source @a pLight.  The algorithm works
 * by successively subtracting "light" from an initially white (fully lit) frame buffer by
 * using hardware blending and read back.  The method stores the illumination from each light
 * source passed to it separately at each particle, unless @a bReset is true, in which case
 * the lists of illumination in the particles are reset before the lighting is computed.
 * 
 */ 
SKYRESULT SkyCloud::Illuminate(SkyLight *pLight, SkyRenderableInstance* pInstance, bool bReset)
{
  int iOldVP[4];	

  glGetIntegerv(GL_VIEWPORT, iOldVP);
  glViewport(0, 0, s_iShadeResolution, s_iShadeResolution);

  Vec3f vecDir(pLight->GetDirection());

  // if this is the first pass through the lights, reset will be true, and the cached light 
  // directions should be updated.  Light directions are cached in cloud space to accelerate 
  // computation of the phase function, which depends on light direction and view direction.
  if (bReset)
    _lightDirections.clear();
  _lightDirections.push_back(vecDir); // cache the (unit-length) light direction

  // compute the light/sort position for particles from the light direction.
  // don't just use the camera position -- if it is too far away from the cloud, then
  // precision limitations may cause the STL sort to hang.  Instead, put the sort position
  // just outside the bounding sphere of the cloud in the direction of the camera.
  Vec3f vecLightPos(vecDir);
  vecLightPos *= (1.1*_boundingBox.GetRadius());
  vecLightPos += _boundingBox.GetCenter();

  // Set up a camera to look at the cloud from the light position.  Since the sun is an infinite
  // light source, this camera will use an orthographic projection tightly fit to the bounding
  // sphere of the cloud.  
  Camera cam;
  
  // Avoid degenerate camera bases.
  Vec3f vecUp(0, 1, 0);
  if (fabs(vecDir * vecUp) - 1 < 1e-6) // check that the view and up directions are not parallel.
    vecUp.Set(1, 0, 0);
  
  cam.LookAt(vecLightPos, _boundingBox.GetCenter(), vecUp);

  // sort the particles away from the light source.
  _SortParticles(cam.ViewDir(), vecLightPos, SKY_CLOUD_SORT_AWAY);

  // projected dist to cntr along viewdir
  float DistToCntr = (_boundingBox.GetCenter() - vecLightPos) * cam.ViewDir();
  
  // calc tight-fitting near and far distances for the orthographic frustum
  float rNearDist = DistToCntr - _boundingBox.GetRadius();
  float rFarDist  = DistToCntr + _boundingBox.GetRadius();

  // set the modelview matrix from this camera.
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  float M[16];
 
 
  cam.GetModelviewMatrix(M);
  glLoadMatrixf(M);
  
  // switch to parallel projection
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(-_boundingBox.GetRadius(), _boundingBox.GetRadius(),
          -_boundingBox.GetRadius(), _boundingBox.GetRadius(),
          rNearDist, rFarDist);
  
  // set the material state / properties that clouds use for shading:
  // Enables blending, with blend func (ONE, ONE_MINUS_SRC_ALPHA).
  // Enables alpha test to discard completely transparent fragments.
  // Disables depth test.
  // Enables texturing, with modulation, and the texture set to the shared splat texture.
  s_pShadeMaterial->Activate();

  // these are used for projecting the particle position to determine where to read pixels.
  double MM[16], PM[16];
  int VP[4] = { 0, 0, s_iShadeResolution, s_iShadeResolution };
  glGetDoublev(GL_MODELVIEW_MATRIX, MM);
  glGetDoublev(GL_PROJECTION_MATRIX, PM);
    
  // initialize back buffer to all white -- modulation darkens areas where cloud particles
  // absorb light, and lightens it where they scatter light in the forward direction.
  glClearColor(1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  float rPixelsPerLength = s_iShadeResolution / (2 * _boundingBox.GetRadius());

  // the solid angle over which we will sample forward-scattered light.
  float rSolidAngle = 0.09;
  int i = 0;
  int iNumFailed = 0;
  for (ParticleIterator iter = _particles.begin(); iter != _particles.end(); ++iter, ++i)
  {
    Vec3f vecParticlePos = (*iter)->GetPosition();

    Vec3f vecOffset(vecLightPos);
    vecOffset -= vecParticlePos;
   
    // compute the pixel area to read back in order to integrate the illumination of the particle
    // over a constant solid angle.
    float rDistance  = fabs(cam.ViewDir() * vecOffset) - rNearDist;

    float rArea      = rSolidAngle * rDistance * rDistance;
    int   iPixelDim  = sqrt(rArea) * rPixelsPerLength;
    int   iNumPixels = iPixelDim * iPixelDim;
    if (iNumPixels < 1) 
    {
      iNumPixels = 1;
      iPixelDim = 1;
    }

    // the scale factor to convert the read back pixel colors to an average illumination of the area.
    float rColorScaleFactor = rSolidAngle / (iNumPixels * 255.0f);

    unsigned char *c = new unsigned char[4 * iNumPixels];
    
    Vec3d vecWinPos;
    
    // find the position in the buffer to which the particle position projects.
    if (!gluProject(vecParticlePos.x, vecParticlePos.y, vecParticlePos.z, 
                    MM, PM, VP, 
                    &(vecWinPos.x), &(vecWinPos.y), &(vecWinPos.z)))
    {
      FAIL_RETURN_MSG(SKYRESULT_FAIL, 
                      "Error: SkyCloud::Illuminate(): failed to project particle position.");
    }
   
    // offset the projected window position by half the size of the readback region.
    vecWinPos.x -= 0.5 * iPixelDim;
    if (vecWinPos.x < 0) vecWinPos.x = 0;
    vecWinPos.y -= 0.5 * iPixelDim;
    if (vecWinPos.y < 0) vecWinPos.y = 0;
    
    // read back illumination of this particle from the buffer.
    glReadBuffer(GL_BACK);
    glReadPixels(vecWinPos.x, vecWinPos.y, iPixelDim, iPixelDim, GL_RGBA, GL_UNSIGNED_BYTE, c);
    
    // scattering coefficient vector.
    Vec4f vecScatter(s_rScatterFactor, s_rScatterFactor, s_rScatterFactor, 1);

    // add up the read back pixels (only need one component -- its grayscale)
    int iSum = 0;
    for (int k = 0; k < 4 * iNumPixels; k+=4)
      iSum += c[k];
    delete [] c;
    
    // compute the amount of light scattered to this particle by particles closer to the light.
    // this is the illumination over the solid angle that we measured (using glReadPixels) times
    // the scattering coefficient (vecScatter);
    Vec4f vecScatteredAmount(iSum * rColorScaleFactor, 
                             iSum * rColorScaleFactor, 
                             iSum * rColorScaleFactor, 
                             1 - s_rTransparency);
    vecScatteredAmount &= vecScatter;

    // the color of th particle (iter) contributed by this light source (pLight) is the 
    // scattered light from the part of the cloud closer to the light, times the diffuse color
    // of the light source.  The alpha is 1 - the uniform transparency of all particles (modulated
    // by the splat texture).
    Vec4f vecColor = vecScatteredAmount;
    vecColor      &= pLight->GetDiffuse();  
    vecColor.w     = 1 - s_rTransparency;
    
    // add this color to the list of lit colors for the particle.  The contribution from each light
    // is kept separate because the phase function we apply at runtime depends on the light vector
    // for each light source separately.  This view-dependent effect is impossible without knowing 
    // the amount of light contributed for each light.  This, of course, assumes the clouds will
    // be lit by a reasonably small number of lights (The sun plus some simulation of light reflected
    // from the sky and / or ground.) This technique works very well for simulating anisotropic 
    // illumination by skylight.
    if (bReset)
    {
      (*iter)->SetBaseColor(s_pMaterial->GetAmbient());  
      (*iter)->ClearLitColors();
      (*iter)->AddLitColor(vecColor);
    }
    else
    {
      (*iter)->AddLitColor(vecColor);
    }
    
    // the following computation (scaling of the scattered amount by the phase function) is done
    // after the lit color is stored so we don't add the scattering to this particle twice.
    vecScatteredAmount *= 1.5; // rayleigh scattering phase function for angle of zero or 180 = 1.5!
    
    // clamp the color
    if (vecScatteredAmount.x > 1) vecScatteredAmount.x = 1;
    if (vecScatteredAmount.y > 1) vecScatteredAmount.y = 1;
    if (vecScatteredAmount.z > 1) vecScatteredAmount.z = 1;
    vecScatteredAmount.w = 1 - s_rTransparency;
    
    vecScatteredAmount.x = 0.50;  vecScatteredAmount.y = 0.60;  vecScatteredAmount.z = 0.70; 

    // Draw the particle as a texture billboard.  Use the scattered light amount as the color to 
    // simulate forward scattering of light by this particle.
    glBegin(GL_QUADS);
    DrawQuad(vecParticlePos, cam.X * (*iter)->GetRadius(), cam.Y * (*iter)->GetRadius(), vecScatteredAmount);
    glEnd();

    //glutSwapBuffers(); // Uncomment this swap buffers to visualize cloud illumination computation.
  }
  
  // Note: here we could optionally store the current back buffer as a shadow image
  // to be projected from the light position onto the scene.  This way we can have clouds shadow
  // the environment.
  
  // restore matrix stack and viewport.
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glViewport(iOldVP[0], iOldVP[1], iOldVP[2], iOldVP[3]);

  return SKYRESULT_OK; 
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::CopyBoundingVolume
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::CopyBoundingVolume() const
 * @brief Returns a new copy of the SkyMinMaxBox for this cloud.
 */ 
SkyMinMaxBox* SkyCloud::CopyBoundingVolume() const
{
  SkyMinMaxBox *pBox = new SkyMinMaxBox();
  pBox->SetMax(_boundingBox.GetMax());
  pBox->SetMin(_boundingBox.GetMin());
  return pBox; 
}

SKYRESULT SkyCloud::Load(const SkyArchive &archive, 
                         float rScale, /* = 1.0f */ 
                         double latitude, double longitude )
{
  unsigned int iNumParticles;
  Vec3f vecCenter = Vec3f::ZERO;
  //Vec3f vecCenter;
  //float rRadius;
  //archive.FindVec3f("CldCenter", &vecCenter);
  //archive.FindFloat32("CldRadius", &rRadius);

  //_boundingBox.SetMin(vecCenter - Vec3f(rRadius, rRadius, rRadius));
  //_boundingBox.SetMax(vecCenter + Vec3f(rRadius, rRadius, rRadius));

  archive.FindUInt32("CldNumParticles", &iNumParticles);
  //if (!bLocal)
    archive.FindVec3f("CldCenter", &vecCenter);

  Vec3f *pParticlePositions = new Vec3f[iNumParticles];
  float *pParticleRadii     = new float[iNumParticles];
  Vec4f *pParticleColors    = new Vec4f[iNumParticles];

  unsigned int iNumBytes;
  archive.FindData("CldParticlePositions", ANY_TYPE, (void**const)&pParticlePositions, &iNumBytes);
  archive.FindData("CldParticleRadii",     ANY_TYPE, (void**const)&pParticleRadii,     &iNumBytes);
  archive.FindData("CldParticleColors",    ANY_TYPE, (void**const)&pParticleColors,    &iNumBytes);
  
  for (unsigned int i = 0; i < iNumParticles; ++i)
  {
    SkyCloudParticle *pParticle = new SkyCloudParticle((pParticlePositions[i] + vecCenter) * rScale,
                                                       pParticleRadii[i] * rScale,
                                                       pParticleColors[i]);
    _boundingBox.AddPoint(pParticle->GetPosition());
    
    _particles.push_back(pParticle);
  }
  // this is just a bad hack to align cloud field from skyworks with local horizon at KSFO
  // this "almost" works not quite the right solution okay to get some up and running
  // we need to develop our own scheme for loading and positioning clouds
  Mat33f R;
  Vec3f  moveit;

  R.Set( 0, 1, 0,
  			 1, 0, 0,
  			 0, 0, 1);
  // clouds sit in the y-z plane and x-axis is the vertical cloud height
  Rotate( R ); 
  
// rotate the cloud field about the fgfs z-axis based on initial longitude
float ex = 0.0;
float ey = 0.0;
float ez = 1.0;
float phi = longitude / 57.29578;
float one_min_cos = 1 - cos(phi);
  
R.Set(
cos(phi) + one_min_cos*ex*ex, one_min_cos*ex*ey - ez*sin(phi), one_min_cos*ex*ez + ey*sin(phi),
one_min_cos*ex*ey + ez*sin(phi), cos(phi) + one_min_cos*ey*ey, one_min_cos*ey*ez - ex*sin(phi),
one_min_cos*ex*ez - ey*sin(phi), one_min_cos*ey*ez + ex*sin(phi), cos(phi) + one_min_cos*ez*ez );
  			
Rotate( R );

// okay now that let's rotate about a vector for latitude where longitude forms the 
// components of a unit vector in the x-y plane
ex = sin( longitude  / 57.29578 );
ey = -cos( longitude  / 57.29578 );
ez = 0.0;
phi = latitude / 57.29578;
one_min_cos = 1 - cos(phi);

R.Set(
cos(phi) + one_min_cos*ex*ex, one_min_cos*ex*ey - ez*sin(phi), one_min_cos*ex*ez + ey*sin(phi),
one_min_cos*ex*ey + ez*sin(phi), cos(phi) + one_min_cos*ey*ey, one_min_cos*ey*ez - ex*sin(phi),
one_min_cos*ex*ez - ey*sin(phi), one_min_cos*ey*ez + ex*sin(phi), cos(phi) + one_min_cos*ez*ez );
  			
Rotate( R );
// need to calculate an offset to place the clouds at ~3000 feet MSL  ATM this is an approximation 
// to move the clouds to some altitude above sea level. At some locations this could be underground
// will need a better scheme to position clouds per user preferences
float cloud_level_msl = 3000.0f;

float x_offset = ex * cloud_level_msl;
float y_offset = ey * cloud_level_msl; 
float z_offset = cloud_level_msl * 0.5;
moveit.Set( x_offset, y_offset, z_offset  );
  
  Translate( moveit );
  
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::Save
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyCloud::Save(SkyArchive &archive) const
* @brief Saves the cloud data to @a archive.
* 
* @todo <WRITE EXTENDED SkyCloud::Save FUNCTION DOCUMENTATION>
*/ 
SKYRESULT SkyCloud::Save(SkyArchive &archive) const
{
  SkyArchive myArchive("Cloud");
  //myArchive.AddVec3f("CldCenter", _center);
  //myArchive.AddFloat32("CldRadius", _boundingBox.GetRadius());
  myArchive.AddUInt32("CldNumParticles", _particles.size());
  
  // make temp arrays
  Vec3f *pParticlePositions = new Vec3f[_particles.size()];
  float *pParticleRadii     = new float[_particles.size()];
  Vec4f *pParticleColors    = new Vec4f[_particles.size()];
  
  unsigned int i = 0;

  for (ParticleConstIterator iter = _particles.begin(); iter != _particles.end(); ++iter, ++i)
  {
    pParticlePositions[i] = (*iter)->GetPosition(); // position around origin
    pParticleRadii[i]     = (*iter)->GetRadius();
    pParticleColors[i]    = (*iter)->GetBaseColor();
  }
  
  myArchive.AddData("CldParticlePositions", 
                    ANY_TYPE, 
                    pParticlePositions, 
                    sizeof(Vec3f), 
                    _particles.size());
  
  myArchive.AddData("CldParticleRadii", 
                    ANY_TYPE, 
                    pParticleRadii, 
                    sizeof(float), 
                    _particles.size());
  
  myArchive.AddData("CldParticleColors", 
                    ANY_TYPE, 
                    pParticleColors, 
                    sizeof(Vec3f), 
                    _particles.size());
  
  archive.AddArchive(myArchive);

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::Rotate
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::Rotate(const Mat33f& rot)
 * @brief @todo <WRITE BRIEF SkyCloud::Rotate DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyCloud::Rotate FUNCTION DOCUMENTATION>
 */ 
void SkyCloud::Rotate(const Mat33f& rot)
{
  _boundingBox.Clear();
  for (int i = 0; i < _particles.size(); ++i)
  {
    _particles[i]->SetPosition(rot * _particles[i]->GetPosition());
    _boundingBox.AddPoint(_particles[i]->GetPosition());
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::Translate
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::Translate(const Vec3f& trans)
 * @brief @todo <WRITE BRIEF SkyCloud::Translate DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyCloud::Translate FUNCTION DOCUMENTATION>
 */ 
void SkyCloud::Translate(const Vec3f& trans)
{
  for (int i = 0; i < _particles.size(); ++i)
  {
    _particles[i]->SetPosition(_particles[i]->GetPosition() + trans);
  }
  _boundingBox.SetMax(_boundingBox.GetMax() + trans);
  _boundingBox.SetMin(_boundingBox.GetMin() + trans);
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::Scale
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::Scale(const float scale)
 * @brief @todo <WRITE BRIEF SkyCloud::Scale DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyCloud::Scale FUNCTION DOCUMENTATION>
 */ 
void SkyCloud::Scale(const float scale)
{
  _boundingBox.Clear();
  for (int i = 0; i < _particles.size(); ++i)
  {
    _particles[i]->SetPosition(_particles[i]->GetPosition() * scale);
    _boundingBox.AddPoint(_particles[i]->GetPosition());
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::_SortParticles
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::_SortParticles(const Vec3f& vecViewDir, const Vec3f& sortPoint, SortDirection dir)
 * @brief Sorts the cloud particles in the direction specified by @a dir.
 * 
 * @vecSortPoint is assumed to already be transformed into the basis space of the cloud.
 */ 
void SkyCloud::_SortParticles(const Vec3f& vecViewDir,
                              const Vec3f& vecSortPoint, 
                              SortDirection dir)
{
  Vec3f partPos;
	for (int i = 0; i < _particles.size(); ++i)
	{
		partPos = _particles[i]->GetPosition();
		partPos -= vecSortPoint;
		_particles[i]->SetSquareSortDistance(partPos * vecViewDir);//partPos.LengthSqr());
	}

  switch (dir)
  {
  case SKY_CLOUD_SORT_TOWARD:
    std::sort(_particles.begin(), _particles.end(), _towardComparator);
    break;
  case SKY_CLOUD_SORT_AWAY:
    std::sort(_particles.begin(), _particles.end(), _awayComparator);
    break;
  default:
    break;
  }
}



//------------------------------------------------------------------------------
// Function     	  : EvalHermite
// Description	    : 
//------------------------------------------------------------------------------
/**
 * EvalHermite(float pA, float pB, float vA, float vB, float u)
 * @brief Evaluates Hermite basis functions for the specified coefficients.
 */ 
inline float EvalHermite(float pA, float pB, float vA, float vB, float u)
{
  float u2=(u*u), u3=u2*u;
  float B0 = 2*u3 - 3*u2 + 1;
  float B1 = -2*u3 + 3*u2;
  float B2 = u3 - 2*u2 + u;
  float B3 = u3 - u;
  return( B0*pA + B1*pB + B2*vA + B3*vB );
}

// NORMALIZED GAUSSIAN INTENSITY MAP (N must be a power of 2)

//------------------------------------------------------------------------------
// Function     	  : CreateGaussianMap
// Description	    : 
//------------------------------------------------------------------------------
/**
 * CreateGaussianMap(int N)
 * 
 * Creates a 2D gaussian image using a hermite surface.
 */ 
unsigned char* CreateGaussianMap(int N)
{
  float *M = new float[2*N*N];
  unsigned char *B = new unsigned char[4*N*N];
  float X,Y,Y2,Dist;
  float Incr = 2.0f/N;
  int i=0;  
  int j = 0;
  Y = -1.0f;
  for (int y=0; y<N; y++, Y+=Incr)
  {
    Y2=Y*Y;
    X = -1.0f;
    for (int x=0; x<N; x++, X+=Incr, i+=2, j+=4)
    {
      Dist = (float)sqrt(X*X+Y2);
      if (Dist>1) Dist=1;
      M[i+1] = M[i] = EvalHermite(0.4f,0,0,0,Dist);// * (1 - noise);
      B[j+3] = B[j+2] = B[j+1] = B[j] = (unsigned char)(M[i] * 255);
    }
  }
  SAFE_DELETE_ARRAY(M);
  return(B);
}              


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::_CreateSplatTexture
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::_CreateSplatTexture(unsigned int iResolution)
 * @brief Creates the texture map used for cloud particles.
 */ 
void SkyCloud::_CreateSplatTexture(unsigned int iResolution)
{
  unsigned char *splatTexture = CreateGaussianMap(iResolution);
  SkyTexture texture;
  TextureManager::InstancePtr()->Create2DTextureObject(texture, iResolution, iResolution, 
                                                       GL_RGBA, splatTexture);

  s_pMaterial->SetTexture(0, GL_TEXTURE_2D, texture);
  s_pShadeMaterial->SetTexture(0, GL_TEXTURE_2D, texture);
  s_pMaterial->SetTextureParameter(0, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  s_pShadeMaterial->SetTextureParameter(0, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  s_pMaterial->SetTextureParameter(0, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  s_pShadeMaterial->SetTextureParameter(0, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  s_pMaterial->SetTextureParameter(0, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  s_pShadeMaterial->SetTextureParameter(0, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  s_pMaterial->EnableTexture(0, true);
  s_pShadeMaterial->EnableTexture(0, true);

  SAFE_DELETE_ARRAY(splatTexture);
}     


//------------------------------------------------------------------------------
// Function     	  : SkyCloud::_PhaseFunction
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyCloud::_PhaseFunction(const Vec3f& vecLightDir, const Vec3f& vecViewDir)
 * @brief Computes the phase (scattering) function of the given light and view directions.
 * 
 * A phase function is a transfer function that determines, for any angle between incident 
 * and outgoing directions, how much of the incident light intensity will be 
 * scattered in the outgoing direction.  For example, scattering by very small 
 * particles such as those found in clear air, can be approximated using <i>Rayleigh 
 * scattering</i>.  The phase function for Rayleigh scattering is 
 * p(q) = 0.75*(1 + cos<sup>2</sup>(q)), where q  is the angle between incident 
 * and scattered directions.  Scattering by larger particles is more complicated.  
 * It is described by Mie scattering theory.  Cloud particles are more in the regime 
 * of Mie scattering than Rayleigh scattering.  However, we obtain good visual 
 * results by using the simpler Rayleigh scattering phase function as an approximation.
 */ 
float SkyCloud::_PhaseFunction(const Vec3f& vecLightDir, const Vec3f& vecViewDir)
{
  float rCosAlpha = vecLightDir * vecViewDir;
  return .75f * (1 + rCosAlpha * rCosAlpha); // rayleigh scattering = (3/4) * (1+cos^2(alpha))
}
