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
#include <plib/sg.h>

#include <simgear/misc/sg_path.hxx>
#include <simgear/math/point3d.hxx>
#include <simgear/math/polar3d.hxx>
#include <simgear/math/vector.hxx>

#include "SkySceneLoader.hpp"
#include "SkySceneManager.hpp"
#include "SkyTextureManager.hpp"
#include "SkySceneManager.hpp"
#include "SkyDynamicTextureManager.hpp"
#include "SkyContext.hpp"
#include "SkyLight.hpp"
#include "camera.hpp"

ssgLight _sky_ssgLights [ 8 ] ;

sgdVec3 cam_pos;
static sgdVec3 delta;
Point3D origin;

Camera *pCam = new Camera();
// Need to add a light here until we figure out how to use the sun position and color
SkyLight::SkyLightType eType = SkyLight::SKY_LIGHT_DIRECTIONAL;
SkyLight *pLight = new SkyLight(eType);

// hack
sgMat4 my_copy_of_ssgOpenGLAxisSwapMatrix =
{
  {  1.0f,  0.0f,  0.0f,  0.0f },
  {  0.0f,  0.0f, -1.0f,  0.0f },
  {  0.0f,  1.0f,  0.0f,  0.0f },
  {  0.0f,  0.0f,  0.0f,  1.0f }
};
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
//bool SkySceneLoader::Load(std::string filepath)
bool SkySceneLoader::Load( SGPath filename, double latitude, double longitude )
{ 
  SkyArchive archive;
 
  SGPath base = filename.dir();
 
  if (SKYFAILED(archive.Load(filename.c_str()))) {
      cout << "Archive file not found\n" <<  filename.c_str();
      return false;
  }
    
  char *pFilename;
  
  // Need to create the managers
  cout << "GraphicsContext::Instantiate();" << endl;
  GraphicsContext::Instantiate();
  cout << "  TextureManager::Instantiate();" << endl;
  TextureManager::Instantiate();
  cout << "  DynamicTextureManager::Instantiate();" << endl;
  DynamicTextureManager::Instantiate();
  cout << "  SceneManager::Instantiate();" << endl;
  SceneManager::Instantiate();

  unsigned int iNumFiles;
  if (!SKYFAILED(archive.GetInfo("CloudFile", STRING_TYPE, &iNumFiles)))
  {
    for (unsigned int i = 0; i < iNumFiles; ++i)
    {
      FAIL_RETURN(archive.FindString("CloudFile", &pFilename, i));
      //  this is where we have to append the $fg_root string to the filename
      base.append( pFilename );
      const char *FilePath = base.c_str();
     
      //float rScale = 1.0;
      //FAIL_RETURN(archive.FindFloat32("CloudScale", &rScale, i));
      float rScale = 40.0;
      SkyArchive cloudArchive;
      FAIL_RETURN(cloudArchive.Load(FilePath));
      FAIL_RETURN(SceneManager::InstancePtr()->LoadClouds(cloudArchive, rScale, latitude, longitude)); 
    }
  }
  
  Vec3f dir(0, 0, 1);
  pLight->SetPosition(Vec3f(0, 0, 17000));
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
{ 
	// set origin for cloud coordinates to initial tile center
	origin = *posit;
	sgdSetVec3( delta, origin[0], origin[1], origin[2]);	
	//printf("Cloud marker %f %f %f\n", origin[0], origin[1], origin[2] );
	
}

void SkySceneLoader::Update( double *view_pos )
{
	
	double wind_x, wind_y, wind_z;
	wind_x = 0.0; wind_z =  0.0;
	// just a dumb test to see what happens if we can move the clouds en masse via the camera
	delta[0] += wind_x; delta[2] += wind_z;
	
	sgdSubVec3( cam_pos, view_pos, delta );
	//cout << "ORIGIN: " << delta[0] << " " << delta[1] << " " << delta[2] << endl;
	//cout << "CAM   : " << cam_pos[0] << " " << cam_pos[1] << " "  << cam_pos[2] << endl;
	
	SceneManager::InstancePtr()->Update(*pCam);
	
	// need some scheme to reshade selected clouds a few at a time to save frame rate cycles
	// SceneManager::InstancePtr()->ShadeClouds();
	
}

void SkySceneLoader::Resize(  double w, double h )
{
	
  pCam->Perspective( (float) h, (float) (w / h), 0.5, 120000.0 );
 
}

void SkySceneLoader::Draw( sgMat4 mat )
{/* need this if you want to look at FG matrix
	if ( _ssgCurrentContext == NULL )
  {
    cout<< "ssg: No Current Context: Did you forgot to call ssgInit()?" ; char x; cin >> x;
  }
  
  ssgForceBasicState () ;
  */
  sgMat4 test, m, *pm, viewmat,  cameraMatrix;
  pm = &m;
  sgSetVec4(mat[3], cam_pos[0], cam_pos[1], cam_pos[2], SG_ONE);
  // at this point the view matrix has the cloud camera position relative to cloud origin
  // now transform to screen coordinates
  sgTransposeNegateMat4 ( viewmat, mat ) ;

  sgCopyMat4    ( cameraMatrix, my_copy_of_ssgOpenGLAxisSwapMatrix ) ;
  sgPreMultMat4 ( cameraMatrix, viewmat ) ;
  
  //sgCopyMat4 ( test, cameraMatrix );
  
  //printf( "\nSkyworks ViewModel matrix\n" );
	//cout << test[0][0] << " " << test[1][0] << " " << test[2][0] << " " << test[3][0] << endl;
  //cout << test[0][1] << " " << test[1][1] << " " << test[2][1] << " " << test[3][1] << endl;
  //cout << test[0][2] << " " << test[1][2] << " " << test[2][2] << " " << test[3][2] << endl;
  //cout << test[0][3] << " " << test[1][3] << " " << test[2][3] << " " << test[3][3] << endl;
	
	 // this is the cameraview matrix used by flightgear to render scene
	//_ssgCurrentContext->getModelviewMatrix( m );
	
  glMatrixMode ( GL_MODELVIEW ) ;
  glLoadIdentity () ;
  glLoadMatrixf( (float *) cameraMatrix );
  
  //sgCopyMat4( test, m );

	pCam->SetModelviewMatrix( (float *) cameraMatrix );
  
  //printf( "\nFG modelview matrix\n" );
  //cout << test[0][0] << " " << test[1][0] << " " << test[2][0] << " " << test[3][0] << endl;
  //cout << test[0][1] << " " << test[1][1] << " " << test[2][1] << " " << test[3][1] << endl;
  //cout << test[0][2] << " " << test[1][2] << " " << test[2][2] << " " << test[3][2] << endl;
  //cout << test[0][3] << " " << test[1][3] << " " << test[2][3] << " " << test[3][3] << endl;

	SceneManager::InstancePtr()->Display(*pCam);
	
	//pLight->Display(); // draw the light position to  debug with sun position

  glMatrixMode ( GL_MODELVIEW ) ;
  glLoadIdentity () ;

}
