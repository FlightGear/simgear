//------------------------------------------------------------------------------
// File : SkySceneLoader.cpp
//------------------------------------------------------------------------------
// Adapted from SkyWorks for FlightGear by J. Wojnaroski -- castle@mminternet.com
// Copywrite July 2002
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
/**
 * @file SkySceneLoader.cpp
 * 
 * Implementation of class SkySceneLoader.
 */

#include <plib/ssg.h>
#include <simgear/math/point3d.hxx>

#include "SkySceneLoader.hpp"
#include "SkySceneManager.hpp"
#include "SkyTextureManager.hpp"
#include "SkySceneManager.hpp"
#include "SkyDynamicTextureManager.hpp"
#include "SkyContext.hpp"
//#include "SkyViewManager.hpp"
//#include "SkyRenderableInstanceGroup.hpp" 
#include "SkyLight.hpp"
#include "camera.hpp"

ssgLight _sky_ssgLights [ 8 ] ;
static Point3D origin;
Point3D offset;
//int      _ssgFrameCounter = 0 ;
Camera *pCam = new Camera();
// Need to add a light here until we figure out how to use the sun position and color
SkyLight::SkyLightType eType = SkyLight::SKY_LIGHT_DIRECTIONAL;
SkyLight *pLight = new SkyLight(eType);

//------------------------------------------------------------------------------
// Function     	  : SkySceneLoader::SkySceneLoader
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneLoader::SkySceneLoader()
 * @brief Constructor.
 */ 
SkySceneLoader::SkySceneLoader()
{

}

//------------------------------------------------------------------------------
// Function     	  : SkySceneLoader::~SkySceneLoader
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneLoader::~SkySceneLoader()
 * @brief Destructor.
 */ 
SkySceneLoader::~SkySceneLoader()
{
	SceneManager::Destroy();
	DynamicTextureManager::Destroy();
	TextureManager::Destroy();
	GraphicsContext::Destroy();
}


//------------------------------------------------------------------------------
// Function     	  : SkySceneLoader::Load
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkySceneLoader::Load(std::string filename)
 * @brief Loads a SkyWorks scene.
 * 
 * This is a temporary fix, as it loads only limited scenes
 *  It can however, load any number of Cloud
 +
 */ 
bool SkySceneLoader::Load(std::string filename)
{ 
  SkyArchive archive;
  if (SKYFAILED(archive.Load(filename.c_str()))) {
  	cout << "Archive file not found\n";
    return false; }
  char *pFilename;
  
  // Need to create the managers
  GraphicsContext::Instantiate();
  TextureManager::Instantiate();
  DynamicTextureManager::Instantiate();
  SceneManager::Instantiate();

  unsigned int iNumFiles;
  if (!SKYFAILED(archive.GetInfo("CloudFile", STRING_TYPE, &iNumFiles)))
  {
    for (unsigned int i = 0; i < iNumFiles; ++i)
    {
      FAIL_RETURN(archive.FindString("CloudFile", &pFilename, i));  
      float rScale = 1.0;
      FAIL_RETURN(archive.FindFloat32("CloudScale", &rScale, i));
      rScale = 5.0;
      SkyArchive cloudArchive;
      FAIL_RETURN(cloudArchive.Load(pFilename));
      FAIL_RETURN(SceneManager::InstancePtr()->LoadClouds(cloudArchive, rScale)); 
    }
  }
  
  Vec3f dir(0, 0, 1);
  pLight->SetPosition(Vec3f(0, 0, 7000));
  pLight->SetDirection(dir);
  pLight->SetAmbient(Vec4f( 0.0f, 0.0f, 0.0f, 0.0f));
  pLight->SetDiffuse(Vec4f(1.0f, 1.0f, 1.0f, 0.0f));
  //pLight->SetDiffuse(Vec4f(0.0f, 0.0f, 0.0f, 0.0f));
  //pLight->SetSpecular(Vec4f(1.0f, 1.0f, 1.0f, 0.0f));
  
   // No attenuation
  pLight->SetAttenuation(1.0f, 0.0f, 0.0f);
  SceneManager::InstancePtr()->AddLight(pLight);
  
  SceneManager::InstancePtr()->ShadeClouds();
 
  return true;
}

void SkySceneLoader::Set_Cloud_Orig( Point3D *posit )
{ // use this to adjust camera position for a new tile center

	origin = *posit; // set origin to current tile center
	printf("Cloud marker %f %f %f\n", origin.x(), origin.y(), origin.z() );
	
}

void SkySceneLoader::Update( sgMat4  viewmat, Point3D *posit )
//void SkySceneLoader::Update()
{
	offset = *posit - origin;
	cout << "X: " << offset.x() << "Y: " <<  offset.y() << "Z: " << offset.z() << endl;
	
	SceneManager::InstancePtr()->Update(*pCam);
	
	// need some scheme to reshade selected clouds a few at a time to save frame rate cycles
	///SceneManager::InstancePtr()->ShadeClouds();
	
}

void SkySceneLoader::Resize(  double w, double h )
{
	
  pCam->Perspective( (float) h, (float) (w / h), 0.5, 120000.0 );
 
}

void SkySceneLoader::Draw()
{ // this is a clone of the plib ssgCullAndDraw except there is no scene graph
	if ( _ssgCurrentContext == NULL )
  {
    cout<< "ssg: No Current Context: Did you forgot to call ssgInit()?" ; char x; cin >> x;
  }
  
  //ssgForceBasicState () ;
  
  sgMat4 test;

  //glMatrixMode ( GL_PROJECTION );
  //glLoadIdentity();
  //_ssgCurrentContext->loadProjectionMatrix ();
  // test/debug section
 
  //_ssgCurrentContext->getProjectionMatrix( test );
  /*
  printf( "\nFG Projection matrix\n" );
  cout << test[0][0] << " " << test[1][0] << " " << test[2][0] << " " << test[3][0] << endl;
  cout << test[0][1] << " " << test[1][1] << " " << test[2][1] << " " << test[3][1] << endl;
  cout << test[0][2] << " " << test[1][2] << " " << test[2][2] << " " << test[3][2] << endl;
  cout << test[0][3] << " " << test[1][3] << " " << test[2][3] << " " << test[3][3] << endl;
  */
	sgMat4 m, *pm;
	sgVec3 temp;
	pm = &m;
	
	 // this is the cameraview matrix used by flightgear to render scene
  // need to play with this to build a new matrix that accounts for tile crossings
  // for now it resets the clouds when a boundary is crossed
	 _ssgCurrentContext->getModelviewMatrix( m );
	
	///pCam->GetProjectionMatrix( (float *) pm );
	//sgCopyMat4( test, m );
	/*printf( "\nSkyworks Projection matrix\n" );
	cout << test[0][0] << " " << test[1][0] << " " << test[2][0] << " " << test[3][0] << endl;
  cout << test[0][1] << " " << test[1][1] << " " << test[2][1] << " " << test[3][1] << endl;
  cout << test[0][2] << " " << test[1][2] << " " << test[2][2] << " " << test[3][2] << endl;
  cout << test[0][3] << " " << test[1][3] << " " << test[2][3] << " " << test[3][3] << endl;
   */  
  glMatrixMode ( GL_MODELVIEW ) ;
  glLoadIdentity () ;
  glLoadMatrixf( (float *) pm );
  
  //sgCopyMat4( test, m );

	pCam->SetModelviewMatrix( (float *) pm );
 
 //printf( "\nFG modelview matrix\n" );
  //cout << test[0][0] << " " << test[1][0] << " " << test[2][0] << " " << test[3][0] << endl;
  //cout << test[0][1] << " " << test[1][1] << " " << test[2][1] << " " << test[3][1] << endl;
  //cout << test[0][2] << " " << test[1][2] << " " << test[2][2] << " " << test[3][2] << endl;
  //cout << test[0][3] << " " << test[1][3] << " " << test[2][3] << " " << test[3][3] << endl;
	
	//pCam->Print();
	
  //_ssgCurrentContext->cull(r) ;
  //_ssgDrawDList () ;

  SceneManager::InstancePtr()->Display(*pCam);
	
  //pLight->Display(); // draw the light position to  debug with sun position

  glMatrixMode ( GL_MODELVIEW ) ;
  glLoadIdentity () ;

}
