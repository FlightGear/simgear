//------------------------------------------------------------------------------
// File : SkyUtil.hpp
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
 * @file  SkyUtil.hpp
 * @brief Safe deallocation functions, result codes, trace functions, and macros.
 */
#ifndef __SKYUTIL_HPP__
#define __SKYUTIL_HPP__

#include <stdio.h>
#include <stdarg.h>
#include <math.h>

//-----------------------------------------------------------------------------
// Useful constants
//-----------------------------------------------------------------------------
//! Pi.
const float SKY_PI      = 4.0f * (float) atan(1.0f);
//! 1.0 / Pi
const float SKY_INV_PI  = 1.0f / SKY_PI;
//! 1.0 / (4.0 * Pi)
const float SKY_INV_4PI = 1.0f / (4.0f * SKY_PI);

//-----------------------------------------------------------------------------
// Safe deallocation
//-----------------------------------------------------------------------------
//! Delete and set pointer to NULL.
#define SAFE_DELETE(p)       { delete   (p); (p)=NULL; }
//! Delete an array and set pointer to NULL.
#define SAFE_DELETE_ARRAY(p) { delete[] (p); (p)=NULL; }
//#define SAFE_RELEASE(p)      { (p) = NULL; } 
//{ if(p) {  (p)->Release(); (p)=NULL; } }

//------------------------------------------------------------------------------
// Useful Macros
//------------------------------------------------------------------------------
//! Convert Degrees to Radians
#define SKYDEGREESTORADS 0.01745329252f
//! Convert Radians to Degrees
#define SKYRADSTODEGREES 57.2957795131f

//------------------------------------------------------------------------------
// Function     	  : SkyGetLogBaseTwo
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyGetLogBaseTwo(int iNum)
 * @brief Returns the integer base two logarithm of the integer input.
 */ 
inline int SkyGetLogBaseTwo(int iNum)
{
  int i, n;
  for(i = iNum-1, n = 0; i > 0; i >>= 1, n++ );
  return n;
}


//------------------------------------------------------------------------------
// Function     	  : SkyTrace
// Description	    : 
//------------------------------------------------------------------------------
void SkyTrace( char* strMsg, ... );


//.----------------------------------------------------------------------------.
//|   Result Codes                                                             |
//.----------------------------------------------------------------------------.
//! SKYRESULTs are used for returning error information that can be used to trace bugs.
typedef int SKYRESULT;

//! Returns true if the SKYRESULT is a success result.
#define SKYSUCCEEDED(Status) ((SKYRESULT)(Status) >= 0)
//! Returns true if the SKYRESULT is a failure result.
#define SKYFAILED(Status) ((SKYRESULT)(Status) < 0)

//! SKYRESULTs are used for returning error information that can be used to trace bugs.
enum SKYRESULT_CODES
{
    // SUCCESS CODES: non-negative
    SKYRESULT_OK = 1,
    // FAILURE CODES: negative
    SKYRESULT_FAIL = -1
};


//-----------------------------------------------------------------------------
// FAIL_RETURN
//-----------------------------------------------------------------------------
// Print debug messages to the WIN32 debug window 
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Function     	  : FAIL_RETURN
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn FAIL_RETURN(p)
 * @brief Prints a trace message if @a p failed, and returns the failure code.
 *
 * Outputs in a format that can be double-clicked in DevStudio to open the 
 * appropriate file and location.
 */ 
#if defined(DEBUG) | defined(_DEBUG)
	#define FAIL_RETURN(p) \
	{ \
		SKYRESULT __SKYUTIL__result__; \
			if ( SKYFAILED( __SKYUTIL__result__ = (p) ) ) { \
				fprintf(stderr, "!!!! FAIL_RETURN TRAP !!!! %s: %d: %d\n",__FILE__, __LINE__, __SKYUTIL__result__); \
				return __SKYUTIL__result__; \
			} \
	}
#else
	#define FAIL_RETURN(p) p
#endif


//------------------------------------------------------------------------------
// Function     	  : FAIL_RETURN_MSG
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn FAIL_RETURN_MSG(p,str)
 * @brief Similar to FAIL_RETURN, but also appends a user-supplied message.
 * 
 * @see FAIL_RETURN, FAIL_RETURN_MSG
 */ 
#if defined(DEBUG) | defined(_DEBUG)
	#define FAIL_RETURN_MSG(p,str) \
	{ \
		SKYRESULT __SKYUTIL__result__; \
			if ( SKYFAILED( __SKYUTIL__result__ = (p) ) ) { \
				fprintf(stderr, "!!!! FAIL_RETURN_MSG TRAP !!!! %s: %d: %d: %s\n",__FILE__,__LINE__,__SKYUTIL__result__,str); \
				return __SKYUTIL__result__; \
			} \
	}
#else
	#define FAIL_RETURN_MSG(p,str) p
#endif


//------------------------------------------------------------------------------
// Function     	  : FAIL_RETURN_MSGBOX
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn FAIL_RETURN_MSGBOX(p,str)
 * @brief Similar to FAIL_RETURN_MSG, but also displays the error in a message box (in Windows).
 * 
 * @see FAIL_RETURN_MSG, FAIL_RETURN
 */ 
#if defined(DEBUG) | defined(_DEBUG)
#ifdef USEWINDOWSOUTPUT
	#define FAIL_RETURN_MSGBOX(p,str) \
	{ \
		SKYRESULT __SKYUTIL__result__; \
			if ( SKYFAILED( __SKYUTIL__result__ = (p) ) ) { \
                char msg[512]; \
                sprintf(msg, "%s: %d: %d: %s\n",__FILE__,__LINE__,__SKYUTIL__result__,str); \
                MessageBox(NULL, msg, "!!!! FAIL_RETURN_MSG TRAP !!!!", MB_OK); \
				return __SKYUTIL__result__; \
			} \
	}
#else
    #define FAIL_RETURN_MSGBOX(p,str) \
	{ \
		SKYRESULT __SKYUTIL__result__; \
			if ( SKYFAILED( __SKYUTIL__result__ = (p) ) ) { \
                fprintf(stderr, "!!!! FAIL_RETURN_MSG TRAP !!!! %s: %d: %d: %s\n",__FILE__,__LINE__,__D3DUTIL__hres__,str); \
				return __SKYUTIL__result__; \
			} \
	}
#endif
#else
	#define FAIL_RETURN_MSGBOX(p,str) p
#endif

#endif //__SKYUTIL_HPP__
