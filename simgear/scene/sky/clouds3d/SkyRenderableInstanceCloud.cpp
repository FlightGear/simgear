//------------------------------------------------------------------------------
// File : SkyRenderableInstanceCloud.cpp
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
 * @file SkyRenderableInstanceCloud.cpp
 * 
 * Implementation of class SkyRenderableInstanceCloud.
 */
#include "SkyUtil.hpp"
#include "SkyCloud.hpp"
#include "SkyMaterial.hpp"
#include "SkyBoundingVolume.hpp"
#include "SkyRenderableInstanceCloud.hpp"
#include "SkyDynamicTextureManager.hpp"

//! Set this to 1 to see verbose messages about impostor updates.
#define SKYCLOUD_VERBOSE 0

//! Set this to control the number of frames a cloud has to be culled before its textures are released.
#define SKYCLOUD_CULL_RELEASE_COUNT 100

//------------------------------------------------------------------------------
// Static declarations.
//------------------------------------------------------------------------------
unsigned int        SkyRenderableInstanceCloud::s_iCount               = 0;
float               SkyRenderableInstanceCloud::s_rErrorToleranceAngle = SKYDEGREESTORADS * 0.125f;
SkyMaterial*        SkyRenderableInstanceCloud::s_pMaterial            = NULL;

//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::SkyRenderableInstanceCloud
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::SkyRenderableInstanceCloud(SkyCloud *pCloud, bool bUseOffScreenBuffer)
 * @brief Constructor.
 */ 
SkyRenderableInstanceCloud::SkyRenderableInstanceCloud(SkyCloud *pCloud, 
                                                       bool bUseOffScreenBuffer /* = true */)
: SkyRenderableInstance(),
  _iCloudID(-1),
  _pCloud(pCloud),
  _pWorldSpaceBV(NULL),
  _rRadius(0),
  _bScreenImpostor(false),
  _bImageExists(false),
  _bEnabled(true),
  _bUseOffScreenBuffer(bUseOffScreenBuffer),
  _bSplit(false),
  _vecSplit(0, 0, 0),
  _vecNearPoint(0, 0, 0),
  _vecFarPoint(0, 0, 0), 
  _iLogResolution(0),
  _pBackTexture(NULL),
  _pFrontTexture(NULL),
  _iCulledCount(0)
{
  _Initialize();
//   cout << "Cloud Instance created" << endl;
}

//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::SkyRenderableInstanceCloud
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::SkyRenderableInstanceCloud(SkyCloud *pCloud, const Vec3f &position, const Mat33f &rotation, const float scale, bool bUseOffScreenBuffer)
 * @brief Constructor.
 */ 
SkyRenderableInstanceCloud::SkyRenderableInstanceCloud(SkyCloud     *pCloud, 
                                                       const Vec3f  &position, 
                                                       const Mat33f &rotation, 
                                                       const float  scale,
                                                       bool         bUseOffScreenBuffer /* = true */)
: SkyRenderableInstance(position, rotation, scale),
  _iCloudID(-1),
  _pCloud(pCloud),
  _pWorldSpaceBV(NULL),
  _rRadius(0),
  _bScreenImpostor(false),
  _bImageExists(false),
  _bEnabled(true),
  _bUseOffScreenBuffer(false),
  _bSplit(false),
  _vecSplit(0, 0, 0),
  _vecNearPoint(0, 0, 0),
  _vecFarPoint(0, 0, 0), 
  _iLogResolution(0),
  _pBackTexture(NULL),
  _pFrontTexture(NULL)
{
  _Initialize();
}

//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::~SkyRenderableInstanceCloud
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::~SkyRenderableInstanceCloud()
 * @brief Destructor
 */ 
SkyRenderableInstanceCloud::~SkyRenderableInstanceCloud()
{
  _pCloud = NULL;
  SAFE_DELETE(_pWorldSpaceBV);

  s_iCount--;
  // delete the offscreen buffer when no one else is using it.
  if (0 == s_iCount)
  {
//JW??    SAFE_DELETE(s_pRenderBuffer);
    SAFE_DELETE(s_pMaterial);
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::SetPosition
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::SetPosition(const Vec3f  &position)
 * @brief Set the world space position of the instance.
 * 
 * @todo <WRITE EXTENDED SkyRenderableInstanceCloud::SetPosition FUNCTION DOCUMENTATION>
 */ 
void SkyRenderableInstanceCloud::SetPosition(const Vec3f  &position)
{ 
  if (_pCloud)
  {
    _pCloud->Translate(position - _vecPosition);
  }
  _vecPosition = position; 
   
  _UpdateWorldSpaceBounds(); 
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::SetRotation
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::SetRotation(const Mat33f &rotation)
 * @brief Set the world space rotation of the instance.
 * 
 * @todo <WRITE EXTENDED SkyRenderableInstanceCloud::SetRotation FUNCTION DOCUMENTATION>
 */ 
void SkyRenderableInstanceCloud::SetRotation(const Mat33f &rotation)
{ 
  if (_pCloud)
  {
    _pCloud->Translate(-_vecPosition);
    _pCloud->Rotate(_matInvRotation * rotation);
    _pCloud->Translate(_vecPosition);
  }
  _matRotation    = rotation;
  _matInvRotation = rotation; 
  _matInvRotation.Transpose();  
  _UpdateWorldSpaceBounds(); 
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::SetScale
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::SetScale(const float  &scale)
 * @brief Set the world space scale of the instance. 
 */ 
void SkyRenderableInstanceCloud::SetScale(const float  &scale)
{ 
  if (_pCloud)
  {
    _pCloud->Translate(-_vecPosition);
    _pCloud->Scale(scale);
    _pCloud->Translate(_vecPosition);
  }
  _rScale = scale; 
  _UpdateWorldSpaceBounds(); 
}


//------------------------------------------------------------------------------
// Function     	  : DrawQuad
// Description	    : 
//------------------------------------------------------------------------------
/**
 * DrawQuad(Vec3f pos, Vec3f x, Vec3f y, Vec4f color)
 * @brief Simply draws an OpenGL quad at @a pos.
 *
 * The quad's size and orientation are determined by the (non-unit) vectors @a x
 * and @a y.  Its color is given by @a color.
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
// Function     	  : SkyRenderableInstanceCloud::Display
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::Display(bool bDisplayFrontOfSplit)
 * @brief Display the instance of the cloud using the impostor image.
 */ 
SKYRESULT SkyRenderableInstanceCloud::Display(bool bDisplayFrontOfSplit /* = false */)
{
	
  if (!_bImageExists || !_bEnabled)
  {
    //FAIL_RETURN(DisplayWithoutImpostor(*(GLVU::GetCurrent()->GetCurrentCam())));
    FAIL_RETURN(DisplayWithoutImpostor(Camera::Camera()));
  }
  else
  {//cout << "Using impostor image\n";
    if (!_pBackTexture || (bDisplayFrontOfSplit && !_pFrontTexture))
      FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyRenderableInstanceCloud::Display(): missing texture!");

    s_pMaterial->SetTexture(0, GL_TEXTURE_2D, bDisplayFrontOfSplit ? *_pFrontTexture : *_pBackTexture);     
    if (_bScreenImpostor)
    {
      s_pMaterial->EnableDepthTest(false);
    }
    else if (_bSplit)
    {
      if (!bDisplayFrontOfSplit)
      {
        s_pMaterial->EnableDepthTest(true);
        s_pMaterial->SetDepthMask(false);
      }
      else
        s_pMaterial->EnableDepthTest(false);
    }
    else
    {
      s_pMaterial->EnableDepthTest(true);
      s_pMaterial->SetDepthMask(true); 
    }
     
    s_pMaterial->Activate();

    Vec3f x, y, z;
    
    if (!_bScreenImpostor)
    {//cout << "Outside the cloud\n";
      z  =    _vecPosition; 
      z -=    _impostorCam.Orig;
      z.Normalize();
      x  =    (z ^ _impostorCam.Y);
      x.Normalize();
      x *=    _rRadius;
      y  =    (x ^ z);
      y.Normalize();
      y *=    _rRadius;      

      glBegin(GL_QUADS);
      DrawQuad(_vecPosition, x, y, Vec4f(1, 1, 1, 1));
      glEnd();	
    }
    else
    { //cout <<  "Drawing a polygon - must be inside a cloud\n";
      x  =  _impostorCam.X;
      x *=  0.5f * (_impostorCam.wR - _impostorCam.wL);
      y  =  _impostorCam.Y;
      y *=  0.5f * (_impostorCam.wT - _impostorCam.wB);
      z  =  -_impostorCam.Z;
      z *=  _impostorCam.Near;

      // draw a polygon with this texture...
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      gluOrtho2D(-1, 1, -1, 1);
      
      glColor4f(1, 1, 1, 1);
      glBegin(GL_QUADS);
      glTexCoord2f(0, 0); glVertex2f(-1, -1);
      glTexCoord2f(1, 0); glVertex2f(1, -1);
      glTexCoord2f(1, 1); glVertex2f(1, 1);
      glTexCoord2f(0, 1); glVertex2f(-1, 1);
      glEnd();
      
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
    }
  }
  return SKYRESULT_OK;
}



//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::DisplayWithoutImpostor
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::DisplayWithoutImpostor(const Camera &cam)
 * @brief Displays the cloud directly -- without an impotor.
 * 
 * This is used both when the impostor is disabled and to create the impostor image
 * when it needs to be updated.
 */ 
SKYRESULT SkyRenderableInstanceCloud::DisplayWithoutImpostor(const Camera &cam)
{
  // Get and set the world space transformation
  /*Mat44f mat;
  GetModelToWorldTransform(mat);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glMultMatrixf(mat.M);*/
 
  FAIL_RETURN_MSG(_pCloud->Display(cam, this), "SkyRenderableInstanceCloud:Display(): Cloud's display failed.");

  //glMatrixMode(GL_MODELVIEW);
  //glPopMatrix();

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::ViewFrustumCull
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::ViewFrustumCull(const Camera &cam)
 * @brief View frustum cull the object given its world position
 */ 
bool SkyRenderableInstanceCloud::ViewFrustumCull(const Camera &cam)
{
  Mat44f xform;
  //GetModelToWorldTransform(xform);
  xform.Identity();
  _bCulled = (_pWorldSpaceBV == NULL) ? false : _pWorldSpaceBV->ViewFrustumCull(cam, xform);
  return _bCulled;
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::ReleaseImpostorTextures
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::ReleaseImpostorTextures()
 * @brief Causes the instance to release its impostor textures for use by other impostors.
 * 
 * This method is called when the cloud is view frustum culled.
 */ 
void SkyRenderableInstanceCloud::ReleaseImpostorTextures()
{
  _iCulledCount++;

  if (_iCulledCount > SKYCLOUD_CULL_RELEASE_COUNT)
  {
    _iCulledCount = 0;
  
    if (_pBackTexture)
    {
      DynamicTextureManager::InstancePtr()->CheckInTexture(_pBackTexture);
      _pBackTexture = NULL;
    }
   
    if (_pFrontTexture)
    {
      DynamicTextureManager::InstancePtr()->CheckInTexture(_pFrontTexture);
      _pFrontTexture = NULL;
    }
    _bImageExists = false;
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::Update
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::Update(const Camera &cam)
 * @brief Updates the impostor image to be valid for the current viewpoint.
 * 
 * If the image is already valid, exits early.
 *
 * @see SetErrorToleranceAngle, IsValid
 */ 
SKYRESULT SkyRenderableInstanceCloud::Update(const Camera &cam)
{
  if (!_bEnabled || IsImpostorValid(cam)) 
    return SKYRESULT_OK;

  // since we are going to update it anyway, let's make sure we don't try to use it if something
  // goes wrong.  This will be set to true on the successful completion of this Update() method.
  _bImageExists = false;
//cout << "updating impostor\n";
  Mat44f M;
  
  _impostorCam = cam;
  float rDistance  = (_vecPosition - cam.Orig).Length();
  
  float rRadius    = _pWorldSpaceBV->GetRadius();
  float rCamRadius = sqrt(cam.wR*cam.wR + cam.Near*cam.Near);
  
  float rWidth     = cam.wR - cam.wL;
  float rHeight    = cam.wT - cam.wB;
  float rMaxdim    = (rWidth > rHeight) ? rWidth : rHeight;
  
  if (rRadius * cam.Near / rDistance < 0.5 * rMaxdim && (rDistance - rRadius > rCamRadius)) 
  { // outside cloud
    _impostorCam.TightlyFitToSphere(cam.Orig, cam.Y, _vecPosition, rRadius);
    _rRadius = 0.5f * (_impostorCam.wR - _impostorCam.wL) * rDistance / _impostorCam.Near;
    _rRadius *= GetScale();
    _bScreenImpostor = false;	
    // store points used in later error estimation 
    _vecNearPoint   =   -_impostorCam.Z;
    _vecNearPoint   *=  _impostorCam.Near;
    _vecNearPoint   +=  _impostorCam.Orig;
    _vecFarPoint    =   -_impostorCam.Z;
    _vecFarPoint    *=  _impostorCam.Far;
    _vecFarPoint    +=  _impostorCam.Orig;
  }
  else // inside cloud
  {
    _impostorCam.Far  =  _impostorCam.Near + 3 * rRadius;
    _bScreenImpostor =  true;
  }
  
  // resolution based on screensize, distance, and object size.
  // Cam radius is used to heuristically reduce resolution for clouds very close to the camera.
  _iLogResolution = _GetRequiredLogResolution(rDistance, rRadius, rCamRadius);

  int iRes = 1 << _iLogResolution;

  int iOldVP[4];

  glGetIntegerv(GL_VIEWPORT, iOldVP);
  
  _impostorCam.GetProjectionMatrix(M);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadMatrixf(M);
  
  _impostorCam.GetModelviewMatrix(M);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadMatrixf(M);
  
  glViewport(0, 0, iRes, iRes);
  
  s_pMaterial->SetDepthMask(true);  // so that the depth buffer gets cleared!
  s_pMaterial->Activate();
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  if (!_bSplit)
  {
    FAIL_RETURN(DisplayWithoutImpostor(_impostorCam));
    
    if (_pBackTexture && _pBackTexture->GetWidth() != iRes)
    {
      DynamicTextureManager::InstancePtr()->CheckInTexture(_pBackTexture);
      _pBackTexture = NULL;
    }
    
    if (!_pBackTexture)
    {
      _pBackTexture = DynamicTextureManager::InstancePtr()->CheckOutTexture(iRes, iRes);
    }
    
    s_pMaterial->SetTexture(0, GL_TEXTURE_2D, *_pBackTexture);     // shared material for clouds. 
    s_pMaterial->Activate();
    
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, iRes, iRes);
  }
  else
  {  
    FAIL_RETURN_MSG(_pCloud->DisplaySplit(cam, _vecSplit, true, this), 
                    "SkyRenderableInstanceCloud:Display(): Cloud's display failed.");

    if (_pBackTexture && _pBackTexture->GetWidth() != iRes)
    {
      DynamicTextureManager::InstancePtr()->CheckInTexture(_pBackTexture);
      _pBackTexture = NULL;
    }
    if (_pFrontTexture && _pFrontTexture->GetWidth() != iRes)
    {
      DynamicTextureManager::InstancePtr()->CheckInTexture(_pFrontTexture);
      _pFrontTexture = NULL;
    }

    if (!_pBackTexture)
    {
      _pBackTexture = DynamicTextureManager::InstancePtr()->CheckOutTexture(iRes, iRes);
    }

    s_pMaterial->SetTexture(0, GL_TEXTURE_2D, *_pBackTexture);     // shared material for clouds. 
    FAIL_RETURN(s_pMaterial->Activate());
   
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, iRes, iRes);

    // now clear and draw the front.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    FAIL_RETURN_MSG(_pCloud->DisplaySplit(cam, _vecSplit, false, this), 
                    "SkyRenderableInstanceCloud:Display(): Cloud's display failed.");

    if (!_pFrontTexture)
    {
      _pFrontTexture = DynamicTextureManager::InstancePtr()->CheckOutTexture(iRes, iRes);
    }

    s_pMaterial->GetTextureState().SetTexture(0, GL_TEXTURE_2D, *_pFrontTexture);
    FAIL_RETURN(s_pMaterial->GetTextureState().Activate());

    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, iRes, iRes);
  }
  
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  
  //GLVU::CheckForGLError("Cloud Impostor Update");

    glViewport(iOldVP[0], iOldVP[1], iOldVP[2], iOldVP[3]);

  _bImageExists = true;

  // the textures should now exist.
  assert(_pBackTexture);
  assert(!_bSplit || (_bSplit && _pFrontTexture));

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::IsImpostorValid
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::IsImpostorValid(const Camera& cam)
 * @brief Returns true if the impostor image is valid for the given camera.
 * 
 * Will return false if this is a screen impostor, or if there is error in either
 * the translation of the camera from the capture point or the impostor image resolution.
 *
 * @see SetErrorToleranceAngle
 */ 
bool SkyRenderableInstanceCloud::IsImpostorValid(const Camera& cam)
{ 
  // first make sure there is a current image.
  if (!_bImageExists) 
    return false;

  // screen impostors should always be updated
  if (_bScreenImpostor)
  {
    _vecFarPoint = Vec3f::ZERO;
    _vecNearPoint = Vec3f::ZERO;
#if SKYCLOUD_VERBOSE
    SkyTrace("Screen Impostor Update");
#endif
    return false;
  }
  // impostors are valid from the viewpoint from which they were captured
  if (cam.Orig == _impostorCam.Orig) 
    return true;
  
  if (_bSplit) 
  {
  #if SKYCLOUD_VERBOSE
    SkyTrace("Split Impostor Update");
  #endif
    return false;
  }
  
  Vec3f vecX = _vecNearPoint - cam.Orig;
  Vec3f vecY = _vecFarPoint - cam.Orig;
  float rXLength = vecX.Length();
  float rYLength = vecY.Length();
  if (rXLength > rYLength) 
  {
#if SKYCLOUD_VERBOSE
    SkyTrace("Backwards Impostor Update");
#endif
    return false;
  }
  
  vecX /= rXLength; 
  vecY /= rYLength; 
  float rCosAlpha = vecX * vecY; // dot product of normalized vectors = cosine
  
  if (fabs(rCosAlpha) < 1.0) 
  {
    float rAlpha = acos(rCosAlpha);
    if (rAlpha >= s_rErrorToleranceAngle) 
    {
#if SKYCLOUD_VERBOSE
      SkyTrace("Angle Error Update %f", SKYRADSTODEGREES * rAlpha);
#endif
      return false;
    }
  }
 
  float rDistance = (_vecPosition - cam.Orig).Length();
  float rCamRadius = sqrt(cam.wR*cam.wR + cam.Near*cam.Near);
  
  int iRes = _GetRequiredLogResolution(rDistance, _pWorldSpaceBV->GetRadius(), rCamRadius);

  if (iRes > _iLogResolution)
  {
#if SKYCLOUD_VERBOSE
    SkyTrace("Resolution Error Update: Required: %d Actual: %d", iRes, _iLogResolution);
#endif
    return false;
  }
 
  return true;
}


//------------------------------------------------------------------------------
  // Function     	  : SetErrorToleranceAngle
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
  * @fn SkyRenderableInstanceCloud::SetErrorToleranceAngle(float rDegrees)
  * @brief Set the global error tolerance angle for all impostors.
  */ 
void SkyRenderableInstanceCloud::SetErrorToleranceAngle(float rDegrees)
{ 
  s_rErrorToleranceAngle = SKYDEGREESTORADS * rDegrees; 
}



//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::_Initialize
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::_Initialize()
 * @brief Initializer used by the constructors.
 */ 
void SkyRenderableInstanceCloud::_Initialize()
{
  _UpdateWorldSpaceBounds();

//  if (!s_pRenderBuffer && _bUseOffScreenBuffer)
//  {
//JW??    s_pRenderBuffer = new SkyOffScreenBuffer(GLUT_SINGLE | GLUT_DEPTH | GLUT_STENCIL);
//JW??    s_pRenderBuffer->Initialize(true);
    
//JW??    s_pRenderBuffer->MakeCurrent();
    // set some GL state:
//    glClearColor(0, 0, 0, 0);
//JW??    GLVU::GetCurrent()->MakeCurrent();
//  }
  if (!s_pMaterial)
  {
    s_pMaterial = new SkyMaterial;
    s_pMaterial->EnableBlending(true);
    s_pMaterial->SetBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    s_pMaterial->SetAlphaFunc(GL_GREATER);
    s_pMaterial->EnableDepthTest(false);
    s_pMaterial->SetDepthMask(true);
    s_pMaterial->EnableAlphaTest(true);
    s_pMaterial->EnableLighting(false);
    s_pMaterial->SetTextureParameter(0, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    s_pMaterial->SetTextureParameter(0, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    s_pMaterial->SetTextureParameter(0, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    s_pMaterial->EnableTexture(0, true);    
  }
  s_iCount++;
}
