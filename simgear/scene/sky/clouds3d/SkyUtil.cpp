//------------------------------------------------------------------------------
// File : SkyUtil.cpp
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
 * @file SkyUtil.cpp
 * 
 * Implemtation of global utility functions.
 */
#include "SkyUtil.hpp"

//------------------------------------------------------------------------------
// Function     	  : SkyTrace
// Description	    : 
//------------------------------------------------------------------------------
/**
 * SkyTrace( char* strMsg, ... )
 * @brief Prints formatted output, debug only.
 *
 * Includes file and line number information automatically.
 */ 
void SkyTrace( char* strMsg, ... )
{
#if defined(DEBUG) | defined(_DEBUG)
  
  char strBuffer[512];

  va_list args;
  va_start(args, strMsg);
  _vsnprintf( strBuffer, 512, strMsg, args );
  va_end(args);

  fprintf(stderr, "[SkyTrace] %s(%d): %s\n",__FILE__, __LINE__, strBuffer);
#endif
}


