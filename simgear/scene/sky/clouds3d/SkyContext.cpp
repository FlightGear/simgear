//------------------------------------------------------------------------------
// File : SkyContext.cpp
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
 * @file SkyContext.cpp
 * 
 * Graphics Context Interface.  Initializes GL extensions, etc.
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#endif

// #include GLUT_H

#ifndef WIN32
#include <GL/glx.h>
#endif

//#include "extgl.h"

#include "SkyContext.hpp"
#include "SkyUtil.hpp"
#include "SkyMaterial.hpp"
#include "SkyTextureState.hpp"

//------------------------------------------------------------------------------
// Function     	  : SkyContext::SkyContext
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyContext::SkyContext()
 * @brief Constructor.
 *
 */ 
SkyContext::SkyContext()
{
  // _iWidth  = glutGet(GLUT_WINDOW_WIDTH);
  // _iHeight = glutGet(GLUT_WINDOW_HEIGHT);
  _iWidth  = 640;
  _iHeight = 480;
  
  // materials and structure classes
  AddCurrentGLContext();
  // Initialize all the extensions and load the functions - JW (file is extgl.c)
  #ifdef WIN32
  glInitialize();
  InitializeExtension("GL_ARB_multitexture");
  #endif
}


//------------------------------------------------------------------------------
// Function     	  : SkyContext::~SkyContext
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyContext::~SkyContext()
 * @brief Destructor.
 */ 
SkyContext::~SkyContext()
{
  // delete map of materials
  for (ContextMaterialIterator cmi = _currentMaterials.begin(); cmi != _currentMaterials.end(); ++cmi)
  {
    SAFE_DELETE(cmi->second);
  }
  _currentMaterials.clear();
}


//------------------------------------------------------------------------------
// Function     	  : SkyContext::ProcessReshapeEvent
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyContext::ProcessReshapeEvent(int iWidth, int iHeight)
 * @brief Handles window resize events, and notifies all context listeners of the event.
 */ 
SKYRESULT SkyContext::ProcessReshapeEvent(int iWidth, int iHeight)
{ 
  _iWidth = iWidth; 
  _iHeight = iHeight; 
  return _SendMessage(SKYCONTEXT_MESSAGE_RESHAPE);
}


//------------------------------------------------------------------------------
// Function     	  : SkyContext::InitializeExtensions
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyContext::InitializeExtensions(const char *pExtensionNames)
* @brief Initializes GL extension specified by @a pExtensionNames.      
*/ 
SKYRESULT SkyContext::InitializeExtension(const char *pExtensionName)
{ /***
  if (!QueryExtension(pExtensionName)) // see query search function defined in extgl.c
  {
    SkyTrace(
      "ERROR: SkyContext::InitializeExtenstions: The following extensions are unsupported: %s\n", 
      pExtensionName);
    return SKYRESULT_FAIL;
  }  **/
  //set this false to catch all the extensions until we come up with a linux version
  return SKYRESULT_FAIL;
}


//------------------------------------------------------------------------------
// Function     	  : SkyContext::GetCurrentMaterial
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn SkyContext::GetCurrentMaterial()
* @brief Returns the current cached material state that is active in this OpenGL context.
* 
* @todo <WRITE EXTENDED SkyContext::GetCurrentMaterial FUNCTION DOCUMENTATION>
*/ 
SkyMaterial* SkyContext::GetCurrentMaterial()
{
#ifdef WIN32
    ContextMaterialIterator cmi = _currentMaterials.find(wglGetCurrentContext());
#else
    ContextMaterialIterator cmi = _currentMaterials.find(glXGetCurrentContext());
#endif
  if (_currentMaterials.end() != cmi)
    return cmi->second;
  else
  {
    SkyTrace("SkyContext::GetCurrentMaterial(): Invalid context.");
    return NULL;
  }
  
}


//------------------------------------------------------------------------------
// Function     	  : SkyContext::GetCurrentTextureState
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyContext::GetCurrentTextureState()
 * @brief Returns the current cached texture state that is active in this OpenGL context.
 * 
 * @todo <WRITE EXTENDED SkyContext::GetCurrentTextureState FUNCTION DOCUMENTATION>
 */ 
SkyTextureState* SkyContext::GetCurrentTextureState()
{
#ifdef WIN32
    ContextTextureStateIterator ctsi = _currentTextureState.find(wglGetCurrentContext());
#else
    ContextTextureStateIterator ctsi = _currentTextureState.find(glXGetCurrentContext());
#endif
  if (_currentTextureState.end() != ctsi)
    return ctsi->second;
  else
  {
    SkyTrace("SkyContext::GetCurrentTextureState(): Invalid context.");
    return NULL;
  }
}


//------------------------------------------------------------------------------
// Function     	  : SkyContext::AddCurrentGLContext
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyContext::AddCurrentGLContext()
 * @brief @todo <WRITE BRIEF SkyContext::AddCurrentGLContext DOCUMENTATION>
 * 
 * @todo <WRITE EXTENDED SkyContext::AddCurrentGLContext FUNCTION DOCUMENTATION>
 */ 
SKYRESULT SkyContext::AddCurrentGLContext()
{
    SkyMaterial *pCurrentMaterial = new SkyMaterial;
#ifdef WIN32
    _currentMaterials.insert(std::make_pair(wglGetCurrentContext(), pCurrentMaterial));
#else
    _currentMaterials.insert(std::make_pair(glXGetCurrentContext(), pCurrentMaterial));
#endif

    SkyTextureState *pCurrentTS = new SkyTextureState;
#ifdef WIN32
    _currentTextureState.insert(std::make_pair(wglGetCurrentContext() , pCurrentTS));
#else
    _currentTextureState.insert(std::make_pair(glXGetCurrentContext() , pCurrentTS));
#endif
    return SKYRESULT_OK;
}

//------------------------------------------------------------------------------
// Function     	  : SkyContext::Register
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyContext::Register(Listener* pListener, int priority)
 * @brief Register with the messaging system to handle notification of mode changes
 */ 
SKYRESULT SkyContext::Register(Listener* pListener, int priority)
{
	std::list<ListenerPair>::iterator iter = 
    std::find_if(_listeners.begin(), _listeners.end(), _ListenerPred(pListener));
	if (iter == _listeners.end())
	{
		// insert the listener, sorted by priority
		for (iter=_listeners.begin(); iter != _listeners.end(); ++iter)
		{
			if (priority <= iter->first)
			{
				_listeners.insert(iter, ListenerPair(priority, pListener));
				break;
			}
		}
		if (iter == _listeners.end())
		{
				_listeners.push_back(ListenerPair(priority, pListener));
		}
		// Send a message to the pListener if we are already active so it
		// can intialize itself
		//FAIL_RETURN(pListener->GraphicsReshapeEvent());
	} 
  else
	{
		FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyContext: Listener is already registered");
	}
	return SKYRESULT_OK;
}


//------------------------------------------------------------------------------
// Function     	  : SkyContext::UnRegister
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn SkyContext::UnRegister(Listener *pListener)
 * @brief UnRegister with the messaging system.
 */ 
SKYRESULT SkyContext::UnRegister(Listener *pListener)
{
	std::list<ListenerPair>::iterator iter = 
    std::find_if(_listeners.begin(), _listeners.end(), _ListenerPred(pListener));
	if (iter == _listeners.end())
	{
		FAIL_RETURN_MSG(SKYRESULT_FAIL, "SkyContext: Listener is not registered");
	} 
  else
	{
		_listeners.erase(iter);
	}
	return SKYRESULT_OK;
}


/**
 * @fn SkyContext::_SendMessage(SkyMessageType msg)
 * @brief Messaging system to handle notification of mode changes
 */ 
SKYRESULT SkyContext::_SendMessage(SkyMessageType msg)
{
	if (_listeners.size() == 0) return SKYRESULT_OK;

	bool failure = false;
	SKYRESULT res, failureCode = SKYRESULT_OK;
	std::list<ListenerPair>::iterator iter;
	SKYRESULT (Listener::*fnPtr)() = NULL;

	// Make a pointer to the appropriate method
	switch (msg)
	{
	case SKYCONTEXT_MESSAGE_RESHAPE: fnPtr = &Listener::GraphicsReshapeEvent; break;
	}

	// Notify all listeners of the messag. catch failures, but still call everyone else.
	// !!! WRH HORRIBLE HACK must cache the current "end" because these functions could register new listeners
	std::list<ListenerPair>::iterator endIter = _listeners.end();
	endIter--;

	iter = _listeners.begin();
	do
	{
		if ( SKYFAILED( res = (iter->second->*fnPtr)() ) )
		{
			failureCode = res;
			SkyTrace("SkyContext: SendMessage failed");
		}
		if (iter == endIter) break;
		iter++;
	} while (true);

	FAIL_RETURN(failureCode);

	return SKYRESULT_OK;
}
