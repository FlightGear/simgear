//------------------------------------------------------------------------------
// File : SkyRenderableInstanceCloud.hpp
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
 * @file SkyRenderableInstanceCloud.hpp
 * 
 * Interface for class SkyRenderableInstanceCloud, an instance of a SkyCloud object.
 */
#ifndef __SKYRENDERABLEINSTANCECLOUD_HPP__
#define __SKYRENDERABLEINSTANCECLOUD_HPP__

class  SkyCloud;
class  SkyTexture;
//class  SkyOffScreenBuffer;

#include <vector>
#include "camera.hpp"
#include "SkyContext.hpp"
#include "SkyRenderableInstance.hpp"
#include "SkyRenderableInstanceGeneric.hpp"
#include "SkyCloud.hpp"

//------------------------------------------------------------------------------
/**
 * @class SkyRenderableInstanceCloud
 * @brief An instance of a cloud.  Renders using an impostor.
 * 
 * @todo <WRITE EXTENDED CLASS DESCRIPTION>
 */
class SkyRenderableInstanceCloud : public SkyRenderableInstance
{
public:
  SkyRenderableInstanceCloud( SkyCloud      *pCloud, bool bUseOffScreenBuffer = false);
  SkyRenderableInstanceCloud( SkyCloud      *pCloud, 
                              const Vec3f   &position, 
                              const Mat33f  &rotation, 
                              const float   scale,
                              bool          bUseOffScreenBuffer = false);
  virtual ~SkyRenderableInstanceCloud();
  
  // Setters / Getters
    //! Sets the identifier for this cloud (used by SkySceneManager.)
  void                   SetID(int id)                        { _iCloudID = id;               }
  
  virtual void           SetPosition(const Vec3f  &position);
  
  virtual void           SetRotation(const Mat33f &rotation);
  
  virtual void           SetScale(const float  &scale);

  //! Returns SkySceneManager's id for this cloud.
  int                    GetID() const                        { return _iCloudID;             }
  //! Returns a pointer to the renderable that this instance represents.
  virtual SkyCloud*      GetCloud() const                     { return _pCloud;               }
  
  //! Returns the world space bounding box of the cloud instance.
  const SkyMinMaxBox&    GetWorldSpaceBounds()                { return *_pWorldSpaceBV;      }

  //! Make this into a split impostor.  Will be rendered as two halves, around the split point.
  void                   SetSplit(bool bSplit)                { _bSplit = bSplit;             }
  
  //! Set the distance at which the cloud will be split (from the camera position).
  void                   SetSplitPoint(const Vec3f& vecSplit) { _vecSplit = vecSplit;         }

  //! Returns true if this is a split impostor (cloud contains objects)
  bool                   IsSplit() const                      { return _bSplit;               }

  //! Returns true if the camera is inside this clouds bounding volume.
  bool                   IsScreenImpostor() const             { return _bScreenImpostor;      }
  
  virtual SKYRESULT      Update(const Camera& cam);
  virtual SKYRESULT      Display(bool bDisplayFrontOfSplit = false);
  SKYRESULT              DisplayWithoutImpostor(const Camera& cam);
    
  virtual bool           ViewFrustumCull( const Camera &cam );

  void                   ReleaseImpostorTextures();

  
  virtual SkyMinMaxBox*  GetBoundingVolume() const            { return _pWorldSpaceBV;        }
  
  //----------------------------------------------------------------------------
  // Determine if the current impostor image is valid for the given viewpoint
  //----------------------------------------------------------------------------
  bool                   IsImpostorValid(const Camera &cam);
  
  //------------------------------------------------------------------------------
  // Function     	  : Enable
  // Description	    : 
  //------------------------------------------------------------------------------
  /**
  * @fn Enable(bool bEnable)
  * @brief Enable / Disable the use of impostor images (if disabled, displays geometry).
  */ 
  void                   Enable(bool bEnable)                 { _bEnabled = bEnable;          }
  
  static void            SetErrorToleranceAngle(float rDegrees);  

protected: // methods
  void        _Initialize();
  inline int  _GetRequiredLogResolution(float rObjectDistance, float rObjectRadius, float rCamRadius);
  
  inline void _UpdateWorldSpaceBounds();

protected: // data
  int               _iCloudID;        // used by the scene manager to identify clouds.

  SkyCloud          *_pCloud;         // Pointer to the cloud object
  SkyMinMaxBox      *_pWorldSpaceBV; // Pointer to bounding volume in object space
  
  float             _rRadius;         // Radius of the impostor.
  bool              _bScreenImpostor; // Is this a screen space or world space impostor?
  bool              _bImageExists;    // The impostor image exists and is ready to use.
  bool              _bEnabled;        // if disabled, draw geometry -- otherwise, draw impostor.
  bool              _bUseOffScreenBuffer;  // if enabled, uses off-screen rendering to create impostor images.
  
  bool              _bSplit;          // true if the cloud contains other object instances.
  Vec3f             _vecSplit;        // the point about which this cloud is split.

  Vec3f             _vecNearPoint;    // Nearest point on bounding sphere to viewpoint.
  Vec3f             _vecFarPoint;     // Farthest point on bounding sphere from viewpoint.
  
  Camera            _impostorCam;     // camera used to generate this impostor
  
  unsigned int      _iLogResolution;  // Log base 2 of current impostor image resolution.

  SkyTexture        *_pBackTexture;   // Back texture for split clouds or main texture for unsplit.
  SkyTexture        *_pFrontTexture;  // Front texture for split clouds.

  unsigned int      _iCulledCount;    // used to determine when to release textures
  
  static unsigned int        s_iCount;  // keep track of number of impostors using the render buffer.
//JW??  static SkyOffScreenBuffer* s_pRenderBuffer;
  static float               s_rErrorToleranceAngle;
  static SkyMaterial        *s_pMaterial; // shared material for cloud impostors. 
};


//------------------------------------------------------------------------------
/**
 * @class SkyContainerCloud
 * @brief A class used to organize the rendering order of cloud impostors and the objects in the clouds
 * 
 * @todo <WRITE EXTENDED CLASS DESCRIPTION>
 */
class SkyContainerCloud : public SkyRenderableInstance
{
public: // methods
  //! Constructor.
  SkyContainerCloud(SkyRenderableInstanceCloud *cloud) : SkyRenderableInstance(), pCloud(cloud) {}
  //! Destructor.
  ~SkyContainerCloud() 
  { pCloud = NULL; containedOpaqueInstances.clear(); containedTransparentInstances.clear(); }

  virtual SKYRESULT Display()                      
  {
    // display the back half of the split impostor.
    FAIL_RETURN_MSG(pCloud->Display(false),
                    "SkySceneManager::Display(): cloud instance display failed.");

    if (pCloud->IsSplit())
    {
      // display contained instances -- first opaque, then transparent.
      InstanceIterator ii;
      for (ii = containedOpaqueInstances.begin(); ii != containedOpaqueInstances.end(); ++ii)
        FAIL_RETURN((*ii)->Display());
      for (ii = containedTransparentInstances.begin(); ii != containedTransparentInstances.end(); ++ii)
        FAIL_RETURN((*ii)->Display());

      // now display the front half of the split impostor.
      FAIL_RETURN_MSG(pCloud->Display(true),
                      "SkySceneManager::Display(): cloud instance display failed.");
    }  

    return SKYRESULT_OK;
  }

  virtual const Vec3f& GetPosition() const { return pCloud->GetPosition(); }

public: //data -- here the data are public because the interface is minimal.
  SkyRenderableInstanceCloud *pCloud;

  InstanceArray               containedOpaqueInstances;
  InstanceArray               containedTransparentInstances;

  // This operator is used to sort ContainerCloud arrays. 
  bool operator<(const SkyContainerCloud& container) const
  {
    return (*((SkyRenderableInstance*)pCloud) < *((SkyRenderableInstance*)container.pCloud));
  }
};


//------------------------------------------------------------------------------
// Function     	  : _GetRequiredLogResolution
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::_GetRequiredLogResolution(float rObjectDistance, float rObjectRadius, float rCamRadius)
 * @brief Returns the integer logarithm base two of the expected impostor resolution.
 * 
 * Impostor resolution is based on object size, distance, and the FOV of the camera (stored as the
 * radius of a sphere centered at the camera position and passing through the corners of the 
 * projection plane. 
 */ 
inline int SkyRenderableInstanceCloud::_GetRequiredLogResolution(float rObjectDistance, 
                                                                 float rObjectRadius, 
                                                                 float rCamRadius)
{
  int iScreenWidth, iScreenHeight;
  GraphicsContext::InstancePtr()->GetWindowSize(iScreenWidth, iScreenHeight);
  //cout << "SkyRes: w=" << iScreenWidth << "h=" << iScreenHeight << endl; char ff; cin >> ff;
  int iScreenResolution = (iScreenWidth > iScreenHeight) ? iScreenWidth : iScreenHeight;
  int iLogMinScreenResolution = (iScreenWidth > iScreenHeight) ? iScreenHeight : iScreenWidth;
  iLogMinScreenResolution = SkyGetLogBaseTwo(iLogMinScreenResolution) - 1;

  int iRes = 2 * iScreenResolution * _pWorldSpaceBV->GetRadius() / rObjectDistance;
  int iLogResolution;
  
  if (rObjectDistance - (0.5f * rObjectRadius) < rCamRadius) 
  {
    iLogResolution = SkyGetLogBaseTwo(iScreenResolution / 8);
  }
  else if (rObjectDistance - rObjectRadius < rCamRadius) 
  {
    iLogResolution = SkyGetLogBaseTwo(iScreenResolution / 4);
  }
  else if (iRes > iScreenResolution) 
  {
    iLogResolution = SkyGetLogBaseTwo(iScreenResolution / 2);
  }
  else
  {
    iLogResolution = SkyGetLogBaseTwo(iRes);
  }

  // if not rendering to an off screen buffer, make sure the resolution fits in the window!
  if (!_bUseOffScreenBuffer && (iLogMinScreenResolution < iLogResolution))
    iLogResolution = iLogMinScreenResolution;

  return iLogResolution;
}


//------------------------------------------------------------------------------
// Function     	  : SkyRenderableInstanceCloud::_UpdateWorldSpaceBounds
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyRenderableInstanceCloud::_UpdateWorldSpaceBounds()
 * @brief Updates the world space bounding box of the cloud.
 */ 
inline void SkyRenderableInstanceCloud::_UpdateWorldSpaceBounds()
{
  SAFE_DELETE(_pWorldSpaceBV);
  _pWorldSpaceBV = _pCloud->CopyBoundingVolume();
  _vecPosition = _pCloud->CopyBoundingVolume()->GetCenter();
}

#endif //__SKYRENDERABLEINSTANCECLOUD_HPP__