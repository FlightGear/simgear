//------------------------------------------------------------------------------
// File : SkyContext.hpp
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
 * @file SkyContext.hpp
 * 
 * Graphics Context Interface.  Initializes GL extensions, etc. 
 */
#ifndef __SKYCONTEXT_HPP__
#define __SKYCONTEXT_HPP__

// warning for truncation of template name for browse info
#pragma warning( disable : 4786)

#include "SkySingleton.hpp"


#ifdef _WIN32
#include "extgl.h"
#endif

#include <list>
#include <map>
#include <algorithm>

//  ifdef to replace windows stuff for handles-JW
typedef void *HANDLE;
typedef HANDLE *PHANDLE;
#define DECLARE_HANDLE(n)  typedef HANDLE n

DECLARE_HANDLE(HGLRC);
// end of ifdef

class SkyContext;
class SkyMaterial;
class SkyTextureState;

//! Graphics Context Singleton declaration.
/*! The Context must be created by calling GraphicsContext::Instantiate(). */
typedef SkySingleton<SkyContext> GraphicsContext;

//------------------------------------------------------------------------------
/**
 * @class SkyContext
 * @brief A manager / proxy for the state of OpenGL contexts.
 * 
 * @todo <WRITE EXTENDED CLASS DESCRIPTION>
 */
class SkyContext
{
public: // datatypes

  //------------------------------------------------------------------------------
  /**
   * @class Listener
   * @brief Inherit this class and overide its methods to be notified of context events.
   */
  class Listener
	{
	public:

    //! Handle a change in the dimensions of the graphics window.
		virtual SKYRESULT GraphicsReshapeEvent()      { return SKYRESULT_OK; }
	};

  /**
   * @enum SkyMessageType messages that the context can generate for it's listeners.
   */
	enum SkyMessageType
	{
		SKYCONTEXT_MESSAGE_RESHAPE,
	  SKYCONTEXT_MESSAGE_COUNT
	};

public: // methods

  SKYRESULT        ProcessReshapeEvent(int iWidth, int iHeight);
  SKYRESULT        InitializeExtension(const char *pExtensionName);

  //! Returns the current dimensions of the window.
  void             GetWindowSize(int &iWidth, int &iHeight) { iWidth = _iWidth; iHeight = _iHeight; }

  SkyMaterial*     GetCurrentMaterial();
  SkyTextureState* GetCurrentTextureState();

  SKYRESULT        AddCurrentGLContext();

  //------------------------------------------------------------------------------
	// Register with the messaging system to handle notification of mode changes
	//------------------------------------------------------------------------------
	SKYRESULT        Register(Listener   *pListener, int priority = 0);
	SKYRESULT        UnRegister(Listener *pLlistener);

protected: // methods
	SkyContext();
	~SkyContext();

protected: // data
	int _iWidth;
  int _iHeight;

  typedef std::map<HGLRC, SkyMaterial*>     ContextMaterialMap;
  typedef ContextMaterialMap::iterator      ContextMaterialIterator;
  typedef std::map<HGLRC, SkyTextureState*> ContextTextureStateMap;
  typedef ContextTextureStateMap::iterator  ContextTextureStateIterator;

  ContextMaterialMap      _currentMaterials;
  ContextTextureStateMap  _currentTextureState;

  //------------------------------------------------------------------------------
	// Messaging system to handle notification of mode changes
	//------------------------------------------------------------------------------
	typedef std::pair<int, Listener*> ListenerPair;
	class _ListenerPred
	{
	public:
		_ListenerPred(const Listener* l) { _l = l; }
		bool operator()(const ListenerPair& pair) { return pair.second == _l; }
	protected:
		const Listener *_l;
	};

	SKYRESULT _SendMessage(SkyMessageType msg);
	std::list<ListenerPair> _listeners;

};

#endif //__SKYCONTEXT_HPP__
