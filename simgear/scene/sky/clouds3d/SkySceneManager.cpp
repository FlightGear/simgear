//------------------------------------------------------------------------------
// File : SkySceneManager.cpp
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
 * @file SkySceneManager.cpp
 * 
 * Implementation of the singleton class SkySceneManager, which manages objects,
 * instances, scene update, visibility, culling, and rendering.
 */

// warning for truncation of template name for browse info
#pragma warning( disable : 4786)

#include "SkySceneManager.hpp"
#include "SkyMaterial.hpp"
#include "SkyLight.hpp"
#include "SkyCloud.hpp"
#include "SkyRenderable.hpp"
#include "SkyRenderableInstance.hpp"
#include "SkyRenderableInstanceCloud.hpp"

#include "camutils.hpp"
#include <algorithm>

//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::SkySceneManager
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::SkySceneManager()
 * @brief Constructor
 */ 
SkySceneManager::SkySceneManager()
: /*_pSkyBox(NULL),
  _pTerrain(NULL),*/
  _bDrawLights(false),
  _bDrawTree(false),
  _bReshadeClouds(true)
{
  _wireframeMaterial.SetColorMaterialMode(GL_DIFFUSE);
  _wireframeMaterial.EnableColorMaterial(true);
  _wireframeMaterial.EnableLighting(false);

  // add the default material with ID -1 
  // this should avoid errors caused by models without materials exported from MAX
  // (because flexporter gives them the ID -1).
  SkyMaterial *pDefaultMaterial = new SkyMaterial;
  pDefaultMaterial->SetMaterialID(-1);
  AddMaterial(pDefaultMaterial);
}

//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::~SkySceneManager
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::~SkySceneManager()
 * @brief Destructor.
 *
 * This destructor deletes all renderables, instances (renderable instances, cloud instances, 
 * and otherwise), materials, and lights that were added to the scene using the Add*() functions.
 * In other words, the scene manager owns all entities added to the scene.  This eases cleanup 
 * and shutdown.
 */ 
SkySceneManager::~SkySceneManager()
{
  ObjectIterator oi;
  for (oi = _objects.begin(); oi != _objects.end(); ++oi)
    SAFE_DELETE(*oi);
  _objects.clear();

  CloudIterator ci;
  for (ci = _clouds.begin(); ci != _clouds.end(); ++ci)
    SAFE_DELETE(*ci);
  _clouds.clear();

  InstanceIterator ii;
  for (ii = _instances.begin(); ii != _instances.end(); ++ii)
    SAFE_DELETE(*ii);
  _instances.clear();

  CloudInstanceIterator cii;
  for (cii = _cloudInstances.begin(); cii != _cloudInstances.end(); ++cii)
    SAFE_DELETE(*cii);
  _cloudInstances.clear();

  ContainerSetIterator cni;
  for (cni = _containerClouds.begin(); cni != _containerClouds.end(); ++cni)
    SAFE_DELETE(cni->second);
  _containerClouds.clear();

  MaterialIterator mi;
  for (mi = _materials.begin(); mi != _materials.end(); ++mi)
    SAFE_DELETE(mi->second);
  _materials.clear();

  LightIterator li;
  for (li = _lights.begin(); li!= _lights.end(); ++li)
    SAFE_DELETE(li->second);
  _lights.clear();

  //SAFE_DELETE(_pSkyBox);
  //SAFE_DELETE(_pTerrain);
}

//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::AddObject
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::AddObject(SkyRenderable *pObject)
 * @brief Add a new SkyRenderable object to the manager.
 */ 
SKYRESULT SkySceneManager::AddObject(SkyRenderable *pObject)
{
  // Check for null object
  if (NULL == pObject)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, 
                    "SkySceneManager::AddObject(): Attempting to add NULL Renderable Object.");
  }

  _objects.push_back(pObject);

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::AddInstance
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkySceneManager::AddInstance(SkyRenderableInstance *pInstance, bool bTransparent)
* @brief Add a new SkyRenderableInstance to the manager.
*/ 
SKYRESULT SkySceneManager::AddInstance(SkyRenderableInstance *pInstance, bool bTransparent /* = false */)
{
  // Check for null instance
  if (NULL == pInstance)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, 
      "SkySceneManager::AddObject(): Attempting to add NULL Renderable Instance.");
  }
  
  if (!bTransparent)
    _instances.push_back(pInstance);
  else
    _transparentInstances.push_back(pInstance);
  
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::AddCloud
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::AddCloud(SkyCloud *pCloud)
 * @brief Add a new cloud object to the manager.
 * 
 * @todo <WRITE EXTENDED SkySceneManager::AddCloud FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkySceneManager::AddCloud(SkyCloud *pCloud)
{
  if (NULL == pCloud)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, 
      "SkySceneManager::AddObject(): Attempting to add NULL SkyCloud Object.");
  }
  
  _clouds.push_back(pCloud);
  
  return SKYRESULT_OK;
}



//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::AddCloudInstance
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::AddCloudInstance(SkyRenderableInstanceCloud *pInstance)
 * @brief Add a new instance of a cloud to the manager.
 * 
 * @todo Note that since cloud instances share shading information, if two instances
 * of a cloud have different orientations, one of the instances will have incorrect
 * lighting for the scene.  For this reason, I recommend that the number of clouds and 
 * cloud instances is equal.
 */ 
SKYRESULT SkySceneManager::AddCloudInstance(SkyRenderableInstanceCloud *pInstance)
{
  // Check for null instance
  if (NULL == pInstance)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, 
      "SkySceneManager::AddObject(): Attempting to add NULL SkyCloud Instance.");
  }
  
  pInstance->SetID(_cloudInstances.size());

  _cloudInstances.push_back(pInstance);

  SkyContainerCloud *pContainer = new SkyContainerCloud(pInstance);
  _containerClouds.insert(std::make_pair(pInstance->GetID(), pContainer));

  RebuildCloudBVTree();
  
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::AddMaterial
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkySceneManager::AddMaterial(SkyMaterial *pMaterial)
* @brief Adds a material to the scene.
* 
* Materials are kept in a map with their ID as key.  A material can be retrieved
* from the scene manager by passing its ID to GetMaterial.
*
* @see GetMaterial, SkyMaterial
*/ 
SKYRESULT SkySceneManager::AddMaterial(SkyMaterial *pMaterial)
{
  // Check for null instance
  if (NULL == pMaterial)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, 
      "SkySceneMananger::AddMaterial(): Attempting to add NULL Material to Scene Manager");
  }
  
  _materials.insert(std::make_pair(pMaterial->GetMaterialID(), pMaterial));
  return SKYRESULT_OK;
}



//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::GetMaterial
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::GetMaterial(int iMaterialID)
 * @brief Returns the material with ID @a iMaterialID.
 * 
 * If the material is not found, returns NULL.
 *
 * @see AddMaterial, SkyMaterial
 */ 
SkyMaterial* SkySceneManager::GetMaterial(int iMaterialID)
{
  MaterialIterator mi = _materials.find(iMaterialID);
  if (_materials.end() == mi)
  {
    SkyTrace("SkySceneManager::GetMaterial: Error: invalid material ID");
    return NULL;
  }
  else
    return mi->second;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::ActivateMaterial
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::ActivateMaterial(int iMaterialID)
 * @brief Makes the specified material active, setting the appropriate rendering state.
 * 
 * @todo <WRITE EXTENDED SkySceneManager::ActivateMaterial FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkySceneManager::ActivateMaterial(int iMaterialID)
{
//   cout  << "Activating material\n"; char mm; cin >> mm;
  MaterialIterator mi = _materials.find(iMaterialID);
  if (_materials.end() == mi)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, 
                    "SkySceneManager::ActivateMaterial: Error: invalid material ID.");
  }
  else
  {
    FAIL_RETURN_MSG(mi->second->Activate(), 
                    "SkySceneManager::ActivateMaterial: Error: failed to activate.");
  }

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::AddLight
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::AddLight(SkyLight *pLight)
 * @brief @todo <WRITE BRIEF SkySceneManager::AddLight DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::AddLight FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkySceneManager::AddLight(SkyLight *pLight)
{
  // Check for null instance
  if (NULL == pLight)
  {
    FAIL_RETURN_MSG(SKYRESULT_FAIL, 
                    "SkySceneMananger::AddLight(): Attempting to add NULL Light to Scene Manager");
  }
  
  _lights.insert(std::make_pair(_lights.size(), pLight));
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::GetLight
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::GetLight(int iLightID)
 * @brief @todo <WRITE BRIEF SkySceneManager::GetLight DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::GetLight FUNCTION DOCUMENTATION>
 */ 
SkyLight* SkySceneManager::GetLight(int iLightID)
{
  LightIterator li = _lights.find(iLightID);
  if (_lights.end() == li)
  {
    SkyTrace("SkySceneManager::GetLight: Error: Invalid light ID");
    return NULL;
  }
  else
    return li->second;
}


//------------------------------------------------------------------------------
// Function     	  : Alive
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn Alive(SkyRenderableInstance* pInstance)
 * @brief A predicate to determine if an object is dead or not.
 */ 
bool Alive(SkyRenderableInstance* pInstance)
{
  return (pInstance->IsAlive());
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::Update
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkySceneManager::Update(const Camera &cam)
* @brief Iterate through all SkyRenderableInstances and update them.
*/ 
SKYRESULT SkySceneManager::Update(const Camera &cam)
{
  _ResolveVisibility(cam);
  
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::Display
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkySceneManager::Display(const Camera &cam)
* @brief Iterate through all SkyRenderableInstances and display them.
*/ 
SKYRESULT SkySceneManager::Display( const Camera &cam )

{
  _clearMaterial.Activate();
  //glClear(GL_DEPTH_BUFFER_BIT);

  // set lights (only lights that have changed will be passed to GL).
  for (LightIterator li = _lights.begin(); li != _lights.end(); ++li)
  {
    li->second->Activate(li->first);
    //if (_bDrawLights)
    //li->second->Display();
  }
 
  //if (_bDrawTree)// force the issue and draw
    //_VisualizeCloudBVTree(cam, _cloudBVTree.GetRoot());
    
  glLineWidth(2.0);
  glBegin(GL_LINES);
  //  red is Cartesian y-axis
  glColor3ub( 255, 0, 0 );
  glVertex3f( 0.0,0.0,0.0 );
  glVertex3f( 0.0, -104000.0, 0.0);
  // yellow is Cartesian z-axis
  glColor3ub( 255, 255, 0 );
  glVertex3f( 0.0, 0.0, 0.0);
  glVertex3f( 0.0, 0.0, 104000.0);
  // blue is Cartesian x-axis
  glColor3ub( 0, 0, 255 );
  glVertex3f( 0.0, 0.0, 0.0);
  glVertex3f( -104000.0, 0.0, 0.0);
  glEnd();

  // draw all container clouds and "free" objects not in clouds.
  int i = 0;
  for (InstanceIterator iter = _visibleInstances.begin(); iter != _visibleInstances.end(); ++iter)
  {
    FAIL_RETURN_MSG((*iter)->Display(),
                  "SkySceneManager::Display(): instance display failed.");
    i++;
  }
  //cout << "There are " << i << " visible clouds\n";
 
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::RebuildCloudBVTree
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::RebuildCloudBVTree()
 * @brief Builds an AABB tree of the cloud bounding volumes.  
 */ 
SKYRESULT SkySceneManager::RebuildCloudBVTree()
{
  CloudInstanceIterator cii;
  SkyMinMaxBox bbox;

  _cloudBVTree.BeginTree();
  for (cii = _cloudInstances.begin(); cii != _cloudInstances.end(); ++cii)
  {
    bbox = (*cii)->GetWorldSpaceBounds();
    _cloudBVTree.AddObject(*cii, bbox);
  }
  _cloudBVTree.EndTree();

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::ShadeClouds
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::ShadeClouds()
 * @brief @todo <WRITE BRIEF SkySceneManager::ShadeClouds DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::ShadeClouds FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkySceneManager::ShadeClouds()
{
//   cout  <<  "SkySceneManager::ShadeClouds()\n";
  int i=0;
  
  for (CloudInstanceIterator cii = _cloudInstances.begin(); cii != _cloudInstances.end(); ++cii)
  {
    for (LightIterator li = _lights.begin(); li != _lights.end(); ++li)
    {
      
      if (SkyLight::SKY_LIGHT_DIRECTIONAL == li->second->GetType())
      {
        (*cii)->GetCloud()->Illuminate(li->second, *cii, li == _lights.begin());
//         printf("Shading Cloud %d of %d with light %d \n", i++, _cloudInstances.size(), *li );
      }
    }
  }
  _bReshadeClouds = false;
  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::LoadClouds
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::LoadClouds(SkyArchive& cloudArchive, float rScale)
 * @brief @todo <WRITE BRIEF SkySceneManager::LoadClouds DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::LoadClouds FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkySceneManager::LoadClouds(SkyArchive& cloudArchive, float rScale /* = 1.0f */)
{
  unsigned int iNumClouds = 0;
  cloudArchive.FindUInt32("CldNumClouds", &iNumClouds);
 
  SkyArchive subArchive;
	//iNumClouds = 5;  //set this value to reduce cloud field for debugging
  for (int i = 0; i < iNumClouds; ++i)
  {printf("Loading # %d of %d clouds\n", i+1, iNumClouds);
    cloudArchive.FindArchive("Cloud", &subArchive, i);
    SkyCloud *pCloud = new SkyCloud();
    pCloud->Load(subArchive, rScale);
    SkyRenderableInstanceCloud *pInstance = new SkyRenderableInstanceCloud(pCloud, false);
    AddCloud(pCloud);
    AddCloudInstance(pInstance);
  }
  RebuildCloudBVTree();
  return SKYRESULT_OK;
}

//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::_SortClouds
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::_SortClouds(CloudInstanceArray& clouds, const Vec3f& vecSortPoint)
 * @brief @todo <WRITE BRIEF SkySceneManager::_SortClouds DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::_SortClouds FUNCTION DOCUMENTATION>
 */ 
void SkySceneManager::_SortClouds(CloudInstanceArray& clouds, const Vec3f& vecSortPoint)
{
  static InstanceComparator comparator;

  for (CloudInstanceIterator cii = clouds.begin(); cii != clouds.end(); ++cii)
  {
    Vec3f vecPos = (*cii)->GetPosition();
    vecPos -= vecSortPoint;
    (*cii)->SetSquareSortDistance(vecPos.LengthSqr());
  }

  std::sort(clouds.begin(), clouds.end(), comparator);
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::_SortInstances
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::SortInstances(InstanceArray& instances, const Vec3f& vecSortPoint)
 * @brief @todo <WRITE BRIEF SkySceneManager::_SortInstances DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::_SortInstances FUNCTION DOCUMENTATION>
 */ 
void SkySceneManager::SortInstances(InstanceArray& instances, const Vec3f& vecSortPoint)
{
  static InstanceComparator comparator;

  for (InstanceIterator ii = instances.begin(); ii != instances.end(); ++ii)
  {
    Vec3f vecPos = (*ii)->GetPosition();
    vecPos -= vecSortPoint;
    (*ii)->SetSquareSortDistance(vecPos.LengthSqr());
  }
  
  std::sort(instances.begin(), instances.end(), comparator);
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::_ViewFrustumCullClouds
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::_ViewFrustumCullClouds(const Camera& cam, const CloudBVTree::Node *pNode)
 * @brief @todo <WRITE BRIEF SkySceneManager::_ViewFrustumCullClouds DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::_ViewFrustumCullClouds FUNCTION DOCUMENTATION>
 */ 
void SkySceneManager::_ViewFrustumCullClouds(const Camera& cam, const CloudBVTree::Node *pNode)
{ 
  if (!pNode)
    return;

  int i;
  int iResult = CamMinMaxBoxOverlap(&cam, pNode->GetNodeBV().GetMin(), pNode->GetNodeBV().GetMax());
  
 //iResult = COMPLETEIN; // just a hack to force the issue
  if (COMPLETEIN == iResult)
  {
    // trivially add all instances 
    for (i = 0; i < pNode->GetNumObjs(); ++i)
    {
      SkyRenderableInstanceCloud* pInstance = 
        const_cast<SkyRenderableInstanceCloud*>(pNode->GetObj(i));
      _visibleCloudInstances.push_back(pInstance);
    }
  }
  else if ((PARTIAL == iResult) && pNode->IsLeaf())
  {
    SkyMinMaxBox bbox;
    
    // check each instance in this node against camera
    for (i = 0; i < pNode->GetNumObjs(); ++i)
    {
      SkyRenderableInstanceCloud* pInstance = 
        const_cast<SkyRenderableInstanceCloud*>(pNode->GetObj(i));
      bbox = pInstance->GetWorldSpaceBounds();
      iResult = CamMinMaxBoxOverlap(&cam, bbox.GetMin(), bbox.GetMax());
      if (COMPLETEOUT != iResult)
        _visibleCloudInstances.push_back(pInstance);
      else
        pInstance->ReleaseImpostorTextures();
    }
  }
  else if (PARTIAL == iResult)
  {
    _ViewFrustumCullClouds(cam, pNode->GetLeftChild());
    _ViewFrustumCullClouds(cam, pNode->GetRightChild());
  }
  else // the node is completely out.  All of its child clouds should release their textures.
  {
    for (i = 0; i < pNode->GetNumObjs(); ++i)
    {
      SkyRenderableInstanceCloud* pInstance = 
        const_cast<SkyRenderableInstanceCloud*>(pNode->GetObj(i));
      pInstance->ReleaseImpostorTextures();
    }
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::_VisualizeCloudBVTree
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::_VisualizeCloudBVTree(const Camera& cam, const CloudBVTree::Node *pNode)
 * @brief @todo <WRITE BRIEF SkySceneManager::_VisualizeCloudBVTree DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::_VisualizeCloudBVTree FUNCTION DOCUMENTATION>
 */ 
void SkySceneManager::_VisualizeCloudBVTree(const Camera& cam, const CloudBVTree::Node *pNode)
{
  // set display state.
  _wireframeMaterial.Activate();

  int iResult = CamMinMaxBoxOverlap(&cam, pNode->GetNodeBV().GetMin(), pNode->GetNodeBV().GetMax());

  if (COMPLETEIN == iResult)
  {
    // draw this node's bounding box in green.
    glColor3f(0, 1, 0);
    pNode->GetNodeBV().Display();
  }
  else if (PARTIAL == iResult)
  {
    SkyMinMaxBox bbox;

    if (pNode->IsLeaf())
    {
      // draw this node's bounding box and the boxes of all of its objects that are visible.
      // draw this node's bbox in orange.
      glColor3f(1, 0.5, 0);
      pNode->GetNodeBV().Display();

      int i;
      for (i = 0; i < pNode->GetNumObjs(); ++i)
      {
        SkyRenderableInstanceCloud* pInstance = 
          const_cast<SkyRenderableInstanceCloud*>(pNode->GetObj(i));
        bbox = pInstance->GetWorldSpaceBounds();
        iResult = CamMinMaxBoxOverlap(&cam, bbox.GetMin(), bbox.GetMax());

        if (COMPLETEIN == iResult)
        {
          // draw the box in green
          glColor3f(0, 1, 0);
          bbox.Display();
        }
        else if (PARTIAL == iResult)
        {
          // draw the box in yellow
          glColor3f(1, 1, 0);
          bbox.Display();          
        }
      }
    }
    else
    {
      _VisualizeCloudBVTree(cam, pNode->GetLeftChild());
      _VisualizeCloudBVTree(cam, pNode->GetRightChild());
    }
  }
  else
  {
    // draw the node's bbox in red.  
    // This should NEVER be visible from the camera from which it was culled!
    glColor3f(1, 0, 0);
    pNode->GetNodeBV().Display();
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::_ResolveVisibility
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::_ResolveVisibility(const Camera &cam)
 * @brief @todo <WRITE BRIEF SkySceneManager::_ResolveRenderingOrder DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::_ResolveRenderingOrder FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkySceneManager::_ResolveVisibility(const Camera &cam)
{
  InstanceIterator ii;

  // clear the free instance array
  _visibleInstances.clear();

  // clear the contained instance arrays
  ContainerSetIterator csi;
  for (csi = _containerClouds.begin(); csi != _containerClouds.end(); ++csi)
  {
    csi->second->containedOpaqueInstances.clear();
    csi->second->containedTransparentInstances.clear();
  }

  // clear the visible cloud array.
  _visibleCloudInstances.clear();
  
  // Test each instance for containment inside a cloud's bounding volume.
  // If the instance is inside a cloud, it is considered a "contained" instance, and will be 
  // rendered with the cloud in which it is contained for correct visibility.  If the instance is
  // not inside any cloud, then it is a "free" instance, and will be rendered after all contained
  // instances.  Transparent instances of each type are rendered after opaque instances of each 
  // type.

  // opaque instances
  for (ii = _instances.begin(); ii != _instances.end(); ++ii)
  {
//     cout <<  "Opague instance\n"; char zz; cin >> zz;
    (*ii)->ViewFrustumCull(cam);  // First VFC then check if culled, some instances may
    // manually set the culled flag, instead of using VFC  
    if (!(*ii)->IsCulled())
    {
      // first update this instance.
      FAIL_RETURN_MSG((*ii)->Update(cam), "SkySceneManager::_ResolveVisibility(): instance update failed.");
      
      if (!_TestInsertInstanceIntoClouds(cam, _cloudBVTree.GetRoot(), *ii, false))
        _visibleInstances.push_back(*ii);
    }
  }

  // transparent instances
  for (ii = _transparentInstances.begin(); ii != _transparentInstances.end(); ++ii)
  {
//     cout << "Transparent instance\n"; char tt; cin >> tt;
    (*ii)->ViewFrustumCull(cam);  // First VFC then check if culled, some instances may
    // manually set the culled flag, instead of using VFC  
    if (!(*ii)->IsCulled())
    {
      // first update this instance.
      FAIL_RETURN_MSG((*ii)->Update(cam), "SkySceneManager::Update(): instance update failed.");
      
      if (!_TestInsertInstanceIntoClouds(cam, _cloudBVTree.GetRoot(), *ii, true))
        _visibleInstances.push_back(*ii);
    }
  }

  // view frustum cull the clouds
  _ViewFrustumCullClouds(cam, _cloudBVTree.GetRoot());  

  // Clouds must be rendered in sorted order.
  //_SortClouds(_visibleCloudInstances, cam.Orig);

  // reshade the clouds if necessary.
  if (_bReshadeClouds)
  {
  printf("ReShading clouds\n");
    FAIL_RETURN(ShadeClouds());
  }

  // Now process the visible clouds.  First, go through the container clouds corresponding to the
  // clouds, calculate their split points, and update their impostors.
  for (CloudInstanceIterator cii = _visibleCloudInstances.begin(); 
       cii != _visibleCloudInstances.end(); 
       ++cii)
  {
    // get the container corresponding to this cloud
    ContainerSetIterator csi = _containerClouds.find((*cii)->GetID());
   
    if (csi == _containerClouds.end())
    {
      SkyTrace("Error: SkySceneManager::_ResolveVisibility(): Invalid cloud instance %d.", 
               (*cii)->GetID());
      return SKYRESULT_FAIL;
    }
    
    if (csi->second->containedOpaqueInstances.size() > 0 ||
        csi->second->containedTransparentInstances.size() > 0)
    {
      SortInstances(csi->second->containedOpaqueInstances, cam.Orig);
      SortInstances(csi->second->containedTransparentInstances, cam.Orig);
    
    
      SkyRenderableInstance *pOpaque = (csi->second->containedOpaqueInstances.size() > 0) ? 
                                        csi->second->containedOpaqueInstances.back() : NULL;
      SkyRenderableInstance *pTransparent = (csi->second->containedTransparentInstances.size() > 0) ? 
                                             csi->second->containedTransparentInstances.back() : NULL;
    
      // find the closest contained instance to the camera
      if (pOpaque && pTransparent)
      {
        if (*pOpaque < *pTransparent)
          (*cii)->SetSplitPoint(pOpaque->GetPosition());
        else
          (*cii)->SetSplitPoint(pTransparent->GetPosition());
      }
      else if (pOpaque)
        (*cii)->SetSplitPoint(pOpaque->GetPosition());
      else if (pTransparent)
        (*cii)->SetSplitPoint(pTransparent->GetPosition());
      else
        (*cii)->SetSplit(false);
    }
    else
      (*cii)->SetSplit(false);
    
    // add the container to the list of visiblie clouds to be rendered this frame.
    _visibleInstances.push_back(csi->second);

    // now update the impostors
    FAIL_RETURN_MSG((*cii)->Update(cam),
      "SkySceneManager::_ResolveVisibility(): cloud instance update failed.");
  } 

  SortInstances(_visibleInstances, cam.Orig);

  return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneManager::_TestInsertInstanceIntoClouds
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneManager::_TestInsertInstanceIntoClouds(const Camera &cam, const CloudBVTree::Node *pNode, SkyRenderableInstance *pInstanceToInsert, bool bTransparent)
 * @brief @todo <WRITE BRIEF SkySceneManager::_TestInsertInstanceIntoClouds DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkySceneManager::_TestInsertInstanceIntoClouds FUNCTION DOCUMENTATION>
 */ 
bool SkySceneManager::_TestInsertInstanceIntoClouds(const Camera            &cam,
                                                    const CloudBVTree::Node *pNode, 
                                                    SkyRenderableInstance   *pInstanceToInsert,
                                                    bool                    bTransparent)
{
  if (_clouds.size() <= 0)
    return false;

  if (pNode->GetNodeBV().PointInBBox(pInstanceToInsert->GetPosition()))
  {
    if (pNode->IsLeaf())
    {
      SkyMinMaxBox bbox;
      int i;

      // check the instance against each cloud in this leaf node.
      for (i = 0; i < pNode->GetNumObjs(); ++i)
      {
        SkyRenderableInstanceCloud* pCloud = 
          const_cast<SkyRenderableInstanceCloud*>(pNode->GetObj(i));
        bbox = pCloud->GetWorldSpaceBounds();
        if (bbox.PointInBBox(pInstanceToInsert->GetPosition()))
        {
          // get the container cloud struct for this cloud instance, and add this instance.
          ContainerSetIterator csi = _containerClouds.find(pCloud->GetID());
          if (csi == _containerClouds.end())
          {
            SkyTrace(
              "Error: SkySceneManager::_TestInsertInstanceIntoClouds(): Invalid cloud instance %d.", 
              pCloud->GetID());
            return false;
          }
          else // this instance is inside a cloud.  Set up for split cloud rendering.
          {
            if (!bTransparent)
              csi->second->containedOpaqueInstances.push_back(pInstanceToInsert);
            else
              csi->second->containedTransparentInstances.push_back(pInstanceToInsert);
            csi->second->pCloud->SetSplit(true);
            return true;
          }
        }
      }
      return false;
    }
    else
    {
      if (!_TestInsertInstanceIntoClouds(cam, pNode->GetLeftChild(), pInstanceToInsert, bTransparent))
        return _TestInsertInstanceIntoClouds(cam, pNode->GetRightChild(), pInstanceToInsert, bTransparent);
      else
        return true;
    }
  }
  else
    return false;
}
