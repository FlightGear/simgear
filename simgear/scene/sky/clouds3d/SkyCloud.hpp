//------------------------------------------------------------------------------
// File : SkyCloud.hpp
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
 * @file SkyCloud.hpp
 * 
 * Interface definition for class SkyCloud.
 */
#ifndef __SKYCLOUD_HPP__
#define __SKYCLOUD_HPP__

// warning for truncation of template name for browse info
#pragma warning( disable : 4786)

#include "SkyCloudParticle.hpp"
#include "SkyRenderable.hpp"
#include "SkyMinMaxBox.hpp"
#include "camera.hpp"
#include "SkyArchive.hpp"
#include "mat33.hpp"

#include <plib/sg.h>

class SkyMaterial;
class SkyLight;
class SkyRenderableInstance;


//------------------------------------------------------------------------------
/**
 * @class SkyCloud
 * @brief A renderable that represents a volumetric cloud.
 * 
 * A SkyCloud is made up of particles, and is rendered using particle splatting.  
 * SkyCloud is intended to represent realisticly illuminated volumetric clouds
 * through which the viewer and / or other objects can realistically pass.
 *
 * Realistic illumination is performed by the Illuminate() method, which uses a
 * graphics hardware algorithm to precompute and store multiple forward scattering 
 * of light by each particle in the cloud.  Clouds may be illuminated by multiple
 * light sources.  The light from each source is precomputed and stored at each
 * particle.  Each light's contribution is stored separately so that we can 
 * compute view-dependent (anisotropic) scattering at run-time.  This gives realistic
 * effects such as the "silver lining" that you see on a thick cloud when it crosses
 * in front of the sun.
 *
 * At run-time, the cloud is rendered by drawing each particle as a view-oriented 
 * textured billboard (splat), with lighting computed from the precomputed illumination
 * as follows: for each light source <i>l</i>, compute the scattering function (See _PhaseFunction())
 * based on the view direction and the direction from the particle to the viewer.  This 
 * scattering function modulates the lighting contribution of <i>l</i>.  The modulated 
 * contributions are then added and used to modulate the color of the particle.  The result
 * is view-dependent scattering.  
 * 
 * If the phase (scattering) function is not enabled (see IsPhaseFunctionEnabled()), then the 
 * contributions of the light sources are simply added.
 *
 * @see SkyRenderableInstanceCloud, SkyCloudParticle, SkySceneManager
 */
class SkyCloud : public SkyRenderable
{
public:
  SkyCloud();
  virtual ~SkyCloud();
  
  virtual SKYRESULT           Update(const Camera &cam, SkyRenderableInstance* pInstance = NULL);
  
  virtual SKYRESULT           Display(const Camera &camera, SkyRenderableInstance *pInstance = NULL);
 
  SKYRESULT                   DisplaySplit(const Camera           &camera, 
                                           const Vec3f            &vecSplitPoint,
                                           bool                   bBackHalf,
                                           SkyRenderableInstance  *pInstance = NULL);

  SKYRESULT                   Illuminate( SkyLight *pLight, 
                                          SkyRenderableInstance* pInstance,
                                          bool bReset = false);
  
  virtual SkyMinMaxBox*       CopyBoundingVolume() const;
  
  //! Enables the use of a scattering function for anisotropic light scattering.
  void                        EnablePhaseFunction(bool bEnable)   { _bUsePhaseFunction = bEnable; }
  //! Returns true if the use of a scattering function is enabled.
  bool                        IsPhaseFunctionEnabled() const      { return _bUsePhaseFunction;    }
  
  SKYRESULT                   Save(SkyArchive &archive) const;
  SKYRESULT                   Load(const SkyArchive &archive, float rScale = 1.0f,double latitude=0.0, double longitude=0.0);
  SKYRESULT                   Load(const unsigned char *data, unsigned int size, float rScale = 1.0f,double latitude=0.0,double longitude=0.0);
  
  void                        Rotate(const Mat33f& rot);
  void                        Translate(const Vec3f& trans);
  void                        Scale(const float scale);

protected: // methods
  enum SortDirection
  {
    SKY_CLOUD_SORT_TOWARD,
    SKY_CLOUD_SORT_AWAY
  };

  void					              _SortParticles(	const Vec3f& vecViewDir,
                                              const Vec3f& vecSortPoint, 
                                              SortDirection dir);
  
  void                        _CreateSplatTexture( unsigned int iResolution);
  
  float                       _PhaseFunction(const Vec3f& vecLightDir, const Vec3f& vecViewDir);
  
protected: // datatypes 
  typedef std::vector<SkyCloudParticle*>  ParticleArray;
  typedef ParticleArray::iterator         ParticleIterator;
  typedef ParticleArray::const_iterator   ParticleConstIterator;
  
  typedef std::vector<Vec3f>              DirectionArray;
  typedef DirectionArray::iterator        DirectionIterator;
  
  class ParticleAwayComparator
  {
  public:
    bool operator()(SkyCloudParticle* pA, SkyCloudParticle *pB)
    {
      return ((*pA) < (*pB));
    }
  };
  
  class ParticleTowardComparator
  {
  public:
    bool operator()(SkyCloudParticle* pA, SkyCloudParticle *pB)
    {
      return ((*pA) > (*pB));
    }
  };

protected: // data
  ParticleArray               _particles;       // cloud particles
  // particle sorting functors for STL sort.
  ParticleTowardComparator    _towardComparator; 
  ParticleAwayComparator      _awayComparator; 

  DirectionArray              _lightDirections; // light directions in cloud space (cached)

  SkyMinMaxBox                _boundingBox;     // bounds

  bool                        _bUsePhaseFunction;

  Vec3f                       _vecLastSortViewDir;
  Vec3f                       _vecLastSortCamPos;
  Vec3f                       _vecSortPos;

  float                       _rSplitDistance;
  
  static SkyMaterial          *s_pMaterial;     // shared material for clouds. 
  static SkyMaterial          *s_pShadeMaterial;// shared material for illumination pass.
  static unsigned int         s_iShadeResolution; // the resolution of the viewport used for shading
  static float                s_rAlbedo;        // the cloud albedo
  static float                s_rExtinction;    // the extinction of the clouds
  static float                s_rTransparency;  // the transparency of the clouds
  static float                s_rScatterFactor; // How much the clouds scatter
  static float                s_rSortAngleErrorTolerance; // how far the view must turn to cause a resort.
  static float                s_rSortSquareDistanceTolerance; // how far the view must move to cause a resort.
};

#endif //__SKYCLOUD_HPP__
