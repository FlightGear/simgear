//------------------------------------------------------------------------------
// File : SkySceneManager.hpp
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
 * @file SkySceneManager.hpp
 * 
 * The SkySceneManager class manages all of the renderable objects and
 * instances.  This class maintains lists of each object and instance.
 * The scene manager decides what to display, it can make use of various
 * techniques such as view frustum culling, material grouping, etc.
 */
#ifndef __SKYSCENEMANAGER_HPP__
#define __SKYSCENEMANAGER_HPP__

// warning for truncation of template name for browse info
#pragma warning( disable : 4786)

#include "vec3f.hpp"
#include <vector>
#include <map>

#include "SkyUtil.hpp"
#include "SkySingleton.hpp"
#include "SkyRenderableInstance.hpp"
#include "SkyMaterial.hpp"
#include "SkyAABBTree.hpp"
#include "SkyRenderableInstanceCloud.hpp"

// forward to reduce unnecessary dependencies
class Camera;
class SkyRenderable;
class SkyRenderableInstance;
class SkyMaterial;
class SkyLight;
class SkyCloud;
//class SkyHeavens;
//class SkyHeightField;

//------------------------------------------------------------------------------
/**
* @class SkySceneManager
* @brief Manages all of the renderable objects and instances.
*/
class SkySceneManager;  // Forward declaration

//! A singleton of the SkySceneManager.  Can only create the SceneManager with SceneManager::Instantiate();
typedef SkySingleton<SkySceneManager> SceneManager;

class SkySceneManager
{
public:
  SKYRESULT     AddObject(   SkyRenderable *pObject);
  SKYRESULT     AddInstance( SkyRenderableInstance *pInstance, bool bTransparent = false);
  
  SKYRESULT     AddCloud(    SkyCloud *pCloud);
  SKYRESULT     AddCloudInstance(SkyRenderableInstanceCloud *pInstance);
  
  SKYRESULT     AddMaterial( SkyMaterial *pMaterial);
  SkyMaterial*  GetMaterial( int iMaterialID);
  SKYRESULT     ActivateMaterial(int iMaterialID);
  
  SKYRESULT     AddLight(    SkyLight *pLight);
  SkyLight*     GetLight(    int iLightID);

  //! Set the sky box for this scene.
 // void          SetSkyBox(   SkyHeavens *pSkyBox)       { _pSkyBox = pSkyBox;   }
  //! Set the terrain for this scene.
  //void          SetTerrain(  SkyHeightField *pTerrain)  { _pTerrain = pTerrain; }

  //! Enable wireframe display of lights (for debugging).
  void          EnableDrawLights(bool bEnable)          { _bDrawLights = bEnable; }
  //! Enable wireframe display of bounding volume tree of clouds.
  void          EnableDrawBVTree(bool bEnable)          { _bDrawTree   = bEnable; }

  //! Returns true if wireframe display of lights is enabled.
  bool          IsDrawLightsEnabled() const             { return _bDrawLights;  }
  //! Returns true if wireframe display of the cloud bounding volume tree is enabled.
  bool          IsDrawBVTreeEnabled() const             { return _bDrawTree;    }
  
  SKYRESULT     Update( const Camera &cam);
  SKYRESULT     Display( const Camera &cam);

  SKYRESULT     RebuildCloudBVTree();
  SKYRESULT     ShadeClouds();

  //! Force the illumination of all clouds to be recomputed in the next update.
  void          ForceReshadeClouds()                    { _bReshadeClouds = true; }

  // sort instances in @a instances from back to front.
  static void		SortInstances(InstanceArray& instances, const Vec3f& vecSortPoint);

  // load a set of clouds from an archive file.
  SKYRESULT     LoadClouds(unsigned char *data, unsigned int size, float rScale = 1.0f, double latitude=0.0, double longitude=0.0);
  SKYRESULT     LoadClouds(SkyArchive& cloudArchive, float rScale = 1.0f, double latitude=0.0, double longitude=0.0);
  
protected: // datatypes
  // Typedef the vectors into cleaner names
  typedef std::vector<SkyRenderable*>               ObjectArray;
  
  typedef std::map<int, SkyMaterial*>               MaterialSet;
  typedef std::map<int, SkyLight*>                  LightSet;

  typedef std::vector<SkyRenderableInstanceCloud*>  CloudInstanceArray;
  typedef std::vector<SkyCloud*>                    CloudArray;
  typedef std::map<int, SkyContainerCloud*>         ContainerCloudSet;
  
  typedef SkyAABBTree<SkyRenderableInstanceCloud*>  CloudBVTree;
  
  typedef ObjectArray::iterator                     ObjectIterator;
  typedef CloudArray::iterator                      CloudIterator;
  typedef CloudInstanceArray::iterator              CloudInstanceIterator;
  typedef MaterialSet::iterator                     MaterialIterator;
  typedef LightSet::iterator                        LightIterator;
  typedef ContainerCloudSet::iterator               ContainerSetIterator;
  
  class InstanceComparator
  {
  public:
    bool operator()(SkyRenderableInstance* pA, SkyRenderableInstance *pB)
    {
      return ((*pA) < (*pB));
    }
  };

  class ContainerComparator
  {
  public:
    bool operator()(SkyContainerCloud* pA, SkyContainerCloud *pB)
    {
      return ((*pA) < (*pB));
    }
  };
  

protected: // methods
  SkySceneManager();
  ~SkySceneManager();
  
  void					_SortClouds(CloudInstanceArray& clouds,  const Vec3f& vecSortPoint);

  void          _ViewFrustumCullClouds(const Camera& cam, const CloudBVTree::Node *pNode);
  void          _VisualizeCloudBVTree(const Camera& cam, const CloudBVTree::Node *pNode);
  
  SKYRESULT     _ResolveVisibility(const Camera &cam);
  bool          _TestInsertInstanceIntoClouds(const Camera              &cam,
                                              const CloudBVTree::Node   *pNode,
                                              SkyRenderableInstance     *pInstanceToInsert,
                                              bool                      bTransparent);
  
private: // data
  ObjectArray         _objects;
  CloudArray          _clouds;
  InstanceArray       _instances;
  InstanceArray       _transparentInstances;
  InstanceArray       _visibleInstances;      //! @TODO: change this to "_freeInstances"
  CloudInstanceArray  _cloudInstances;
  CloudInstanceArray  _visibleCloudInstances;
  ContainerCloudSet   _containerClouds;

  MaterialSet         _materials;
  LightSet            _lights;

  SkyMaterial         _wireframeMaterial;  // used for rendering the wireframes for debugging
  SkyMaterial         _clearMaterial;      // used to maintain state consistency when clearing.
  CloudBVTree         _cloudBVTree;

  //SkyHeavens          *_pSkyBox;
  //SkyHeightField      *_pTerrain;

  bool                _bDrawLights;
  bool                _bDrawTree;
  bool                _bReshadeClouds;
};

#endif //__SKYSCENEMANAGER_HPP__  
