//------------------------------------------------------------------------------
// File : SkySceneLoader.hpp
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
 * @file SkySceneLoader.hpp
 * 
 * Definition of a simple class for loading scenes into SkyWorks.
 */
#ifndef __SKYSCENELOADER_HPP__
#define __SKYSCENELOADER_HPP__

#ifdef HAVE_CONFIG
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/misc/sg_path.hxx>
#include <simgear/math/point3d.hxx>

#include "SkyUtil.hpp"


//------------------------------------------------------------------------------
/**
 * @class SkySceneLoader
 * @brief A simple scene loader for SkyWorks scenes.
 * 
 * Loads a scene using the Load() method, which is passed the filename of a 
 * file containing a SkyArchive describing the scene. 
 */
class SkySceneLoader
{
public:
  SkySceneLoader();
  ~SkySceneLoader();

  bool Load( unsigned char *data, unsigned int size, double latitude, double longitude );
  bool Load( SGPath fileroot, double latitude, double longitude );
  
  void Set_Cloud_Orig( Point3D *posit );
  
  void Update( double *view_pos );
  
  void Resize( double w, double h);
  
  void Draw( sgMat4 mat );
  
};

#endif //__SKYSCENELOADER_HPP__
