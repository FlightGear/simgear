// File : RenderTexture.cpp
//---------------------------------------------------------------------------
// Copyright (c) 2002-2004 Mark J. Harris
//---------------------------------------------------------------------------
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any
// damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any
// purpose, including commercial applications, and to alter it and
// redistribute it freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you
//    must not claim that you wrote the original software. If you use
//    this software in a product, an acknowledgment in the product
//    documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and
//    must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
// --------------------------------------------------------------------------
// Credits:
// Original RenderTexture class: Mark J. Harris
// Original Render to Depth Texture support: Thorsten Scheuermann
// Linux Copy-to-texture: Eric Werness
// OS X: Alexander Powell (someone please help)
// Various Bug Fixes: Daniel (Redge) Sperl 
//                    Bill Baxter
// Ubuntu 8.04 64bit: Geoff McLane - 2009-07-01
// to work even when version 1.2 is returned.
//
// --------------------------------------------------------------------------
/**
* @file RenderTexture.cpp
* 
* Implementation of class RenderTexture.  A multi-format render to 
* texture wrapper.
*/
#ifdef _MSC_VER
#pragma warning(disable:4786)
#endif

/*
 * Changelog:
 *
 * Jan. 2005, Removed GLEW dependencies, Erik Hofman, Fred Bouvier
 * Nov. 2005, Use the simgear logging facility, Erik Hofman
 * Mar. 2006, Add MAC OS X support, Alexander Powell
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/debug/logstream.hxx>
#include <simgear/screen/extensions.hxx>
#include <simgear/screen/RenderTexture.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

#include <cstring>

#ifdef _WIN32
#pragma comment(lib, "gdi32.lib") // required for GetPixelFormat()
#endif

using namespace std;

// DEBUG - add a lot of noise
//#ifndef _DEBUG
//#define _DEBUG
//#endif

#if defined (_DEBUG) 
const char * get_attr_name( int val, int * pdef );
#define dbg_printf printf
#else
#if defined (__GNUC__)
#define dbg_printf(format,args...) ((void)0)
#else // defined (__GNUC__)
#define dbg_printf(
#endif // defined (__GNUC__)
#endif // defined (_DEBUG)

// CHOP/NOT CHOP SOME CODE TO GET SOMETHING WORKING!
#define ADD_QUERY_BUFFER    
#define ADD_GET_DRAWABLE
// =======================================


#ifdef _WIN32
static bool fctPtrInited = false;
/* WGL_ARB_pixel_format */
static wglChoosePixelFormatARBProc wglChoosePixelFormatARBPtr = 0;
static wglGetPixelFormatAttribivARBProc wglGetPixelFormatAttribivARBPtr = 0;
/* WGL_ARB_pbuffer */
static wglCreatePbufferARBProc wglCreatePbufferARBPtr = 0;
static wglGetPbufferDCARBProc wglGetPbufferDCARBPtr = 0;
static wglQueryPbufferARBProc wglQueryPbufferARBPtr = 0;
static wglReleasePbufferDCARBProc wglReleasePbufferDCARBPtr = 0;
static wglDestroyPbufferARBProc wglDestroyPbufferARBPtr = 0;
/* WGL_ARB_render_texture */
static wglBindTexImageARBProc wglBindTexImageARBPtr = 0;
static wglReleaseTexImageARBProc wglReleaseTexImageARBPtr = 0;

#elif defined( __MACH__ )
#else /* !_WIN32 */
static bool glXVersion1_3Present = false;
static glXChooseFBConfigProc glXChooseFBConfigPtr = 0;
static glXCreatePbufferProc glXCreatePbufferPtr = 0;
static glXCreateGLXPbufferProc glXCreateGLXPbufferPtr = 0;
static glXGetVisualFromFBConfigProc glXGetVisualFromFBConfigPtr = 0;
static glXCreateContextProc glXCreateContextPtr = 0;
static glXCreateContextWithConfigProc glXCreateContextWithConfigPtr = 0;
static glXDestroyPbufferProc glXDestroyPbufferPtr = 0;
static glXQueryDrawableProc glXQueryDrawablePtr = 0;
static glXQueryGLXPbufferSGIXProc glXQueryGLXPbufferSGIXPtr = 0;
#endif

//---------------------------------------------------------------------------
// Function      : RenderTexture::RenderTexture
// Description	 : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::RenderTexture()
* @brief Mode-string-based Constructor.
*/ 
RenderTexture::RenderTexture(const char *strMode)
:   _iWidth(0), 
    _iHeight(0), 
    _bIsTexture(false),
    _bIsDepthTexture(false),
    _bHasARBDepthTexture(true),            // [Redge]
#if defined(_WIN32) || defined(__MACH__)
    _eUpdateMode(RT_RENDER_TO_TEXTURE),
#else
    _eUpdateMode(RT_COPY_TO_TEXTURE),
#endif
    _bInitialized(false),
    _iNumAuxBuffers(0),
    _bIsBufferBound(false),
    _iCurrentBoundBuffer(0),
    _iNumDepthBits(0),
    _iNumStencilBits(0),
    _bFloat(false),
    _bDoubleBuffered(false),
    _bPowerOf2(true),
    _bRectangle(false),
    _bMipmap(false),
    _bShareObjects(false),
    _bCopyContext(false),
#ifdef _WIN32
    _hDC(NULL), 
    _hGLContext(NULL), 
    _hPBuffer(NULL),
    _hPreviousDC(0),
    _hPreviousContext(0),
#elif defined( __MACH__ )
    _hGLContext(NULL),
    _hPBuffer(0),
    _hPreviousContext(NULL),
#else
    _pDisplay(NULL),
    _hGLContext(NULL),
    _hPBuffer(0),
    _hPreviousDrawable(0),
    _hPreviousContext(0),
#endif
    _iTextureTarget(GL_NONE),
    _iTextureID(0),
    _iDepthTextureID(0),
    _pPoorDepthTexture(0) // [Redge]
{
    dbg_printf("RenderTexture::RenderTexture(%s) BGN instantiation.\n", strMode );
    _iNumColorBits[0] = _iNumColorBits[1] = 
        _iNumColorBits[2] = _iNumColorBits[3] = 0;

#ifdef _WIN32
    dbg_printf("RenderTexture::RenderTexture in _WIN32.\n" );
    _pixelFormatAttribs.push_back(WGL_DRAW_TO_PBUFFER_ARB);
    _pixelFormatAttribs.push_back(true);
    _pixelFormatAttribs.push_back(WGL_SUPPORT_OPENGL_ARB);
    _pixelFormatAttribs.push_back(true);
    
    _pbufferAttribs.push_back(WGL_PBUFFER_LARGEST_ARB);
    _pbufferAttribs.push_back(true);
#elif defined( __MACH__ )
    dbg_printf("RenderTexture::RenderTexture in __MACH__.\n" );
    //_pixelFormatAttribs.push_back(kCGLPFANoRecovery);
    _pixelFormatAttribs.push_back(kCGLPFAAccelerated);
    _pixelFormatAttribs.push_back(kCGLPFAWindow);
    _pixelFormatAttribs.push_back(kCGLPFAPBuffer);
#else
    dbg_printf("RenderTexture::RenderTexture !_WIN32 and !__MACH__.\n" );
    _pbufferAttribs.push_back(GLX_RENDER_TYPE_SGIX);
    _pbufferAttribs.push_back(GLX_RGBA_BIT_SGIX);
    _pbufferAttribs.push_back(GLX_DRAWABLE_TYPE_SGIX);
    _pbufferAttribs.push_back(GLX_PBUFFER_BIT_SGIX);
#endif

    _ParseModeString(strMode, _pixelFormatAttribs, _pbufferAttribs);

#ifdef _WIN32
    _pixelFormatAttribs.push_back(0);
    _pbufferAttribs.push_back(0);
#elif defined(__MACH__)
    _pixelFormatAttribs.push_back((CGLPixelFormatAttribute)0);
    _pbufferAttribs.push_back((CGLPixelFormatAttribute)0);
#else
    _pixelFormatAttribs.push_back(None);
#endif

    dbg_printf("RenderTexture::RenderTexture(%s) END instantiation. pf=%d pb=%d\n",
      strMode, (int)_pixelFormatAttribs.size(), (int)_pbufferAttribs.size() );

}


//---------------------------------------------------------------------------
// Function     	: RenderTexture::~RenderTexture
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::~RenderTexture()
* @brief Destructor.
*/ 
RenderTexture::~RenderTexture()
{
    dbg_printf("RenderTexture::~RenderTexture: destructor.\n" );
    _Invalidate();
}

 //---------------------------------------------------------------------------
// Function            : _cglcheckError
// Description     :
//---------------------------------------------------------------------------
/**
* @fn _cglCheckError()
* @brief Prints to stderr a description when an OS X error is found.
*/
#ifdef __MACH__
static void _cglCheckErrorHelper(CGLError err, char *sourceFile, int sourceLine)
{
# ifdef _DEBUG
    if (err)
    {
       fprintf(stderr, "RenderTexture CGL Error:  %s at (%s,%d)",
                        CGLErrorString(err), sourceFile, sourceLine);
    }
# endif
}

# define _cglCheckError(err) _cglCheckErrorHelper(err,__FILE__,__LINE__)

#endif


//---------------------------------------------------------------------------
// Function     	: _wglGetLastError
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn wglGetLastError()
* @brief Returns the last windows error generated.
*/ 
#ifdef _WIN32
void _wglGetLastError()
{
#ifdef _DEBUG
    
    DWORD err = GetLastError();
    switch(err)
    {
    case ERROR_INVALID_PIXEL_FORMAT:
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Win32 Error:  ERROR_INVALID_PIXEL_FORMAT");
        break;
    case ERROR_NO_SYSTEM_RESOURCES:
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Win32 Error:  ERROR_NO_SYSTEM_RESOURCES");
        break;
    case ERROR_INVALID_DATA:
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Win32 Error:  ERROR_INVALID_DATA");
        break;
    case ERROR_INVALID_WINDOW_HANDLE:
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Win32 Error:  ERROR_INVALID_WINDOW_HANDLE");
        break;
    case ERROR_RESOURCE_TYPE_NOT_FOUND:
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Win32 Error:  ERROR_RESOURCE_TYPE_NOT_FOUND");
        break;
    case ERROR_SUCCESS:
        // no error
        break;
    default:
        LPVOID lpMsgBuf;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM | 
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &lpMsgBuf,
            0,
            NULL);
        
        SG_LOG(SG_GL, SG_ALERT, "RenderTexture Win32 Error " <<  err << ":" << lpMsgBuf);
        LocalFree( lpMsgBuf );
        break;
    }
    SetLastError(0);
    
#endif // _DEBUG
}
#endif

//---------------------------------------------------------------------------
// Function     	: PrintExtensionError
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn PrintExtensionError( char* strMsg, ... )
* @brief Prints an error about missing OpenGL extensions.
*/ 
void PrintExtensionError( const char* strMsg, ... )
{
    SG_LOG(SG_GL, SG_ALERT, 
            "Error: RenderTexture requires the following unsupported "
            "OpenGL extensions: ");
    char strBuffer[512];
    va_list args;
    va_start(args, strMsg);
#if defined _WIN32 && !defined __CYGWIN__
    _vsnprintf( strBuffer, 512, strMsg, args );
#else
    vsnprintf( strBuffer, 512, strMsg, args );
#endif
    va_end(args);
    
    SG_LOG(SG_GL, SG_ALERT, strMsg);
    dbg_printf("Error: RenderTexture requires the following unsupported OpenGL extensions: \n[%s]\n", strMsg);
}

//---------------------------------------------------------------------------
// Function     	: RenderTexture::Initialize
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::Initialize(int width, int height, bool shareObjects, bool copyContext);
* @brief Initializes the RenderTexture, sharing display lists and textures if specified.
* 
* This function creates of the p-buffer.  It can only be called once a GL 
* context has already been created.  
*/ 
bool RenderTexture::Initialize(int width, int height,
                                bool shareObjects       /* = true */,
                                bool copyContext        /* = false */)
{
    assert(width > 0 && height > 0);

    dbg_printf("RenderTexture::Initialize w=%d h=%d\n", width, height );
    
    _iWidth = width; _iHeight = height;
    _bPowerOf2 = IsPowerOfTwo(width) && IsPowerOfTwo(height);

    _bShareObjects = shareObjects;
    _bCopyContext  = copyContext;

    // Check if this is an NVXX GPU and verify necessary extensions.
    if (!_VerifyExtensions()) {
       dbg_printf("RenderTexture::Initialize: _VerifyExtensions() FAILED - returning false.\n" );
       return false;
    }
    
    if (_bInitialized)
        _Invalidate();

#ifdef _WIN32
    // Get the current context.
    HDC hdc = wglGetCurrentDC();
    if (NULL == hdc)
        _wglGetLastError();
    HGLRC hglrc = wglGetCurrentContext();
    if (NULL == hglrc)
        _wglGetLastError();
    
    int iFormat = 0;
    unsigned int iNumFormats;
    
    if (_bCopyContext)
    {
        // Get the pixel format for the on-screen window.
        iFormat = GetPixelFormat(hdc);
        if (iFormat == 0)
        {
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Error: GetPixelFormat() failed.");
            return false;
        }
    }
    else 
    {
        if (!wglChoosePixelFormatARBPtr(hdc, &_pixelFormatAttribs[0], NULL, 
                                     1, &iFormat, &iNumFormats))
        {
            SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Error: wglChoosePixelFormatARB() failed.");
            _wglGetLastError();
            return false;
        }
        if ( iNumFormats <= 0 )
        {
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Error: Couldn't find a suitable "
                    "pixel format.");
            _wglGetLastError();
            return false;
        }
    }
    
    // Create the p-buffer.    
    _hPBuffer = wglCreatePbufferARBPtr(hdc, iFormat, _iWidth, _iHeight, 
                                    &_pbufferAttribs[0]);
    if (!_hPBuffer)
    {
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Error: wglCreatePbufferARB() failed.");
        _wglGetLastError();
        return false;
    }
    
    // Get the device context.
    _hDC = wglGetPbufferDCARBPtr( _hPBuffer);
    if ( !_hDC )
    {
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Error: wglGetGetPbufferDCARB() failed.");
        _wglGetLastError();
        return false;
    }
    
    // Create a gl context for the p-buffer.
    if (_bCopyContext)
    {
        // Let's use the same gl context..
        // Since the device contexts are compatible (i.e. same pixelformat),
        // we should be able to use the same gl rendering context.
        _hGLContext = hglrc;
    }
    else
    {
        _hGLContext = wglCreateContext( _hDC );
        if ( !_hGLContext )
        {
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Error: wglCreateContext() failed.");
            _wglGetLastError();
            return false;
        }
    }
    
    // Share lists, texture objects, and program objects.
    if( _bShareObjects )
    {
        if( !wglShareLists(hglrc, _hGLContext) )
        {
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Error: wglShareLists() failed.");
            _wglGetLastError();
            return false;
        }
    }
    
    // Determine the actual width and height we were able to create.
    wglQueryPbufferARBPtr( _hPBuffer, WGL_PBUFFER_WIDTH_ARB, &_iWidth );
    wglQueryPbufferARBPtr( _hPBuffer, WGL_PBUFFER_HEIGHT_ARB, &_iHeight );
    
    _bInitialized = true;
    
    // get the actual number of bits allocated:
    int attrib = WGL_RED_BITS_ARB;
    //int bits[6];
    int value;
    _iNumColorBits[0] = 
        (wglGetPixelFormatAttribivARBPtr(_hDC, iFormat, 0, 1, &attrib, &value)) 
        ? value : 0;
    attrib = WGL_GREEN_BITS_ARB;
    _iNumColorBits[1] = 
        (wglGetPixelFormatAttribivARBPtr(_hDC, iFormat, 0, 1, &attrib, &value)) 
        ? value : 0;
    attrib = WGL_BLUE_BITS_ARB;
    _iNumColorBits[2] = 
        (wglGetPixelFormatAttribivARBPtr(_hDC, iFormat, 0, 1, &attrib, &value)) 
        ? value : 0;
    attrib = WGL_ALPHA_BITS_ARB;
    _iNumColorBits[3] = 
        (wglGetPixelFormatAttribivARBPtr(_hDC, iFormat, 0, 1, &attrib, &value)) 
        ? value : 0; 
    attrib = WGL_DEPTH_BITS_ARB;
    _iNumDepthBits = 
        (wglGetPixelFormatAttribivARBPtr(_hDC, iFormat, 0, 1, &attrib, &value)) 
        ? value : 0; 
    attrib = WGL_STENCIL_BITS_ARB;
    _iNumStencilBits = 
        (wglGetPixelFormatAttribivARBPtr(_hDC, iFormat, 0, 1, &attrib, &value)) 
        ? value : 0; 
    attrib = WGL_DOUBLE_BUFFER_ARB;
    _bDoubleBuffered = 
        (wglGetPixelFormatAttribivARBPtr(_hDC, iFormat, 0, 1, &attrib, &value)) 
        ? (value?true:false) : false; 
    
#if defined(_DEBUG) | defined(DEBUG)
    SG_LOG(SG_GL, SG_ALERT, "Created a " << _iWidth << "x" << _iHeight <<
        " RenderTexture with BPP(" <<
        _iNumColorBits[0] << "," << _iNumColorBits[1] << "," <<
        _iNumColorBits[2] << "," << _iNumColorBits[3] << ")");
    if (_iNumDepthBits) SG_LOG(SG_GL, SG_ALERT, " depth=" << _iNumDepthBits);
    if (_iNumStencilBits) SG_LOG(SG_GL, SG_ALERT, " stencil=" << _iNumStencilBits);
    if (_bDoubleBuffered) SG_LOG(SG_GL, SG_ALERT, " double buffered");
#endif

#elif defined( __MACH__ )
    // Get the current context.
    CGLContextObj hglrc = CGLGetCurrentContext();
    if (NULL == hglrc)
        fprintf(stderr, "Couldn't get current context!");
   
        CGLPixelFormatObj pixFormat = NULL;
        GLint iNumFormats;
        CGLError error;

        // Copy the _pixelFormatAttribs into another array to fix
        // typecast issues
        CGLPixelFormatAttribute *pixFormatAttribs =
            (CGLPixelFormatAttribute *)malloc(sizeof(CGLPixelFormatAttribute)
                                              * _pixelFormatAttribs.size());

        for (unsigned int ii = 0; ii < _pixelFormatAttribs.size(); ii++)
        {
            pixFormatAttribs[ii] =
                               (CGLPixelFormatAttribute)_pixelFormatAttribs[ii];
        }

        if (error =
           CGLChoosePixelFormat(&pixFormatAttribs[0], &pixFormat, &iNumFormats))
        {
            fprintf(stderr,
                    "RenderTexture Error: CGLChoosePixelFormat() failed.\n");
            _cglCheckError(error);
            return false;
        }
        if ( iNumFormats <= 0 )
        {
            SG_LOG(SG_GL, SG_ALERT,
                    "RenderTexture Error: Couldn't find a suitable "
                    "pixel format.");
            return false;
        }

        // Free the copy of the _pixelFormatAttribs array
        free(pixFormatAttribs);
   
       // Create the context.
       error = CGLCreateContext(pixFormat, (_bShareObjects)
                ? CGLGetCurrentContext() : NULL, &_hGLContext);
       if (error)
       {
           fprintf(stderr,
                   "RenderTexture Error: CGLCreateContext() failed.\n");
           _cglCheckError(error);
           return false;
       }
       CGLDestroyPixelFormat(pixFormat);
   
       // Create the p-buffer.
       error = CGLCreatePBuffer(_iWidth, _iHeight, (_bRectangle)
            ? GL_TEXTURE_RECTANGLE_EXT : GL_TEXTURE_2D, GL_RGBA, 0, &_hPBuffer);
       if (error)
       {
           fprintf(stderr,
                   "RenderTexture Error: CGLCreatePBuffer() failed.\n");
           _cglCheckError(error);
           return false;
       }

       GLint screen;
       if (error = CGLGetVirtualScreen(CGLGetCurrentContext(), &screen))
       {
           _cglCheckError(error);
           return false;
       }
       if (error = CGLSetPBuffer(_hGLContext, _hPBuffer, 0, 0, screen))
       {
           _cglCheckError(error);
           return false;
       }
   
       // Determine the actual width and height we were able to create.
       //wglQueryPbufferARB( _hPBuffer, WGL_PBUFFER_WIDTH_ARB, &_iWidth );
       //wglQueryPbufferARB( _hPBuffer, WGL_PBUFFER_HEIGHT_ARB, &_iHeight );
   
       _bInitialized = true;
   
       /*
       // get the actual number of bits allocated:
       int attrib = WGL_RED_BITS_ARB;
       //int bits[6];
       int value;
       _iNumColorBits[0] =
            (wglGetPixelFormatAttribivARB(_hDC, iFormat, 0, 1, &attrib, &value))
            ? value : 0;
       attrib = WGL_GREEN_BITS_ARB;
       _iNumColorBits[1] =
            (wglGetPixelFormatAttribivARB(_hDC, iFormat, 0, 1, &attrib, &value))
            ? value : 0;
       attrib = WGL_BLUE_BITS_ARB;
       _iNumColorBits[2] =
            (wglGetPixelFormatAttribivARB(_hDC, iFormat, 0, 1, &attrib, &value))
            ? value : 0;
       attrib = WGL_ALPHA_BITS_ARB;
       _iNumColorBits[3] =
            (wglGetPixelFormatAttribivARB(_hDC, iFormat, 0, 1, &attrib, &value))
            ? value : 0;
       attrib = WGL_DEPTH_BITS_ARB;
       _iNumDepthBits =
            (wglGetPixelFormatAttribivARB(_hDC, iFormat, 0, 1, &attrib, &value))
            ? value : 0;
       attrib = WGL_STENCIL_BITS_ARB;
       _iNumStencilBits =
            (wglGetPixelFormatAttribivARB(_hDC, iFormat, 0, 1, &attrib, &value))
            ? value : 0;
       attrib = WGL_DOUBLE_BUFFER_ARB;
       _bDoubleBuffered =
            (wglGetPixelFormatAttribivARB(_hDC, iFormat, 0, 1, &attrib, &value))
             ? (value?true:false) : false;
       */

#if defined(_DEBUG) | defined(DEBUG)
       fprintf(stderr, "Created a %dx%d RenderTexture with BPP(%d, %d, %d, %d)",
           _iWidth, _iHeight,
           _iNumColorBits[0], _iNumColorBits[1],
           _iNumColorBits[2], _iNumColorBits[3]);
       if (_iNumDepthBits) fprintf(stderr, " depth=%d", _iNumDepthBits);
       if (_iNumStencilBits) fprintf(stderr, " stencil=%d", _iNumStencilBits);
       if (_bDoubleBuffered) fprintf(stderr, " double buffered");
       fprintf(stderr, "\n");
#endif

#else // !_WIN32, !__MACH_

    _pDisplay = glXGetCurrentDisplay();
    if ( !_pDisplay ) {
        dbg_printf("RenderTexture::Initialize: ERROR: glXGetCurrentDisplay() returned NULL! return false\n");
        return false;
    }
    dbg_printf("RenderTexture::Initialize: got glXGetCurrentDisplay() _pDisplay=[%p]\n", _pDisplay);
    // glXGetCurrentContext returns the current context, as specified by glXMakeCurrent.
    // If there is no current context, NULL is returned.
    GLXContext context = glXGetCurrentContext();
    if ( !context ) {
        dbg_printf("RenderTexture::Initialize: ERROR: glXGetCurrentContext() returned NULL! return false\n");
        return false;
    }
    dbg_printf("RenderTexture::Initialize: got glXGetCurrentContext() context=[%p]\n", context);
    int screen = DefaultScreen(_pDisplay);
    dbg_printf("RenderTexture::Initialize: DefaultScreen(_pDisplay) screen=%d\n", screen);
    
    XVisualInfo *visInfo = NULL;
    
    GLXFBConfig *fbConfigs;
    int nConfigs;
#ifdef _DEBUG
    dbg_printf("Using %d pixelFormatAttribs array\n", (int)_pixelFormatAttribs.size());
    size_t max = _pixelFormatAttribs.size() / 2;
    int dat = 0;
    size_t n;
    for (n = 0; n < max; n++) {
      const char * cp = get_attr_name(_pixelFormatAttribs[n*2], &dat);
      printf( "%s(%d) = %d (def=%d)\n",
       cp, _pixelFormatAttribs[n*2], _pixelFormatAttribs[(n*2)+1], dat );
    }
    n *= 2;
    if ( n < _pixelFormatAttribs.size() )
      printf( "Array ends with %d\n", _pixelFormatAttribs[n] );
    else
      printf( "WARNING: Array does not appear ODD! which is ODD\n" );
      
#endif
    fbConfigs = glXChooseFBConfigPtr(_pDisplay, screen, 
                                      &_pixelFormatAttribs[0], &nConfigs);
    // NOTE: ref: http://www.opengl.org/sdk/docs/man/xhtml/glXChooseFBConfig.xml says
    // Next, call glXGetFBConfigAttrib to retrieve additional attributes for the frame buffer configurations and then select between them.
    
    if (nConfigs <= 0 || !fbConfigs) 
    {
        SG_LOG(SG_GL, SG_ALERT,
            "RenderTexture Error: Couldn't find a suitable pixel format.");
        dbg_printf("RenderTexture Error: Couldn't find a suitable pixel format. return false\n");
        return false;
    }
    
    dbg_printf("RenderTexture::Initialize: glXChooseFBConfigPtr() nConfigs = %d, fbConfigs=[%p]\n", nConfigs, fbConfigs);
    
    int pbufAttrib[] = {
      GLX_PBUFFER_WIDTH,   _iWidth,
      GLX_PBUFFER_HEIGHT,  _iHeight,
      GLX_LARGEST_PBUFFER, False,
      None
    };
    // Pick the first returned format that will return a pbuffer
    // if (glXVersion1_3Present)
    if (glXCreatePbufferPtr && glXGetVisualFromFBConfigPtr && glXCreateContextPtr) 
    {
        dbg_printf("RenderTexture::Initialize: glXVersion1_3Present = TRUE\n");
        for (int i=0;i<nConfigs;i++)
        {
            _hPBuffer = glXCreatePbufferPtr(_pDisplay, fbConfigs[i], pbufAttrib);
            if (_hPBuffer)
            {
                XVisualInfo *visInfo = glXGetVisualFromFBConfigPtr(_pDisplay, fbConfigs[i]);

                _hGLContext = glXCreateContextPtr(_pDisplay, visInfo,
                                               _bShareObjects ? context : NULL,
                                               True);
                if (!_hGLContext)
                {
                    dbg_printf("RenderTexture Error: glXCreateContextPtr(_pDisplay, visInfo,..) FAILED! return false\n");
                    return false;
                }
                XFree( visInfo );
                break;
            }
        }
    }
    else
    {
#ifdef WIN32
        int iFormat = 0;
        int iNumFormats;
        int attrib = 0;
#endif
        dbg_printf("RenderTexture::Initialize: glXVersion1_3Present = FALSE w=%d h=%d\n", _iWidth, _iHeight);
        for (int i=0; i < nConfigs; i++)
        {
             dbg_printf("RenderTexture::Initialize: %d: glXCreateGLXPbufferPtr() get buffer ptr\n", (i + 1));
            _hPBuffer = glXCreateGLXPbufferPtr(_pDisplay, fbConfigs[i], 
                                               _iWidth, _iHeight,
                                               &pbufAttrib[0] ); //NULL);
            if (_hPBuffer) 
            {
                dbg_printf("RenderTexture::Initialize: %d: glXCreateGLXPbufferPtr() got buffer [%p]\n", (i + 1), (void*)_hPBuffer);
                _hGLContext = glXCreateContextWithConfigPtr(_pDisplay, 
                                                             fbConfigs[i], 
                                                             GLX_RGBA_TYPE, 
                                                             _bShareObjects ? context : NULL, 
                                                             True);
                dbg_printf("RenderTexture::Initialize: %d: glXCreateContextWithConfigPtr() _hGLContext=[%p]\n", (i + 1), _hGLContext);
                break;
            } else {
                dbg_printf("RenderTexture::Initialize: %d: glXCreateGLXPbufferPtr() NO buffer\n", (i + 1));
            }
        }
    }
    
    dbg_printf("RenderTexture::Initialize: doing XFree( fbConfigs=[%p].\n", fbConfigs);
    XFree( fbConfigs );
    
    if (!_hPBuffer)
    {
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Error: glXCreateGLXPbufferPtr() failed.");
        dbg_printf("RenderTexture Error: glXCreateGLXPbufferPtr() failed.\n");
        return false;
    }
    
    if(!_hGLContext)
    {
        // Try indirect
        _hGLContext = glXCreateContext(_pDisplay, visInfo, 
                                       _bShareObjects ? context : NULL, False);
        if ( !_hGLContext )
        {
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Error: glXCreateContext() failed.");
            dbg_printf("RenderTexture Error: glXCreateContext() failed. return false\n");
            return false;
        }
    }
    
    // if (!glXVersion1_3Present)
    if ((!glXCreatePbufferPtr || !glXGetVisualFromFBConfigPtr || !glXCreateContextPtr) &&
        (!glXVersion1_3Present))
    {
#ifdef ADD_QUERY_BUFFER    
        dbg_printf("RenderTexture::Initialize: Doing glXQueryGLXPbufferSGIXPtr with GLX_WIDTH_SGIX\n");
        glXQueryGLXPbufferSGIXPtr(_pDisplay, _hPBuffer, GLX_WIDTH_SGIX, 
                                  (GLuint*)&_iWidth);
        dbg_printf("RenderTexture::Initialize: Doing glXQueryGLXPbufferSGIXPtr with GLX_HEIGHT_SGIX\n");
        glXQueryGLXPbufferSGIXPtr(_pDisplay, _hPBuffer, GLX_HEIGHT_SGIX, 
                                  (GLuint*)&_iHeight);
#else
        dbg_printf("RenderTexture::Initialize: SKIPPED doing glXQueryGLXPbufferSGIXPtr with GLX_WIDTH_SGIX and GLX_HEIGHT_SGIX\n");
#endif // #ifdef ADD_QUERY_BUFFER    
    }
    
    _bInitialized = true;
    dbg_printf( "RenderTexture::Initialize: _bIniialized set TRUE\n" );
    // XXX Query the color format
    
#endif

    
    // Now that the pbuffer is created, allocate any texture objects needed,
    // and initialize them (for CTT updates only).  These must be allocated
    // in the context of the pbuffer, though, or the RT won't work without
    // wglShareLists.
#ifdef _WIN32
    if (false == wglMakeCurrent( _hDC, _hGLContext))
    {
        _wglGetLastError();
        return false;
    }
#elif defined( __MACH__ )
    CGLError err;
    if (err = CGLSetCurrentContext(_hGLContext))
    {
        _cglCheckError(err);
        return false;
    }
#else

#ifdef ADD_GET_DRAWABLE
     dbg_printf( "RenderTexture::Initialize: doing glXGetCurrentContext()\n" );
    _hPreviousContext = glXGetCurrentContext();
     dbg_printf( "RenderTexture::Initialize: doing glXGetCurrentDrawable()\n" );
    _hPreviousDrawable = glXGetCurrentDrawable();
#else
    _hPreviousContext = context;
    _hPreviousDrawable = 0;
     dbg_printf( "RenderTexture::Initialize: SKIPPED doing glXGetCurrentContext(2nd) AND glXGetCurrentDrawable()\n" );
#endif // #ifdef ADD_GET_DRAWABLE
    
    dbg_printf( "RenderTexture::Initialize: doing glXMakeCurrent(_pDisplay(%p), _hPBuffer(%p), _hGLContext(%p))\n",
       _pDisplay, (void*)_hPBuffer, _hGLContext );
    if (False == glXMakeCurrent(_pDisplay, _hPBuffer, _hGLContext)) 
    {
        dbg_printf( "glXMakeCurrent(_pDisplay, _hPBuffer, _hGLContext) FAILED. return false\n" );
        return false;
    }
    
#endif
    
    bool result = _InitializeTextures();   
#ifdef _WIN32
    BindBuffer(WGL_FRONT_LEFT_ARB);
    _BindDepthBuffer();
#endif

    
#ifdef _WIN32 
    // make the previous rendering context current 
    if (false == wglMakeCurrent( hdc, hglrc))
    {
        _wglGetLastError();
        return false;
    }
#elif defined( __MACH__ )
    if (err = CGLSetCurrentContext(_hPreviousContext))
    {
        _cglCheckError(err);
        return false;
    }
#else
    if (False == glXMakeCurrent(_pDisplay, 
                                _hPreviousDrawable, _hPreviousContext))
    {
        dbg_printf("glXMakeCurrent(_pDisplay, _hPreviousDrawable, _hPreviousContext)) FAILED! return false\n" );
        return false;
    }
    
    if (glXVersion1_3Present)
    //if ((glXVersion1_3Present) ||
    //    (glXCreatePbufferPtr && glXGetVisualFromFBConfigPtr && glXCreateContextPtr && glXQueryDrawablePtr)) 
    {
        dbg_printf( "RenderTexture::Initialize: doing glXGetCurrentDrawable()\n" );
        GLXDrawable draw = glXGetCurrentDrawable();
        dbg_printf( "RenderTexture::Initialize: doing glXQueryDrawablePtr for GLX_WIDTH and GLX_HEIGHT\n" );
        glXQueryDrawablePtr(_pDisplay, draw, GLX_WIDTH, (unsigned int*)&_iWidth);
        glXQueryDrawablePtr(_pDisplay, draw, GLX_HEIGHT, (unsigned int*)&_iHeight);
    }
#endif

    dbg_printf( "RenderTexture::Initialize(w=%d,h=%d): returning %s\n", _iWidth, _iHeight, (result ? "TRUE" : "FALSE") );
    return result;
}


//---------------------------------------------------------------------------
// Function     	: RenderTexture::_Invalidate
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_Invalidate()
* @brief Returns the pbuffer memory to the graphics device.
* 
*/ 
bool RenderTexture::_Invalidate()
{
    dbg_printf( "RenderTexture::_Invalidate() called...\n" );
    
    _iNumColorBits[0] = _iNumColorBits[1] = 
        _iNumColorBits[2] = _iNumColorBits[3] = 0;
    _iNumDepthBits = 0;
    _iNumStencilBits = 0;
    
    if (_bIsTexture)
        glDeleteTextures(1, &_iTextureID);
    if (_bIsDepthTexture) 
    {
        // [Redge]
        if (!_bHasARBDepthTexture) delete[] _pPoorDepthTexture;
        // [/Redge]
        glDeleteTextures(1, &_iDepthTextureID);
    }
    
#ifdef _WIN32
    if ( _hPBuffer )
    {
        // Check if we are currently rendering in the pbuffer
        if (wglGetCurrentContext() == _hGLContext)
            wglMakeCurrent(0,0);
        if (!_bCopyContext) 
            wglDeleteContext( _hGLContext);
        wglReleasePbufferDCARBPtr( _hPBuffer, _hDC);
        wglDestroyPbufferARBPtr( _hPBuffer );
        _hPBuffer = 0;
        return true;
    }
#elif defined( __MACH__ )
    if ( _hPBuffer )
    {
        if (CGLGetCurrentContext() == _hGLContext)
            CGLSetCurrentContext(NULL);
        if (!_bCopyContext)
            CGLDestroyContext(_hGLContext);
        CGLDestroyPBuffer(_hPBuffer);
        _hPBuffer = NULL;
        return true;
    }
#else
    if ( _hPBuffer )
    {
        if(glXGetCurrentContext() == _hGLContext)
            // XXX I don't know if this is right at all
            glXMakeCurrent(_pDisplay, _hPBuffer, 0);
        glXDestroyPbufferPtr(_pDisplay, _hPBuffer);
        _hPBuffer = 0;
       dbg_printf( "RenderTexture::_Invalidate: glXDestroyPbufferPtr(_pDisplay, _hPBuffer); return true\n" );
        return true;
    }
#endif

    dbg_printf( "RenderTexture::_Invalidate: return false\n" );
    // [WVB] do we need to call _ReleaseBoundBuffers() too?
    return false;
}


//---------------------------------------------------------------------------
// Function     	: RenderTexture::Reset
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::Reset()
* @brief Resets the resolution of the offscreen buffer.
* 
* Causes the buffer to delete itself.  User must call Initialize() again
* before use.
*/ 
bool RenderTexture::Reset(const char *strMode, ...)
{
   dbg_printf("RenderTexure:Reset(): with %s\n", strMode);
   
    _iWidth = 0; _iHeight = 0; 
    _bIsTexture = false; _bIsDepthTexture = false,
    _bHasARBDepthTexture = true;
#if defined(_WIN32) || defined(__MACH__)
    _eUpdateMode = RT_RENDER_TO_TEXTURE;
#else
    _eUpdateMode = RT_COPY_TO_TEXTURE;
#endif
    _bInitialized = false;
    _iNumAuxBuffers = 0; 
    _bIsBufferBound = false;
    _iCurrentBoundBuffer = 0;
    _iNumDepthBits = 0; _iNumStencilBits = 0;
    _bDoubleBuffered = false;
    _bFloat = false; _bPowerOf2 = true;
    _bRectangle = false; _bMipmap = false; 
    _bShareObjects = false; _bCopyContext = false; 
    _iTextureTarget = GL_NONE; _iTextureID = 0; 
    _iDepthTextureID = 0;
    _pPoorDepthTexture = 0;
    _pixelFormatAttribs.clear();
    _pbufferAttribs.clear();
    if (IsInitialized())
    {
      if (!_Invalidate())
      {
        dbg_printf( "RenderTexture::Reset(): failed to invalidate. return false\n");
        return false;
      }
      else
      {
        dbg_printf( "RenderTexture::Reset(): was Initialized, and now _Invalidate() ok.");
      }
    }
    else
    {
      dbg_printf("RenderTexure:Reset(): previous NOT initialised!\n");
    }
    
    _iNumColorBits[0] = _iNumColorBits[1] = 
        _iNumColorBits[2] = _iNumColorBits[3] = 0;

#ifdef _WIN32
    _pixelFormatAttribs.push_back(WGL_DRAW_TO_PBUFFER_ARB);
    _pixelFormatAttribs.push_back(true);
    _pixelFormatAttribs.push_back(WGL_SUPPORT_OPENGL_ARB);
    _pixelFormatAttribs.push_back(true);
    
    _pbufferAttribs.push_back(WGL_PBUFFER_LARGEST_ARB);
    _pbufferAttribs.push_back(true);
#elif defined( __MACH__ )
    //_pixelFormatAttribs.push_back(kCGLPFANoRecovery);
    _pixelFormatAttribs.push_back(kCGLPFAAccelerated);
    _pixelFormatAttribs.push_back(kCGLPFAWindow);
    _pixelFormatAttribs.push_back(kCGLPFAPBuffer);
#else
    _pbufferAttribs.push_back(GLX_RENDER_TYPE_SGIX);
    _pbufferAttribs.push_back(GLX_RGBA_BIT_SGIX);
    _pbufferAttribs.push_back(GLX_DRAWABLE_TYPE_SGIX);
    _pbufferAttribs.push_back(GLX_PBUFFER_BIT_SGIX);
#endif

    va_list args;
    char strBuffer[256];
    va_start(args,strMode);
#if defined _WIN32 && !defined __CYGWIN__
    _vsnprintf( strBuffer, 256, strMode, args );
#else
    vsnprintf( strBuffer, 256, strMode, args );
#endif
    va_end(args);

    _ParseModeString(strBuffer, _pixelFormatAttribs, _pbufferAttribs);

#ifdef _WIN32
    _pixelFormatAttribs.push_back(0);
    _pbufferAttribs.push_back(0);
#elif defined(__MACH__)
    _pixelFormatAttribs.push_back((CGLPixelFormatAttribute)0);
#else
    _pixelFormatAttribs.push_back(None);
#endif
    dbg_printf("RenderTexure:Reset(): returning true.\n");
    return true;
}

//------------------------------------------------------------------------------
// Function     	  : RenderTexture::Resize
// Description	    : 
//------------------------------------------------------------------------------
/**
 * @fn RenderTexture::Resize(int iWidth, int iHeight)
 * @brief Changes the size of the offscreen buffer.
 * 
 * Like Reset() this causes the buffer to delete itself.  
 * But unlike Reset(), this call re-initializes the RenderTexture.
 * Note that Resize() will not work after calling Reset(), or before
 * calling Initialize() the first time.
 */ 
bool RenderTexture::Resize(int iWidth, int iHeight)
{
    if (!_bInitialized) {
        SG_LOG(SG_GL, SG_ALERT, "RenderTexture::Resize(): must Initialize() first.");
        return false;
    }
    if (iWidth == _iWidth && iHeight == _iHeight) {
        return true;
    }
    
    // Do same basic work as _Invalidate, but don't reset all our flags
    if (_bIsTexture)
        glDeleteTextures(1, &_iTextureID);
    if (_bIsDepthTexture)
        glDeleteTextures(1, &_iDepthTextureID);
#ifdef _WIN32
    if ( _hPBuffer )
    {
        // Check if we are currently rendering in the pbuffer
        if (wglGetCurrentContext() == _hGLContext)
            wglMakeCurrent(0,0);
        if (!_bCopyContext) 
            wglDeleteContext( _hGLContext);
        wglReleasePbufferDCARBPtr( _hPBuffer, _hDC);
        wglDestroyPbufferARBPtr( _hPBuffer );
        _hPBuffer = 0;
        return true;
    }
#elif defined( __MACH__ )
    if ( _hPBuffer )
    {
        if (CGLGetCurrentContext() == _hGLContext)
            CGLSetCurrentContext(NULL);
        if (!_bCopyContext)
            CGLDestroyContext(_hGLContext);
        CGLDestroyPBuffer(_hPBuffer);
        _hPBuffer = NULL;
        return true;
    }
#else
    if ( _hPBuffer )
    {
        if(glXGetCurrentContext() == _hGLContext)
            // XXX I don't know if this is right at all
            glXMakeCurrent(_pDisplay, _hPBuffer, 0);
        glXDestroyPbufferPtr(_pDisplay, _hPBuffer);
        _hPBuffer = 0;
    }
#endif
    else {
        SG_LOG(SG_GL, SG_ALERT, "RenderTexture::Resize(): failed to resize.");
        return false;
    }
    _bInitialized = false;
    return Initialize(iWidth, iHeight, _bShareObjects, _bCopyContext);
}

//---------------------------------------------------------------------------
// Function     	: RenderTexture::BeginCapture
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::BeginCapture()
* @brief Activates rendering to the RenderTexture.
*/ 
bool RenderTexture::BeginCapture()
{
    if (!_bInitialized)
    {
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture::BeginCapture(): Texture is not initialized!");
        return false;
    }
#ifdef _WIN32
    // cache the current context so we can reset it when EndCapture() is called.
    _hPreviousDC      = wglGetCurrentDC();
    if (NULL == _hPreviousDC)
        _wglGetLastError();
    _hPreviousContext = wglGetCurrentContext();
    if (NULL == _hPreviousContext)
        _wglGetLastError();
#elif defined( __MACH__ )
    _hPreviousContext = CGLGetCurrentContext();
#else
    _hPreviousContext = glXGetCurrentContext();
    _hPreviousDrawable = glXGetCurrentDrawable();
#endif

    _ReleaseBoundBuffers();

    return _MakeCurrent();
}


//---------------------------------------------------------------------------
// Function     	: RenderTexture::EndCapture
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::EndCapture()
* @brief Ends rendering to the RenderTexture.
*/ 
bool RenderTexture::EndCapture()
{    
    if (!_bInitialized)
    {
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture::EndCapture() : Texture is not initialized!");
        return false;
    }

    glFlush();	// Added by a-lex

    _MaybeCopyBuffer();

#ifdef _WIN32
    // make the previous rendering context current 
    if (FALSE == wglMakeCurrent( _hPreviousDC, _hPreviousContext))
    {
        _wglGetLastError();
        return false;
    }
#elif defined( __MACH__ )
    CGLError err;
    if (err = CGLSetCurrentContext(_hPreviousContext))
    {
            _cglCheckError(err);
            return false;
    }
#else
    if (False == glXMakeCurrent(_pDisplay, _hPreviousDrawable, 
                                _hPreviousContext))
    {
        return false;
    }
#endif

    // rebind the textures to a buffers for RTT
    BindBuffer(_iCurrentBoundBuffer);
    _BindDepthBuffer();

    return true;
}

//---------------------------------------------------------------------------
// Function     	  : RenderTexture::BeginCapture(RenderTexture*)
// Description	    : 
//---------------------------------------------------------------------------
/**
 * @fn RenderTexture::BeginCapture(RenderTexture* other)
 * @brief Ends capture of 'other', begins capture on 'this'
 *
 * When performing a series of operations where you modify one texture after 
 * another, it is more efficient to use this method instead of the equivalent
 * 'EndCapture'/'BeginCapture' pair.  This method switches directly to the 
 * new context rather than changing to the default context, and then to the 
 * new context.
 *
 * RenderTexture doesn't have any mechanism for determining if 
 * 'current' really is currently active, so no error will be thrown 
 * if it is not. 
 */ 
bool RenderTexture::BeginCapture(RenderTexture* current)
{
    // bool bContextReset = false;
    
    if (current == this) {
        return true; // no switch necessary
    }
    if (!current) {
        // treat as normal Begin if current is 0.
        return BeginCapture();
    }
    if (!_bInitialized)
    {
        SG_LOG(SG_GL, SG_ALERT, 
            "RenderTexture::BeginCapture(RenderTexture*): Texture is not initialized!");
        return false;
    }
    if (!current->_bInitialized)
    {
        SG_LOG(SG_GL, SG_ALERT, 
            "RenderTexture::BeginCapture(RenderTexture): 'current' texture is not initialized!");
        return false;
    }
    
    // Sync current pbuffer with its CTT texture if necessary
    current->_MaybeCopyBuffer();

    // pass along the previous context so we can reset it when 
    // EndCapture() is called.
#ifdef _WIN32
    _hPreviousDC      = current->_hPreviousDC;
    if (NULL == _hPreviousDC)
        _wglGetLastError();
    _hPreviousContext = current->_hPreviousContext;
    if (NULL == _hPreviousContext)
        _wglGetLastError();
#elif defined( __MACH__ )
    _hPreviousContext = current->_hPreviousContext;
#else
    _hPreviousContext = current->_hPreviousContext;
    _hPreviousDrawable = current->_hPreviousDrawable;
#endif    

    // Unbind textures before making context current
    if (!_ReleaseBoundBuffers()) 
      return false;

    // Make the pbuffer context current
    if (!_MakeCurrent())
        return false;

    // Rebind buffers of initial RenderTexture
    current->BindBuffer(_iCurrentBoundBuffer);
    current->_BindDepthBuffer();
    
    return true;
}



//---------------------------------------------------------------------------
// Function     	: RenderTexture::Bind
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::Bind()
* @brief Binds RGB texture.
*/ 
void RenderTexture::Bind() const 
{ 
    if (_bInitialized && _bIsTexture) 
    {
        glBindTexture(_iTextureTarget, _iTextureID);
    }    
}


//---------------------------------------------------------------------------
// Function     	: RenderTexture::BindDepth
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::BindDepth()
* @brief Binds depth texture.
*/ 
void RenderTexture::BindDepth() const 
{ 
    if (_bInitialized && _bIsDepthTexture) 
    {
        glBindTexture(_iTextureTarget, _iDepthTextureID); 
    }
}


//---------------------------------------------------------------------------
// Function     	: RenderTexture::BindBuffer
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::BindBuffer()
* @brief Associate the RTT texture id with 'iBuffer' (e.g. WGL_FRONT_LEFT_ARB)
*/ 
bool RenderTexture::BindBuffer( int iBuffer )
{ 
    // Must bind the texture too
    if (_bInitialized && _bIsTexture) 
    {
        glBindTexture(_iTextureTarget, _iTextureID);
        
#ifdef _WIN32
        if (RT_RENDER_TO_TEXTURE == _eUpdateMode && _bIsTexture &&
            (!_bIsBufferBound || _iCurrentBoundBuffer != iBuffer))
        {
            if (FALSE == wglBindTexImageARBPtr(_hPBuffer, iBuffer))
            {
                //  WVB: WGL API considers binding twice to the same buffer
                //  to be an error.  But we don't want to 
                //_wglGetLastError();
                //return false;
                SetLastError(0);
            }
            _bIsBufferBound = true;
            _iCurrentBoundBuffer = iBuffer;
        }
#elif defined(__MACH__)
    if (RT_RENDER_TO_TEXTURE == _eUpdateMode && _bIsTexture &&
         (!_bIsBufferBound || _iCurrentBoundBuffer != iBuffer))
    {
        CGLError err;
        if (err=CGLTexImagePBuffer(CGLGetCurrentContext(), _hPBuffer, iBuffer))
        {
            _cglCheckError(err);
        }
        _bIsBufferBound = true;
        _iCurrentBoundBuffer = iBuffer;
     }
#endif
    }    
    return true;
}


//---------------------------------------------------------------------------
// Function     	: RenderTexture::BindDepthBuffer
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_BindDepthBuffer()
* @brief Associate the RTT depth texture id with the depth buffer
*/ 
bool RenderTexture::_BindDepthBuffer() const
{
#ifdef WIN32
    if (_bInitialized && _bIsDepthTexture && 
        RT_RENDER_TO_TEXTURE == _eUpdateMode)
    {
        glBindTexture(_iTextureTarget, _iDepthTextureID);
        if (FALSE == wglBindTexImageARBPtr(_hPBuffer, WGL_DEPTH_COMPONENT_NV))
        {
            _wglGetLastError();
            return false;
        }
    }
#elif defined(__MACH__)
    if (_bInitialized && _bIsDepthTexture &&
        RT_RENDER_TO_TEXTURE == _eUpdateMode)
    {
        glBindTexture(_iTextureTarget, _iDepthTextureID);
        //if (FALSE == CGLTexImagePBuffer(<#CGLContextObj ctx#>,<#CGLPBufferObj pbuffer#>,<#unsigned long source#>)(_hPBuffer, WGL_DEPTH_COMPONENT_NV))
        //{
        //    _wglGetLastError();
        //    return false;
        //}
    }
#endif
    return true;
}

//---------------------------------------------------------------------------
// Function     	: RenderTexture::_ParseModeString
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_ParseModeString()
* @brief Parses the user-specified mode string for RenderTexture parameters.
*/ 
void RenderTexture::_ParseModeString(const char *modeString, 
                                     vector<int> &pfAttribs, 
                                     vector<int> &pbAttribs)
{
    dbg_printf("RenderTexture::_ParseModeString(%s). BGN vf=%d vp=%d\n", modeString, (int)pfAttribs.size(), (int)pbAttribs.size() );
    if (!modeString || strcmp(modeString, "") == 0)
        return;

	_iNumComponents = 0;
#if defined(_WIN32) || defined(__MACH__)
    _eUpdateMode = RT_RENDER_TO_TEXTURE;
#else
    _eUpdateMode = RT_COPY_TO_TEXTURE;
#endif
    
    int  iDepthBits = 0;
    bool bHasStencil = false;
    bool bBind2D   = false;
    bool bBindRECT = false;
    bool bBindCUBE = false;
    
    char *mode = strdup(modeString);
    

    vector<string> tokens;
    char *buf = strtok(mode, " ");
    while (buf != NULL)
    {
        tokens.push_back(buf);
        buf = strtok(NULL, " ");
    }
    free(mode);
    for (unsigned int i = 0; i < tokens.size(); i++)
    {
        string token = tokens[i];

        KeyVal kv = _GetKeyValuePair(token);
        
        
        if (kv.first == "rgb" && (_iNumComponents <= 1))
        {           
            if (kv.second.find("f") != kv.second.npos)
                _bFloat = true;

            vector<int> bitVec = _ParseBitVector(kv.second);

            if (bitVec.size() < 3) // expand the scalar to a vector
            {
                bitVec.push_back(bitVec[0]);
                bitVec.push_back(bitVec[0]);
            }

#ifdef _WIN32
            pfAttribs.push_back(WGL_RED_BITS_ARB);
            pfAttribs.push_back(bitVec[0]);
            pfAttribs.push_back(WGL_GREEN_BITS_ARB);
            pfAttribs.push_back(bitVec[1]);
            pfAttribs.push_back(WGL_BLUE_BITS_ARB);
            pfAttribs.push_back(bitVec[2]);
#elif defined( __MACH__ )
# if 0
            pfAttribs.push_back(AGL_RED_SIZE);
            pfAttribs.push_back(bitVec[0]);
            pfAttribs.push_back(AGL_GREEN_SIZE);
            pfAttribs.push_back(bitVec[1]);
            pfAttribs.push_back(AGL_BLUE_SIZE);
            pfAttribs.push_back(bitVec[2]);
# else
            pfAttribs.push_back(kCGLPFAColorSize);
            pfAttribs.push_back(bitVec[0] + bitVec[1] + bitVec[2]);
# endif
#else
            pfAttribs.push_back(GLX_RED_SIZE);
            pfAttribs.push_back(bitVec[0]);
            pfAttribs.push_back(GLX_GREEN_SIZE);
            pfAttribs.push_back(bitVec[1]);
            pfAttribs.push_back(GLX_BLUE_SIZE);
            pfAttribs.push_back(bitVec[2]);
#endif
	    _iNumComponents += 3;
            continue;
        }
		else if (kv.first == "rgb")
       {
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Warning: mistake in components definition "
                    "(rgb + " << _iNumComponents << ").");
            dbg_printf("RenderTexture Warning 1: mistake in components definition "
                    "(rgb + %d).\n", _iNumComponents );
       }
        
        if (kv.first == "rgba" && (_iNumComponents == 0))
        {
            if (kv.second.find("f") != kv.second.npos)
                _bFloat = true;

            vector<int> bitVec = _ParseBitVector(kv.second);

            if (bitVec.size() < 4) // expand the scalar to a vector
            {
                bitVec.push_back(bitVec[0]);
                bitVec.push_back(bitVec[0]);
                bitVec.push_back(bitVec[0]);
            }

#ifdef _WIN32
            pfAttribs.push_back(WGL_RED_BITS_ARB);
            pfAttribs.push_back(bitVec[0]);
            pfAttribs.push_back(WGL_GREEN_BITS_ARB);
            pfAttribs.push_back(bitVec[1]);
            pfAttribs.push_back(WGL_BLUE_BITS_ARB);
            pfAttribs.push_back(bitVec[2]);
            pfAttribs.push_back(WGL_ALPHA_BITS_ARB);
            pfAttribs.push_back(bitVec[3]);
#elif defined( __MACH__ )
# if 0 
            pfAttribs.push_back(AGL_RED_SIZE);
            pfAttribs.push_back(bitVec[0]);
            pfAttribs.push_back(AGL_GREEN_SIZE);
            pfAttribs.push_back(bitVec[1]);
            pfAttribs.push_back(AGL_BLUE_SIZE);
            pfAttribs.push_back(bitVec[2]);
            pfAttribs.push_back(AGL_ALPHA_SIZE);
            pfAttribs.push_back(bitVec[3]);
# else
            pfAttribs.push_back(kCGLPFAColorSize);
            pfAttribs.push_back(bitVec[0] + bitVec[1] + bitVec[2] + bitVec[3]);
            //pfAttribs.push_back(kCGLPFAAlphaSize);
            //pfAttribs.push_back(bitVec[3]);
            // printf("Color size: %d\n", bitVec[0] + bitVec[1] + bitVec[2] + bitVec[3]);

# endif
#else
            pfAttribs.push_back(GLX_RED_SIZE);
            pfAttribs.push_back(bitVec[0]);
            pfAttribs.push_back(GLX_GREEN_SIZE);
            pfAttribs.push_back(bitVec[1]);
            pfAttribs.push_back(GLX_BLUE_SIZE);
            pfAttribs.push_back(bitVec[2]);
            pfAttribs.push_back(GLX_ALPHA_SIZE);
            pfAttribs.push_back(bitVec[3]);
#endif
	    _iNumComponents = 4;
            continue;
        }
		else if (kv.first == "rgba") 
      {
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Warning: mistake in components definition "
                    "(rgba + " << _iNumComponents << ").");
            dbg_printf("RenderTexture Warning 2: mistake in components definition "
                    "(rgb + %d).\n", _iNumComponents );
      }
        
        if (kv.first == "r" && (_iNumComponents <= 1))
        {
            if (kv.second.find("f") != kv.second.npos)
                _bFloat = true;

            vector<int> bitVec = _ParseBitVector(kv.second);

#ifdef _WIN32
            pfAttribs.push_back(WGL_RED_BITS_ARB);
            pfAttribs.push_back(bitVec[0]);
#elif defined( __MACH__ )
# if 0
            pfAttribs.push_back(AGL_RED_SIZE);
            pfAttribs.push_back(bitVec[0]);
# else
            pfAttribs.push_back(kCGLPFAColorSize);
            pfAttribs.push_back(bitVec[0]);
# endif
#else
            pfAttribs.push_back(GLX_RED_SIZE);
            pfAttribs.push_back(bitVec[0]);
#endif
			_iNumComponents++;
            continue;
        }
	else if (kv.first == "r") 
   {
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Warning: mistake in components definition "
                    "(r + " << _iNumComponents << ").");
            dbg_printf("RenderTexture Warning 3: mistake in components definition "
                    "(rgb + %d).\n", _iNumComponents );
   }
        if (kv.first == "rg" && (_iNumComponents <= 1))
        {
            if (kv.second.find("f") != kv.second.npos)
                _bFloat = true;

            vector<int> bitVec = _ParseBitVector(kv.second);

            if (bitVec.size() < 2) // expand the scalar to a vector
            {
                bitVec.push_back(bitVec[0]);
            }

#ifdef _WIN32
            pfAttribs.push_back(WGL_RED_BITS_ARB);
            pfAttribs.push_back(bitVec[0]);
            pfAttribs.push_back(WGL_GREEN_BITS_ARB);
            pfAttribs.push_back(bitVec[1]);
#elif defined( __MACH__ )
# if 0
            pfAttribs.push_back(AGL_RED_SIZE);
            pfAttribs.push_back(bitVec[0]);
            pfAttribs.push_back(AGL_GREEN_SIZE);
            pfAttribs.push_back(bitVec[1]);
# else
            pfAttribs.push_back(kCGLPFAColorSize);
            pfAttribs.push_back(bitVec[0] + bitVec[1]);
# endif
#else
            pfAttribs.push_back(GLX_RED_SIZE);
            pfAttribs.push_back(bitVec[0]);
            pfAttribs.push_back(GLX_GREEN_SIZE);
            pfAttribs.push_back(bitVec[1]);
#endif
			_iNumComponents += 2;
            continue;
        }
		else if (kv.first == "rg") 
      {
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Warning: mistake in components definition "
                    "(rg + " << _iNumComponents << ").");
            dbg_printf("RenderTexture Warning 4: mistake in components definition "
                    "(rgb + %d).\n", _iNumComponents );
      }

        if (kv.first == "depth")
        {
            if (kv.second == "")
                iDepthBits = 24;
            else
                iDepthBits = strtol(kv.second.c_str(), 0, 10);
            continue;
        }

        if (kv.first == "stencil")
        {
            bHasStencil = true;
#ifdef _WIN32
            pfAttribs.push_back(WGL_STENCIL_BITS_ARB);
#elif defined( __MACH__ )
# if 0
            pfAttribs.push_back(AGL_STENCIL_SIZE);
# else
            pfAttribs.push_back(kCGLPFAStencilSize);
# endif
#else
            pfAttribs.push_back(GLX_STENCIL_SIZE);
#endif
            if (kv.second == "")
                pfAttribs.push_back(8);
            else
                pfAttribs.push_back(strtol(kv.second.c_str(), 0, 10));
            continue;
        }

        if (kv.first == "samples")
        {
#ifdef _WIN32
            pfAttribs.push_back(WGL_SAMPLE_BUFFERS_ARB);
            pfAttribs.push_back(1);
            pfAttribs.push_back(WGL_SAMPLES_ARB);
            pfAttribs.push_back(strtol(kv.second.c_str(), 0, 10));
#elif defined( __MACH__ )
# if 0
            pfAttribs.push_back(AGL_SAMPLE_BUFFERS_ARB);
            pfAttribs.push_back(1);
            pfAttribs.push_back(AGL_SAMPLES_ARB);
# else
            pfAttribs.push_back(kCGLPFAMultisample);
            pfAttribs.push_back(kCGLPFASamples);
# endif
            pfAttribs.push_back(strtol(kv.second.c_str(), 0, 10));
#else
	    pfAttribs.push_back(GLX_SAMPLE_BUFFERS_ARB);
	    pfAttribs.push_back(1);
	    pfAttribs.push_back(GLX_SAMPLES_ARB);
            pfAttribs.push_back(strtol(kv.second.c_str(), 0, 10));
#endif
            continue;

        }

        if (kv.first == "doublebuffer" || kv.first == "double")
        {
#ifdef _WIN32
            pfAttribs.push_back(WGL_DOUBLE_BUFFER_ARB);
            pfAttribs.push_back(true);
#elif defined( __MACH__ )
# if 0
            pfAttribs.push_back(AGL_DOUBLEBUFFER);
            pfAttribs.push_back(True);
# else
            pfAttribs.push_back(kCGLPFADoubleBuffer);
# endif
#else
            pfAttribs.push_back(GLX_DOUBLEBUFFER);
            pfAttribs.push_back(True);
#endif
            continue;
        }  
        
        if (kv.first == "aux")
        {
#ifdef _WIN32
            pfAttribs.push_back(WGL_AUX_BUFFERS_ARB);
#elif defined( __MACH__ )
# if 0
            pfAttribs.push_back(AGL_AUX_BUFFERS);
# else
            pfAttribs.push_back(kCGLPFAAuxBuffers);
# endif
#else
            pfAttribs.push_back(GLX_AUX_BUFFERS);
#endif
            if (kv.second == "")
                pfAttribs.push_back(0);
            else
                pfAttribs.push_back(strtol(kv.second.c_str(), 0, 10));
            continue;
        }
        
        if (token.find("tex") == 0)
        {            
            _bIsTexture = true;
            
            if ((kv.first == "texRECT") && (GL_NV_texture_rectangle ||
                                            GL_EXT_texture_rectangle))
            {
                _bRectangle = true;
                bBindRECT = true;
            }
            else if (kv.first == "texCUBE")
            {
                bBindCUBE = true;
            }
            else
            {
                bBind2D = true;
            }

            continue;
        }

        if (token.find("depthTex") == 0)
        {
            _bIsDepthTexture = true;
            
            if ((kv.first == "depthTexRECT") && (GL_NV_texture_rectangle ||
                                                 GL_EXT_texture_rectangle))
            {
                _bRectangle = true;
                bBindRECT = true;
            }
            else if (kv.first == "depthTexCUBE")
            {
                bBindCUBE = true;
            }
            else
            {
                bBind2D = true;
            }

            continue;
        }

        if (kv.first == "mipmap")
        {
            _bMipmap = true;    
            continue;
        }

        if (kv.first == "rtt")
        {
            _eUpdateMode = RT_RENDER_TO_TEXTURE;
            continue;
        }
        
        if (kv.first == "ctt")
        {
            _eUpdateMode = RT_COPY_TO_TEXTURE;
            continue;
        }

        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Error: Unknown pbuffer attribute: " <<
                token.c_str());
        dbg_printf("RenderTexture Error: Uknown pbuffer attribute: %s\n",
            token.c_str() );
    }

    // Processing of some options must be last because of interactions.

    // Check for inconsistent texture targets
    if (_bIsTexture && _bIsDepthTexture && !(bBind2D ^ bBindRECT ^ bBindCUBE))
    {
        SG_LOG(SG_GL, SG_ALERT,
                "RenderTexture Warning: Depth and Color texture targets "
                "should match.");
        dbg_printf( "RenderTexture Warning: Depth and Color texture targets should match.\n");
    }

    // Apply default bit format if none specified
#ifdef _WIN32
    if (0 == _iNumComponents)
    {
        pfAttribs.push_back(WGL_RED_BITS_ARB);
        pfAttribs.push_back(8);
        pfAttribs.push_back(WGL_GREEN_BITS_ARB);
        pfAttribs.push_back(8);
        pfAttribs.push_back(WGL_BLUE_BITS_ARB);
        pfAttribs.push_back(8);
        pfAttribs.push_back(WGL_ALPHA_BITS_ARB);
        pfAttribs.push_back(8);
        _iNumComponents = 4;
    }
#endif

    // Depth bits
    if (_bIsDepthTexture && !iDepthBits)
        iDepthBits = 24;

#ifdef _WIN32
    pfAttribs.push_back(WGL_DEPTH_BITS_ARB);
#elif defined( __MACH__ )
# if 0
    pfAttribs.push_back(AGL_DEPTH_SIZE);
# else
    pfAttribs.push_back(kCGLPFADepthSize);
# endif
#else
    pfAttribs.push_back(GLX_DEPTH_SIZE);
#endif
    pfAttribs.push_back(iDepthBits); // default
    
    if (!bHasStencil)
    {
#ifdef _WIN32
        pfAttribs.push_back(WGL_STENCIL_BITS_ARB);
        pfAttribs.push_back(0);
#elif defined( __MACH__ )
# if 0
        pfAttribs.push_back(AGL_STENCIL_SIZE);
# else
        pfAttribs.push_back(kCGLPFAStencilSize);
# endif
        pfAttribs.push_back(0);
#else
        pfAttribs.push_back(GLX_STENCIL_SIZE);
        pfAttribs.push_back(0);
#endif

    }
    if (_iNumComponents < 4)
    {
        // Can't do this right now -- on NVIDIA drivers, currently get 
        // a non-functioning pbuffer if ALPHA_BITS=0 and 
        // WGL_BIND_TO_TEXTURE_RGB_ARB=true
        
        //pfAttribs.push_back(WGL_ALPHA_BITS_ARB); 
        //pfAttribs.push_back(0);
    }

#ifdef _WIN32
    if (!WGL_NV_render_depth_texture && _bIsDepthTexture && (RT_RENDER_TO_TEXTURE == _eUpdateMode))
    {
#if defined(DEBUG) || defined(_DEBUG)
        SG_LOG(SG_GL, SG_ALERT, "RenderTexture Warning: No support found for "
                "render to depth texture.");
#endif
        dbg_printf("RenderTexture Warning: No support found for render to depth texture.\n");
        _bIsDepthTexture = false;
    }
#endif
    
    if ((_bIsTexture || _bIsDepthTexture) && 
        (RT_RENDER_TO_TEXTURE == _eUpdateMode))
    {
#ifdef _WIN32                   
        if (bBindRECT)
        {
            pbAttribs.push_back(WGL_TEXTURE_TARGET_ARB);
            pbAttribs.push_back(WGL_TEXTURE_RECTANGLE_NV);
        }
        else if (bBindCUBE)
        {
            pbAttribs.push_back(WGL_TEXTURE_TARGET_ARB);
            pbAttribs.push_back(WGL_TEXTURE_CUBE_MAP_ARB);
        }
        else if (bBind2D)
        {
            pbAttribs.push_back(WGL_TEXTURE_TARGET_ARB);
            pbAttribs.push_back(WGL_TEXTURE_2D_ARB);
        }
            
        if (_bMipmap)
        {
            pbAttribs.push_back(WGL_MIPMAP_TEXTURE_ARB);
            pbAttribs.push_back(true);
        }
#elif defined(__MACH__)
#elif defined(DEBUG) || defined(_DEBUG)
        SG_LOG(SG_GL, SG_INFO, "RenderTexture Error: Render to Texture not "
               "supported in Linux or MacOS");
#endif  
    }

    // Set the pixel type
    if (_bFloat)
    {
#ifdef _WIN32
        if (WGL_NV_float_buffer)
        {
            pfAttribs.push_back(WGL_PIXEL_TYPE_ARB);
            pfAttribs.push_back(WGL_TYPE_RGBA_ARB);

            pfAttribs.push_back(WGL_FLOAT_COMPONENTS_NV);
            pfAttribs.push_back(true);
        }
        else
        {
            pfAttribs.push_back(WGL_PIXEL_TYPE_ARB);
            pfAttribs.push_back(WGL_TYPE_RGBA_FLOAT_ATI);
        }
#elif defined( __MACH__ )
        // if (GL_MACH_float_pixels) // FIXME
        {
            pfAttribs.push_back(kCGLPFAColorFloat);
        }
#else
        if (GLX_NV_float_buffer)
        {
            pfAttribs.push_back(GLX_FLOAT_COMPONENTS_NV);
            pfAttribs.push_back(1);
        }
#endif
    }
    else
    {
#ifdef _WIN32
        pfAttribs.push_back(WGL_PIXEL_TYPE_ARB);
        pfAttribs.push_back(WGL_TYPE_RGBA_ARB);
#endif
    }

    // Set up texture binding for render to texture
    if (_bIsTexture && (RT_RENDER_TO_TEXTURE == _eUpdateMode))
    {
        
#ifdef _WIN32
        if (_bFloat)
        {
            if (WGL_NV_float_buffer)
            {
                switch(_iNumComponents)
                {
                case 1:
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV);
                    pfAttribs.push_back(true);
                    
                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_FLOAT_R_NV);
                    break;
                case 2:
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV);
                    pfAttribs.push_back(true);
                    
                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_FLOAT_RG_NV);
                    break;
                case 3:
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV);
                    pfAttribs.push_back(true);
                    
                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_FLOAT_RGB_NV);
                    break;
                case 4:
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV);
                    pfAttribs.push_back(true);
                    
                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_FLOAT_RGBA_NV);
                    break;
                default:
                    SG_LOG(SG_GL, SG_ALERT, 
                            "RenderTexture Warning: Bad number of components "
                            "(r=1,rg=2,rgb=3,rgba=4): " <<
                            _iNumComponents);
                    dbg_printf("RenderTexture Warning 1: Bad number of components (r=1,rg=2,rgb=3,rgba=4): %d\n",
                            _iNumComponents);
                    break;
                }
            }
            else
            {
                if (4 == _iNumComponents)
                {
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RGBA_ARB);
                    pfAttribs.push_back(true);
                    
                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_RGBA_ARB);
                }
                else 
                {
                    // standard ARB_render_texture only supports 3 or 4 channels
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RGB_ARB);
                    pfAttribs.push_back(true);
                    
                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_RGB_ARB);
                }
            }
            
        } 
        else
        {
            switch(_iNumComponents)
            {
            case 3:
                pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RGB_ARB);
                pfAttribs.push_back(true);
                
                pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                pbAttribs.push_back(WGL_TEXTURE_RGB_ARB);
                break;
            case 4:
                pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RGBA_ARB);
                pfAttribs.push_back(true);
                
                pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                pbAttribs.push_back(WGL_TEXTURE_RGBA_ARB);
                break;
            default:
                SG_LOG(SG_GL, SG_ALERT, 
                        "RenderTexture Warning: Bad number of components "
                        "(r=1,rg=2,rgb=3,rgba=4): " << _iNumComponents);
                dbg_printf("RenderTexture Warning 2: Bad number of components (r=1,rg=2,rgb=3,rgba=4): %d\n",
                            _iNumComponents);
                break;
            }
        }
#elif defined(__MACH__)
        if (_bFloat)
        {
                       /*
            //if (WGLEW_NV_float_buffer)       // FIXME
            {
                switch(_iNumComponents)
                {
                case 1:
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV);
                    pfAttribs.push_back(true);

                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_FLOAT_R_NV);
                    break;
                case 2:
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV);
                    pfAttribs.push_back(true);

                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_FLOAT_RG_NV);
                    break;
                case 3:
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV);
                    pfAttribs.push_back(true);

                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_FLOAT_RGB_NV);
                    break;
                case 4:
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV);
                    pfAttribs.push_back(true);

                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_FLOAT_RGBA_NV);
                    break;
                default:
                    fprintf(stderr,
                            "RenderTexture Warning: Bad number of components "
                            "(r=1,rg=2,rgb=3,rgba=4): %d.\n",
                            _iNumComponents);
                    break;
                }
           }
            else
            {
                if (4 == _iNumComponents)
                {
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RGBA_ARB);
                    pfAttribs.push_back(true);

                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_RGBA_ARB);
                }
                else
                {
                    // standard ARB_render_texture only supports 3 or 4 channels
                    pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RGB_ARB);
                    pfAttribs.push_back(true);

                    pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                    pbAttribs.push_back(WGL_TEXTURE_RGB_ARB);
                }
            }
            */
        }
        else
        {
                       /*
            switch(_iNumComponents)
            {
            case 3:
                pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RGB_ARB);
                pfAttribs.push_back(true);

                pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                pbAttribs.push_back(WGL_TEXTURE_RGB_ARB);
                break;
            case 4:
                pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RGBA_ARB);
                pfAttribs.push_back(true);

                pbAttribs.push_back(WGL_TEXTURE_FORMAT_ARB);
                pbAttribs.push_back(WGL_TEXTURE_RGBA_ARB);
                break;
            default:
                fprintf(stderr,
                        "RenderTexture Warning: Bad number of components "
                        "(r=1,rg=2,rgb=3,rgba=4): %d.\n", _iNumComponents);
                break;
            }
                       */
        }
#elif defined(DEBUG) || defined(_DEBUG)
        SG_LOG(SG_GL, SG_ALERT, 
                "RenderTexture Error: Render to Texture not supported in Linux or MacOS");
#endif  
      dbg_printf( "RenderTexture Error 1: Render to Texture not supported in Linux or MacOS\n");
    }
        
    if (_bIsDepthTexture && (RT_RENDER_TO_TEXTURE == _eUpdateMode))
    {  
#ifdef _WIN32
        if (_bRectangle)
        {
            pfAttribs.push_back(WGL_BIND_TO_TEXTURE_RECTANGLE_DEPTH_NV);
            pfAttribs.push_back(true);
        
            pbAttribs.push_back(WGL_DEPTH_TEXTURE_FORMAT_NV);
            pbAttribs.push_back(WGL_TEXTURE_DEPTH_COMPONENT_NV);
        }
        else 
        {
            pfAttribs.push_back(WGL_BIND_TO_TEXTURE_DEPTH_NV);
            pfAttribs.push_back(true);
        
            pbAttribs.push_back(WGL_DEPTH_TEXTURE_FORMAT_NV);
            pbAttribs.push_back(WGL_TEXTURE_DEPTH_COMPONENT_NV);
        }
#elif defined(__MACH__)
#elif defined(DEBUG) || defined(_DEBUG)
        SG_LOG(SG_GL, SG_INFO, "RenderTexture Error: Render to Texture not supported in "
               "Linux or MacOS");
#endif 
      dbg_printf( "RenderTexture Error 2: Render to Texture not supported in Linux or MacOS\n");
    }
    dbg_printf("RenderTexture::_ParseModeString(%s). END vf=%d vp=%d\n", modeString, (int)pfAttribs.size(), (int)pbAttribs.size() );

}

//---------------------------------------------------------------------------
// Function     	: RenderTexture::_GetKeyValuePair
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_GetKeyValuePair()
* @brief Parses expressions of the form "X=Y" into a pair (X,Y).
*/ 
RenderTexture::KeyVal RenderTexture::_GetKeyValuePair(string token)
{
    string::size_type pos = 0;
    if ((pos = token.find("=")) != token.npos)
    {
        string key = token.substr(0, pos);
        string value = token.substr(pos+1, token.length()-pos+1);
        return KeyVal(key, value);
    }
    else
        return KeyVal(token, "");
}

//---------------------------------------------------------------------------
// Function     	: RenderTexture::_ParseBitVector
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_ParseBitVector()
* @brief Parses expressions of the form "=r,g,b,a" into a vector: (r,g,b,a)
*/ 
vector<int> RenderTexture::_ParseBitVector(string bitVector)
{
    vector<string> pieces;
    vector<int> bits;

    if (bitVector == "")
    {
        bits.push_back(8);  // if a depth isn't specified, use default 8 bits
        return bits;
    }

    string::size_type pos = 0; 
    string::size_type nextpos = 0;
    do
    { 
        nextpos = bitVector.find_first_of(", ", pos);
        pieces.push_back(string(bitVector, pos, nextpos - pos)); 
        pos = nextpos+1; 
    } while (nextpos != bitVector.npos );

    for ( vector<string>::iterator it = pieces.begin(); it != pieces.end(); it++)
    {
        bits.push_back(strtol(it->c_str(), 0, 10));
    }
    
    return bits;
}

//---------------------------------------------------------------------------
// Function     	: RenderTexture::_VerifyExtensions
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_VerifyExtensions()
* @brief Checks that the necessary extensions are available based on RT mode.
*/ 
bool RenderTexture::_VerifyExtensions()
{
   dbg_printf("RenderTexture::_VerifyExtensions() called...\n");
#ifdef _WIN32
	// a second call to _VerifyExtensions will allways return true, causing a crash
	// if the extension is not supported
    if ( true || !fctPtrInited )
    {
        fctPtrInited = true;
        wglGetExtensionsStringARBProc wglGetExtensionsStringARBPtr = (wglGetExtensionsStringARBProc)wglGetProcAddress( "wglGetExtensionsStringARB" );
        if ( wglGetExtensionsStringARBPtr == 0 )
        {
            PrintExtensionError("WGL_ARB_extensions_string");
            return false;
        }
        string wglExtensionsString = wglGetExtensionsStringARBPtr( wglGetCurrentDC() );
        if ( SGSearchExtensionsString( wglExtensionsString.c_str(), "WGL_ARB_pixel_format" ) )
        {
            wglChoosePixelFormatARBPtr = (wglChoosePixelFormatARBProc)SGLookupFunction("wglChoosePixelFormatARB");
            wglGetPixelFormatAttribivARBPtr = (wglGetPixelFormatAttribivARBProc)SGLookupFunction("wglGetPixelFormatAttribivARB");
        }
        else
        {
            PrintExtensionError("WGL_ARB_pixel_format");
            return false;
        }
        if ( SGSearchExtensionsString( wglExtensionsString.c_str(), "WGL_ARB_pbuffer" ) )
        {
            wglCreatePbufferARBPtr = (wglCreatePbufferARBProc)SGLookupFunction("wglCreatePbufferARB");
            wglGetPbufferDCARBPtr = (wglGetPbufferDCARBProc)SGLookupFunction("wglGetPbufferDCARB");
            wglQueryPbufferARBPtr = (wglQueryPbufferARBProc)SGLookupFunction("wglQueryPbufferARB");
            wglReleasePbufferDCARBPtr = (wglReleasePbufferDCARBProc)SGLookupFunction("wglReleasePbufferDCARB");
            wglDestroyPbufferARBPtr = (wglDestroyPbufferARBProc)SGLookupFunction("wglDestroyPbufferARB");
        }
        else
        {
            PrintExtensionError("WGL_ARB_pbuffer");
            return false;
        }
        if ( SGSearchExtensionsString( wglExtensionsString.c_str(), "WGL_ARB_render_texture" ) )
        {
            wglBindTexImageARBPtr = (wglBindTexImageARBProc)SGLookupFunction("wglBindTexImageARB");
            wglReleaseTexImageARBPtr = (wglReleaseTexImageARBProc)SGLookupFunction("wglReleaseTexImageARB");
        }
        else if ( _bIsTexture )
        {
            PrintExtensionError("WGL_ARB_render_texture");
            return false;
        }
        if (_bRectangle && !SGIsOpenGLExtensionSupported( "GL_NV_texture_rectangle" ))
        {
            PrintExtensionError("GL_NV_texture_rectangle");
            return false;
        }
        if (_bFloat && !(SGIsOpenGLExtensionSupported( "GL_NV_float_buffer" ) ||
            SGSearchExtensionsString( wglExtensionsString.c_str(), "WGL_ATI_pixel_format_float" )))
        {
            PrintExtensionError("GL_NV_float_buffer or GL_ATI_pixel_format_float");
            return false;
        
        }
        if (_bFloat && _bIsTexture && !(SGIsOpenGLExtensionSupported( "GL_NV_float_buffer" ) || SGIsOpenGLExtensionSupported( "GL_ATI_texture_float" )))
        {
            PrintExtensionError("NV_float_buffer or ATI_texture_float");
        }
        if (_bIsDepthTexture && !SGIsOpenGLExtensionSupported( "GL_ARB_depth_texture" ))
        {
            // [Redge]
#if defined(_DEBUG) | defined(DEBUG)
            SG_LOG(SG_GL, SG_ALERT, 
                    "RenderTexture Warning: "
                    "OpenGL extension GL_ARB_depth_texture not available."
                    "         Using glReadPixels() to emulate behavior.");
#endif   
            _bHasARBDepthTexture = false;
            //PrintExtensionError("GL_ARB_depth_texture");
            //return false;
            // [/Redge]
        }
        SetLastError(0);
    }
#elif defined( __MACH__ )
    // FIXME! Check extensions!
#else
    Display* dpy = glXGetCurrentDisplay();
    int minor = 0, major = 0;
    if (!dpy) {
        dbg_printf("_VerifyExtensions: glXGetCurrentDisplay() returned NULL! returning false\n");
        return false;
    }
    if (!glXQueryVersion(dpy, &major, &minor)) 
    {
        dbg_printf("_VerifyExtensions: glXQueryVersion(dpy, &major, &minor) FAILED! returning false\n");
        return false;
    }
    else
    {
        dbg_printf("_VerifyExtensions: glXQueryVersion(dpy, major=%d, minor=%d)\n", major, minor);
    }

    int screen = DefaultScreen(dpy);
    const char* extString = glXQueryExtensionsString(dpy, screen);
    dbg_printf("_VerifyExtensions: glXQueryExtensionsString(dpy, screen) returned -\n[%s]\n",
      (extString ? extString : "<NULL>") );
    if (!SGSearchExtensionsString(extString, "GLX_SGIX_fbconfig") ||
        !SGSearchExtensionsString(extString, "GLX_SGIX_pbuffer"))
    {
        dbg_printf("_VerifyExtensions: glXQueryExtensionsString(dpy,screen) does NOT contain GLX_SGIX_fbconfig or GLX_SGIX_pbuffer!\n" );
        const char * extClient = glXGetClientString( dpy, GLX_EXTENSIONS );
        if (!extClient ||
            !SGSearchExtensionsString(extClient, "GLX_SGIX_fbconfig") ||
            !SGSearchExtensionsString(extClient, "GLX_SGIX_pbuffer"))
        {
            dbg_printf("_VerifyExtensions: AND glXGetClientString(dpy,GLX_EXTENSIONS) also! returning false\n" );
            return false;
        }
        else
        {
            dbg_printf("_VerifyExtensions: BUT glXGetClientString(dpy,GLX_EXTENSIONS) returned \n[%s]\n", extClient );
            dbg_printf("Since this DOES contain fbconfig and pbuffer, continuing...\n");
        }
    }
    // First try the glX version 1.3 functions.
    glXChooseFBConfigPtr = (glXChooseFBConfigProc)SGLookupFunction("glXChooseFBConfig");
    glXCreatePbufferPtr = (glXCreatePbufferProc)SGLookupFunction("glXCreatePbuffer");
    glXGetVisualFromFBConfigPtr = (glXGetVisualFromFBConfigProc)SGLookupFunction("glXGetVisualFromFBConfig");
    glXCreateContextPtr = (glXCreateContextProc)SGLookupFunction("glXCreateContext");
    glXDestroyPbufferPtr = (glXDestroyPbufferProc)SGLookupFunction("glXDestroyPbuffer");
    glXQueryDrawablePtr = (glXQueryDrawableProc)SGLookupFunction("glXQueryDrawable");

    if (((1 <= major && 3 <= minor) || 2 <= major) &&
        glXChooseFBConfigPtr &&
        glXCreatePbufferPtr &&
        glXGetVisualFromFBConfigPtr &&
        glXCreateContextPtr &&
        glXDestroyPbufferPtr &&
        glXQueryDrawablePtr) {
        dbg_printf("_VerifyExtensions: setting glXVersion1_3Present TRUE\n" );
        glXVersion1_3Present = true;
    }
    else
    {
        dbg_printf("_VerifyExtensions: setting glXVersion1_3Present FALSE\n Reason: " );
        if ( !((1 <= major && 3 <= minor) || 2 <= major) ) { dbg_printf( "Version wrong %d.%d ", major, minor ); }
        if ( !glXChooseFBConfigPtr ) { dbg_printf( "missing glXChooseFBConfigPtr " ); }
        if ( !glXCreatePbufferPtr ) { dbg_printf( "missing glXCreatePbufferPtr " ); }
        if ( !glXGetVisualFromFBConfigPtr ) { dbg_printf( "missing glXGetVisualFromFBConfigPtr " ); }
        if ( !glXCreateContextPtr ) { dbg_printf( "missing glXCreateContextPtr " ); }
        if ( !glXDestroyPbufferPtr ) { dbg_printf( "missing glXDestroyPbufferPtr " ); }
        if ( !glXQueryDrawablePtr ) { dbg_printf( "missing glXQueryDrawablePtr " ); }
        dbg_printf("\n");
        
        glXChooseFBConfigPtr = (glXChooseFBConfigProc)SGLookupFunction("glXChooseFBConfigSGIX");
        glXCreateGLXPbufferPtr = (glXCreateGLXPbufferProc)SGLookupFunction("glXCreateGLXPbufferSGIX");
        glXGetVisualFromFBConfigPtr =  (glXGetVisualFromFBConfigProc)SGLookupFunction("glXGetVisualFromFBConfigSGIX");
        glXCreateContextWithConfigPtr = (glXCreateContextWithConfigProc)SGLookupFunction("glXCreateContextWithConfigSGIX");
        glXDestroyPbufferPtr = (glXDestroyPbufferProc)SGLookupFunction("glXDestroyGLXPbufferSGIX");
        glXQueryGLXPbufferSGIXPtr = (glXQueryGLXPbufferSGIXProc)SGLookupFunction("glXQueryGLXPbufferSGIX");

        if (!glXChooseFBConfigPtr ||
            !glXCreateGLXPbufferPtr ||
            !glXGetVisualFromFBConfigPtr ||
            !glXCreateContextWithConfigPtr ||
            !glXDestroyPbufferPtr ||
            !glXQueryGLXPbufferSGIXPtr)
            {
               dbg_printf("_VerifyExtensions: some pointer is NULL! return false\n" );
               if ( !glXCreateGLXPbufferPtr ) {
                  dbg_printf("RenderTexture::Initialize: ERROR glXCreateGLXPbufferPtr = NULL!\n");
               } else if ( !glXCreateContextWithConfigPtr ) {
                  dbg_printf("RenderTexture::Initialize: ERROR glXCreateContextWithConfigPtr = NULL!\n");
               } else if ( !glXVersion1_3Present && !glXQueryGLXPbufferSGIXPtr ) {
                  dbg_printf("RenderTexture::Initialize: ERROR glXQueryGLXPbufferSGIXPtr = NULL!\n");
               }
               return false;
            } else {
               dbg_printf("_VerifyExtensions: appear to have all 'procs' glXCreateGLXPbufferPtr=[%p]\n", glXCreateGLXPbufferPtr );
            }
    }

    if (_bIsDepthTexture && !GL_ARB_depth_texture)
    {
        PrintExtensionError("GL_ARB_depth_texture");
        return false;
    }
    if (_bFloat && _bIsTexture && !GLX_NV_float_buffer)
    {
        PrintExtensionError("GLX_NV_float_buffer");
        return false;
    }
    if (_eUpdateMode == RT_RENDER_TO_TEXTURE)
    {
        PrintExtensionError("Some GLX render texture extension: Please implement me!");
        return false;
    }
#endif
  
    dbg_printf("RenderTexture::_VerifyExtensions: return true.\n");
    return true;
}

//---------------------------------------------------------------------------
// Function     	: RenderTexture::_InitializeTextures
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_InitializeTextures()
* @brief Initializes the state of textures used by the RenderTexture.
*/ 
bool RenderTexture::_InitializeTextures()
{
   dbg_printf( "RenderTexture::_InitializeTextures() called\n" );
    // Determine the appropriate texture formats and filtering modes.
    
    if (_bIsTexture || _bIsDepthTexture)
    {
        if (_bRectangle && GL_NV_texture_rectangle)
            _iTextureTarget = GL_TEXTURE_RECTANGLE_NV;
        else if (_bRectangle && GL_EXT_texture_rectangle)
            _iTextureTarget = GL_TEXTURE_RECTANGLE_EXT;
        else
            _iTextureTarget = GL_TEXTURE_2D;
    }

    if (_bIsTexture)
    {
        glGenTextures(1, (GLuint*)&_iTextureID);
        glBindTexture(_iTextureTarget, _iTextureID);  
        
        // Use clamp to edge as the default texture wrap mode for all tex
        glTexParameteri(_iTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(_iTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
        // Use NEAREST as the default texture filtering mode.
        glTexParameteri(_iTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(_iTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


        if (RT_COPY_TO_TEXTURE == _eUpdateMode)
        {
            GLuint iInternalFormat;
            GLuint iFormat;

            if (_bFloat)
            {                             
                if (_bMipmap)
                {
                    SG_LOG(SG_GL, SG_ALERT, 
                        "RenderTexture Error: mipmapped float textures not "
                        "supported.");
                    dbg_printf( "RenderTexture Error: mipmapped float textures not supported. return false\n");
                    return false;
                }
            
                switch(_iNumComponents) 
                {
                case 1:
                    if (GL_NV_float_buffer)
                    {
                        iInternalFormat = (_iNumColorBits[0] > 16) ? 
                            GL_FLOAT_R32_NV : GL_FLOAT_R16_NV;
                    }
                    else if (GL_ATI_texture_float)
                    {
                        iInternalFormat = (_iNumColorBits[0] > 16) ? 
                            GL_LUMINANCE_FLOAT32_ATI : 
                            GL_LUMINANCE_FLOAT16_ATI;
                    }
                    iFormat = GL_LUMINANCE;
                	break;
                case 2:
                    if (GL_NV_float_buffer)
                    {
                        iInternalFormat = (_iNumColorBits[0] > 16) ? 
                            GL_FLOAT_RG32_NV : GL_FLOAT_RG16_NV;
                    }
                    else if (GL_ATI_texture_float)
                    {
                        iInternalFormat = (_iNumColorBits[0] > 16) ? 
                            GL_LUMINANCE_ALPHA_FLOAT32_ATI : 
                            GL_LUMINANCE_ALPHA_FLOAT16_ATI;
                    }
                    iFormat = GL_LUMINANCE_ALPHA;
                	break;
                case 3:
                    if (GL_NV_float_buffer)
                    {
                        iInternalFormat = (_iNumColorBits[0] > 16) ? 
                            GL_FLOAT_RGB32_NV : GL_FLOAT_RGB16_NV;
                    }
                    else if (GL_ATI_texture_float)
                    {
                        iInternalFormat = (_iNumColorBits[0] > 16) ? 
                            GL_RGB_FLOAT32_ATI : GL_RGB_FLOAT16_ATI;
                    }
                    iFormat = GL_RGB;
                    break;
                case 4:
                    if (GL_NV_float_buffer)
                    {
                        iInternalFormat = (_iNumColorBits[0] > 16) ? 
                            GL_FLOAT_RGBA32_NV : GL_FLOAT_RGBA16_NV;
                    }
                    else if (GL_ATI_texture_float)
                    {
                        iInternalFormat = (_iNumColorBits[0] > 16) ? 
                            GL_RGBA_FLOAT32_ATI : GL_RGBA_FLOAT16_ATI;
                    }
                    iFormat = GL_RGBA;
                    break;
                default:
                    SG_LOG(SG_GL, SG_INFO, "RenderTexture Error: "
                           "Invalid number of components: " <<
                           _iNumComponents);
                    dbg_printf( "RenderTexture Error: Invalid number of components: %d - return false\n",
                        _iNumComponents);
                    return false;
                }
            }
            else // non-float
            {                        
                if (4 == _iNumComponents)
                {
                    iInternalFormat = GL_RGBA8;
                    iFormat = GL_RGBA;
                }
                else 
                {
                    iInternalFormat = GL_RGB8;
                    iFormat = GL_RGB;
                }
            }
        
            // Allocate the texture image (but pass it no data for now).
            glTexImage2D(_iTextureTarget, 0, iInternalFormat, 
                         _iWidth, _iHeight, 0, iFormat, GL_FLOAT, NULL);
        } 
    }
  
    if (_bIsDepthTexture)
    { 
        glGenTextures(1, (GLuint*)&_iDepthTextureID);
        glBindTexture(_iTextureTarget, _iDepthTextureID);  
        
        // Use clamp to edge as the default texture wrap mode for all tex
        glTexParameteri(_iTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(_iTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
        // Use NEAREST as the default texture filtering mode.
        glTexParameteri(_iTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(_iTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
               
        if (RT_COPY_TO_TEXTURE == _eUpdateMode)
        {
            // [Redge]
            if (_bHasARBDepthTexture) 
            {
                // Allocate the texture image (but pass it no data for now).
                glTexImage2D(_iTextureTarget, 0, GL_DEPTH_COMPONENT, 
                             _iWidth, _iHeight, 0, GL_DEPTH_COMPONENT, 
                             GL_FLOAT, NULL);
            } 
            else 
            {
                // allocate memory for depth texture
                // Since this is slow, we warn the user in debug mode. (above)
                _pPoorDepthTexture = new unsigned short[_iWidth * _iHeight];
                glTexImage2D(_iTextureTarget, 0, GL_LUMINANCE16, 
                             _iWidth, _iHeight, 0, GL_LUMINANCE, 
                             GL_UNSIGNED_SHORT, _pPoorDepthTexture);
            }
            // [/Redge]
        }
    }

    dbg_printf( "RenderTexture::_InitializeTextures() returning true\n" );
    return true;
}


//---------------------------------------------------------------------------
// Function     	: RenderTexture::_MaybeCopyBuffer
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_MaybeCopyBuffer()
* @brief Does the actual copying for RenderTextures with RT_COPY_TO_TEXTURE
*/ 
void RenderTexture::_MaybeCopyBuffer()
{
#ifdef _WIN32
    if (RT_COPY_TO_TEXTURE == _eUpdateMode)
    {
        if (_bIsTexture)
        {
            glBindTexture(_iTextureTarget, _iTextureID);
            glCopyTexSubImage2D(_iTextureTarget, 
                                0, 0, 0, 0, 0, _iWidth, _iHeight);
        }
        if (_bIsDepthTexture)
        {
            glBindTexture(_iTextureTarget, _iDepthTextureID);
            // HOW TO COPY DEPTH TEXTURE??? Supposedly this just magically works...
            // [Redge]
            if (_bHasARBDepthTexture) 
            {
                glCopyTexSubImage2D(_iTextureTarget, 0, 0, 0, 0, 0, 
                                    _iWidth, _iHeight);
            } 
            else 
            {
                // no 'real' depth texture available, so behavior has to be emulated
                // using glReadPixels (beware, this is (naturally) slow ...)
                glReadPixels(0, 0, _iWidth, _iHeight, GL_DEPTH_COMPONENT, 
                             GL_UNSIGNED_SHORT, _pPoorDepthTexture);
                glTexImage2D(_iTextureTarget, 0, GL_LUMINANCE16,
                             _iWidth, _iHeight, 0, GL_LUMINANCE, 
                             GL_UNSIGNED_SHORT, _pPoorDepthTexture);
            }
            // [/Redge]
        }
    }
    
#elif defined(__MACH__)
    if (RT_COPY_TO_TEXTURE == _eUpdateMode)
    {
       if (_bIsTexture)
       {
         glBindTexture(_iTextureTarget, _iTextureID);
         glCopyTexSubImage2D(_iTextureTarget, 0, 0, 0, 0, 0, _iWidth, _iHeight);
       }
       if (_bIsDepthTexture)
       {
         glBindTexture(_iTextureTarget, _iDepthTextureID);
         assert(_bHasARBDepthTexture);
         glCopyTexSubImage2D(_iTextureTarget, 0, 0, 0, 0, 0, _iWidth, _iHeight);
       }
    }
#else
    if (_bIsTexture)
    {
      glBindTexture(_iTextureTarget, _iTextureID);
      glCopyTexSubImage2D(_iTextureTarget, 0, 0, 0, 0, 0, _iWidth, _iHeight);
    }
    if (_bIsDepthTexture)
    {
      glBindTexture(_iTextureTarget, _iDepthTextureID);
      assert(_bHasARBDepthTexture);
      glCopyTexSubImage2D(_iTextureTarget, 0, 0, 0, 0, 0, _iWidth, _iHeight);
    }
#endif

}

//---------------------------------------------------------------------------
// Function     	: RenderTexture::_ReleaseBoundBuffers
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_ReleaseBoundBuffers()
* @brief Releases buffer bindings on RenderTextures with RT_RENDER_TO_TEXTURE
*/ 
bool RenderTexture::_ReleaseBoundBuffers()
{
#ifdef _WIN32
    if (_bIsTexture && RT_RENDER_TO_TEXTURE == _eUpdateMode)
    {
        glBindTexture(_iTextureTarget, _iTextureID);
        
        // release the pbuffer from the render texture object
        if (0 != _iCurrentBoundBuffer && _bIsBufferBound)
        {
            if (FALSE == wglReleaseTexImageARBPtr(_hPBuffer, _iCurrentBoundBuffer))
            {
                _wglGetLastError();
                return false;
            }
            _bIsBufferBound = false;
        }
    }
    
    if (_bIsDepthTexture && RT_RENDER_TO_TEXTURE == _eUpdateMode)
    {
        glBindTexture(_iTextureTarget, _iDepthTextureID);
        
        // release the pbuffer from the render texture object
        if (FALSE == wglReleaseTexImageARBPtr(_hPBuffer, WGL_DEPTH_COMPONENT_NV))
        {
            _wglGetLastError();
            return false;
        }
    }
    
#else
    // textures can't be bound in Linux
#endif
    return true;
}

//---------------------------------------------------------------------------
// Function     	: RenderTexture::_MakeCurrent
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::_MakeCurrent()
* @brief Makes the RenderTexture's context current
*/ 
bool RenderTexture::_MakeCurrent() 
{
#ifdef _WIN32
    // make the pbuffer's rendering context current.
    if (FALSE == wglMakeCurrent( _hDC, _hGLContext))
    {
        _wglGetLastError();
        return false;
    }
#elif defined( __MACH__ )
    CGLError err;

    if (err = CGLSetCurrentContext(_hGLContext))
    {
        _cglCheckError(err);
        return false;
    }
#else
static GLXContext last_hGLContext = 0;
    if (false == glXMakeCurrent(_pDisplay, _hPBuffer, _hGLContext)) 
    {
        dbg_printf( "_MakeCurrent: glXMakeCurrent FAILED! returning false\n");
        return false;
    }

    if ( last_hGLContext != _hGLContext ) {
      last_hGLContext = _hGLContext;
      dbg_printf( "_MakeCurrent: glXMakeCurrent set to [%p] SUCCESS! returning true\n", _hGLContext );
    }
#endif
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
// Begin Deprecated Interface
//
/////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
// Function      : RenderTexture::RenderTexture
// Description	 : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::RenderTexture()
* @brief Constructor.
*/ 
RenderTexture::RenderTexture(int width, int height,
                               bool bIsTexture /* = true */,
                               bool bIsDepthTexture /* = false */)
:   _iWidth(width), 
    _iHeight(height), 
    _bIsTexture(bIsTexture),
    _bIsDepthTexture(bIsDepthTexture),
    _bHasARBDepthTexture(true),            // [Redge]
    _eUpdateMode(RT_RENDER_TO_TEXTURE),
    _bInitialized(false),
    _iNumAuxBuffers(0),
    _iCurrentBoundBuffer(0),
    _iNumDepthBits(0),
    _iNumStencilBits(0),
    _bFloat(false),
    _bDoubleBuffered(false),
    _bPowerOf2(true),
    _bRectangle(false),
    _bMipmap(false),
    _bShareObjects(false),
    _bCopyContext(false),
#ifdef _WIN32
    _hDC(NULL), 
    _hGLContext(NULL), 
    _hPBuffer(NULL),
    _hPreviousDC(0),
    _hPreviousContext(0),
#elif defined( __MACH__ )
    _hGLContext(NULL),
    _hPBuffer(NULL),
    _hPreviousContext(NULL),
#else
    _pDisplay(NULL),
    _hGLContext(NULL),
    _hPBuffer(0),
    _hPreviousDrawable(0),
    _hPreviousContext(0),
#endif
    _iTextureTarget(GL_NONE),
    _iTextureID(0),
    _iDepthTextureID(0),
    _pPoorDepthTexture(0) // [Redge]
{
    assert(width > 0 && height > 0);
#if defined DEBUG || defined _DEBUG
    SG_LOG(SG_GL, SG_ALERT, 
            "RenderTexture Warning: Deprecated Contructor interface used.");
#endif
    dbg_printf("RenderTexture Warning: Deprecated Contructor interface used.\n");
    
    _iNumColorBits[0] = _iNumColorBits[1] = 
        _iNumColorBits[2] = _iNumColorBits[3] = 0;
    _bPowerOf2 = IsPowerOfTwo(width) && IsPowerOfTwo(height);
}

//------------------------------------------------------------------------------
// Function     	: RenderTexture::Initialize
// Description	    : 
//------------------------------------------------------------------------------
/**
* @fn RenderTexture::Initialize(bool bShare, bool bDepth, bool bStencil, bool bMipmap, unsigned int iRBits, unsigned int iGBits, unsigned int iBBits, unsigned int iABits);
* @brief Initializes the RenderTexture, sharing display lists and textures if specified.
* 
* This function actually does the creation of the p-buffer.  It can only be called 
* once a GL context has already been created.  Note that if the texture is not
* power of two dimensioned, or has more than 8 bits per channel, enabling mipmapping
* will cause an error.
*/ 
bool RenderTexture::Initialize(bool         bShare       /* = true */, 
                                bool         bDepth       /* = false */, 
                                bool         bStencil     /* = false */, 
                                bool         bMipmap      /* = false */, 
                                bool         bAnisoFilter /* = false */,
                                unsigned int iRBits       /* = 8 */, 
                                unsigned int iGBits       /* = 8 */, 
                                unsigned int iBBits       /* = 8 */, 
                                unsigned int iABits       /* = 8 */,
                                UpdateMode   updateMode   /* = RT_RENDER_TO_TEXTURE */)
{   
    if (0 == _iWidth || 0 == _iHeight)
        return false;

#if defined DEBUG || defined _DEBUG
    SG_LOG(SG_GL, SG_ALERT, 
            "RenderTexture Warning: Deprecated Initialize() interface used.");
#endif   
    dbg_printf("RenderTexture Warning: Deprecated Initialize() interface used.\n");

    // create a mode string.
    string mode = "";
    if (bDepth)
        mode.append("depth ");
    if (bStencil)
        mode.append("stencil ");
    if (bMipmap)
        mode.append("mipmap ");
    if (iRBits + iGBits + iBBits + iABits > 0)
    {
        if (iRBits > 0)
            mode.append("r");
        if (iGBits > 0)
            mode.append("g");
        if (iBBits > 0)
            mode.append("b");
        if (iABits > 0)
            mode.append("a");
        mode.append("=");
        char bitVector[100];
        snprintf( bitVector, 100,
            "%d%s,%d%s,%d%s,%d%s",
            iRBits, (iRBits >= 16) ? "f" : "",
            iGBits, (iGBits >= 16) ? "f" : "",
            iBBits, (iBBits >= 16) ? "f" : "",
            iABits, (iABits >= 16) ? "f" : "");
        mode.append(bitVector);
        mode.append(" ");
    }
    if (_bIsTexture)
    {
        if ((GL_NV_texture_rectangle || GL_EXT_texture_rectangle) && 
            ((!IsPowerOfTwo(_iWidth) || !IsPowerOfTwo(_iHeight))
              || iRBits >= 16 || iGBits > 16 || iBBits > 16 || iABits >= 16))
            mode.append("texRECT ");
        else
            mode.append("tex2D ");
    }
    if (_bIsDepthTexture)
    {
        if ((GL_NV_texture_rectangle || GL_EXT_texture_rectangle) && 
            ((!IsPowerOfTwo(_iWidth) || !IsPowerOfTwo(_iHeight))
              || iRBits >= 16 || iGBits > 16 || iBBits > 16 || iABits >= 16))
            mode.append("texRECT ");
        else
            mode.append("tex2D ");
    }
    if (RT_COPY_TO_TEXTURE == updateMode)
        mode.append("ctt");

    _pixelFormatAttribs.clear();
    _pbufferAttribs.clear();

#ifdef _WIN32
    _pixelFormatAttribs.push_back(WGL_DRAW_TO_PBUFFER_ARB);
    _pixelFormatAttribs.push_back(true);
    _pixelFormatAttribs.push_back(WGL_SUPPORT_OPENGL_ARB);
    _pixelFormatAttribs.push_back(true);
    
    _pbufferAttribs.push_back(WGL_PBUFFER_LARGEST_ARB);
    _pbufferAttribs.push_back(true);
#elif defined( __MACH__ )
    //_pixelFormatAttribs.push_back(kCGLPFANoRecovery);
    _pixelFormatAttribs.push_back(kCGLPFAAccelerated);
    _pixelFormatAttribs.push_back(kCGLPFAWindow);
    _pixelFormatAttribs.push_back(kCGLPFAPBuffer);
#else
    _pixelFormatAttribs.push_back(GLX_RENDER_TYPE_SGIX);
    _pixelFormatAttribs.push_back(GLX_RGBA_BIT_SGIX);
    _pixelFormatAttribs.push_back(GLX_DRAWABLE_TYPE_SGIX);
    _pixelFormatAttribs.push_back(GLX_PBUFFER_BIT_SGIX);
#endif

    _ParseModeString(mode.c_str(), _pixelFormatAttribs, _pbufferAttribs);

#ifdef _WIN32
    _pixelFormatAttribs.push_back(0);
    _pbufferAttribs.push_back(0);
#elif defined(__MACH__)
    _pixelFormatAttribs.push_back(0);
#else
    _pixelFormatAttribs.push_back(None);
#endif

    Initialize(_iWidth, _iHeight, bShare);
    
    return true;
}


//---------------------------------------------------------------------------
// Function     	: RenderTexture::Reset
// Description	    : 
//---------------------------------------------------------------------------
/**
* @fn RenderTexture::Reset(int iWidth, int iHeight, unsigned int iMode, bool bIsTexture, bool bIsDepthTexture)
* @brief Resets the resolution of the offscreen buffer.
* 
* Causes the buffer to delete itself.  User must call Initialize() again
* before use.
*/ 
bool RenderTexture::Reset(int iWidth, int iHeight)
{
    SG_LOG(SG_GL, SG_ALERT, 
            "RenderTexture Warning: Deprecated Reset() interface used.");
            
    dbg_printf("RenderTexture Warning: Deprecated Reset(x,y) interface used.\n");

    if (!_Invalidate())
    {
        SG_LOG(SG_GL, SG_ALERT, "RenderTexture::Reset(): failed to invalidate.");
        dbg_printf( "RenderTexture::Reset(x,y): failed to invalidate. returning false\n");
        return false;
    }
    _iWidth     = iWidth;
    _iHeight    = iHeight;
    
    dbg_printf( "RenderTexture::Reset(x,y): succeeded. returning true\n");
    return true;
}

#if defined( _DEBUG ) && !defined( _WIN32 ) && !defined( __MACH__ )
/* just some DEBUG ONLY code, to show the 'attributes' */

typedef struct tagPXATTS {
   int attr;
   const char * name;
   const char * desc;
   int def;
}PXATTS, * PPXATTS;

static PXATTS pxAtts[] = {
   { GLX_FBCONFIG_ID, "GLX_FBCONFIG_ID",
     "followed by a valid XID that indicates the desired GLX frame buffer configuration. "
     "When a GLX_FBCONFIG_ID is specified, all attributes are ignored. The default value is GLX_DONT_CARE.",
     GLX_DONT_CARE },
   { GLX_BUFFER_SIZE, "GLX_BUFFER_SIZE",
   "Must be followed by a nonnegative integer that indicates the desired color index buffer size."
   "The smallest index buffer of at least the specified size is preferred. This attribute is ignored if GLX_COLOR_INDEX_BIT is not set "
   "in GLX_RENDER_TYPE. The default value is 0.",
    0 },
   { GLX_LEVEL, "GLX_LEVEL",
   "Must be followed by an integer buffer-level specification. This specification is honored exactly."
   "Buffer level 0 corresponds to the default frame buffer of the display. "
   "Buffer level 1 is the first overlay frame buffer, level two the second overlay frame buffer, and so on."
   "Negative buffer levels correspond to underlay frame buffers. The default value is 0.",
    0 },
   { GLX_DOUBLEBUFFER, "GLX_DOUBLEBUFFER",
   "Must be followed by True or False. If True is specified, then only double-buffered frame buffer configurations are considered;"
   "if False is specified, then only single-buffered frame buffer configurations are considered. The default value is GLX_DONT_CARE.",
    GLX_DONT_CARE },
   { GLX_STEREO, "GLX_STEREO",
   "Must be followed by True or False. If True is specified, then only stereo frame buffer configurations are considered;"
   " if False is specified, then only monoscopic frame buffer configurations are considered. The default value is False.",
   False },
   { GLX_AUX_BUFFERS, "GLX_AUX_BUFFERS",
   "Must be followed by a nonnegative integer that indicates the desired number of auxiliary buffers."
   " Configurations with the  smallest number of auxiliary buffers that meet or exceed the specified number are preferred."
   " The default value is 0.",
   0 },
   { GLX_RED_SIZE, "GLX_RED_SIZE",
   "must be followed by a nonnegative minimum size",
   0 },
   { GLX_GREEN_SIZE, "GLX_GREEN_SIZE",
   "must be followed by a nonnegative minimum size",
   0 },
   { GLX_BLUE_SIZE, "GLX_BLUE_SIZE",
   "must be followed by a nonnegative minimum size",
   0 },
   { GLX_ALPHA_SIZE, "GLX_ALPHA_SIZE",
   "Each attribute, if present, must be followed by a nonnegative minimum size specification or GLX_DONT_CARE."
   " The largest available total RGBA color buffer size (sum of GLX_RED_SIZE, GLX_GREEN_SIZE, GLX_BLUE_SIZE, and GLX_ALPHA_SIZE) "
   " of at least the minimum size specified for each color component is preferred. If the requested number of bits for a color "
   " component is 0 or GLX_DONT_CARE, it is not considered. The default value for each color component is 0.",
   0 },
   { GLX_DEPTH_SIZE, "GLX_DEPTH_SIZE",
   "Must be followed by a nonnegative minimum size specification. If this value is zero, "
   "frame buffer configurations with no depth buffer are preferred."
   "Otherwise, the largest available depth buffer of at least the minimum size is preferred. The default value is 0.",
   0 },
   { GLX_STENCIL_SIZE, "GLX_STENCIL_SIZE",
   "Must be followed by a nonnegative integer that indicates the desired number of stencil bitplanes."
   "The smallest stencil buffer of at least the specified size is preferred. If the desired value is zero,"
   " frame buffer configurations with no stencil buffer are preferred. The default value is 0.",
   0 },
   { GLX_ACCUM_RED_SIZE, "GLX_ACCUM_RED_SIZE",
   "Must be followed by a nonnegative minimum size specification. If this value is zero, "
   " frame buffer configurations with no red accumulation buffer are preferred."
   " Otherwise, the largest possible red accumulation buffer of at least the minimum size is preferred. The default value is 0.",
   0 },
   { GLX_ACCUM_GREEN_SIZE, "GLX_ACCUM_GREEN_SIZE",
   "Must be followed by a nonnegative minimum size specification. If this value is zero, "
   "frame buffer configurations with no green accumulation buffer are preferred. "
   "Otherwise, the largest possible green accumulation buffer of at least the minimum size is preferred. The default value is 0.",
   0 },
   { GLX_ACCUM_BLUE_SIZE, "GLX_ACCUM_BLUE_SIZE",
   "Must be followed by a nonnegative minimum size specification. If this value is zero, "
   "frame buffer configurations with no blue accumulation buffer are preferred. "
   "Otherwise, the largest possible blue accumulation buffer of at least the minimum size is preferred. The default value is 0.",
   0 },
   { GLX_ACCUM_ALPHA_SIZE, "GLX_ACCUM_ALPHA_SIZE",
   "Must be followed by a nonnegative minimum size specification. If this value is zero, "
   "frame buffer configurations with no alpha accumulation buffer are preferred. "
   "Otherwise, the largest possible alpha accumulation buffer of at least the minimum size is preferred. The default value is 0.",
   0 },
   { GLX_RENDER_TYPE, "GLX_RENDER_TYPE",
   "Must be followed by a mask indicating which OpenGL rendering modes the frame buffer configuration must support. "
   "Valid bits are GLX_RGBA_BIT and GLX_COLOR_INDEX_BIT. If the mask is set to GLX_RGBA_BIT | GLX_COLOR_INDEX_BIT, "
   "then only frame buffer configurations that can be bound to both RGBA contexts and color index contexts will be considered. "
   "The default value is GLX_RGBA_BIT.",
    GLX_RGBA_BIT },
   { GLX_DRAWABLE_TYPE, "GLX_DRAWABLE_TYPE",
   "Must be followed by a mask indicating which GLX drawable types the frame buffer configuration must support. "
   "Valid bits are GLX_WINDOW_BIT, GLX_PIXMAP_BIT, and GLX_PBUFFER_BIT. For example, if mask is set to "
   "GLX_WINDOW_BIT | GLX_PIXMAP_BIT,  only frame buffer configurations that support both windows and GLX pixmaps "
   "will be considered. The default value is GLX_WINDOW_BIT.", 
   GLX_WINDOW_BIT },
   { GLX_X_RENDERABLE, "GLX_X_RENDERABLE",
   "Must be followed by True or False. If True is specified, then only frame buffer configurations that "
   "have associated X visuals (and can be used to render to Windows and/or GLX pixmaps) will be considered. "
   "The default value is GLX_DONT_CARE. ",
   GLX_DONT_CARE },
   { GLX_X_VISUAL_TYPE, "GLX_X_VISUAL_TYPE",
   "Must be followed by one of GLX_TRUE_COLOR, GLX_DIRECT_COLOR, GLX_PSEUDO_COLOR, GLX_STATIC_COLOR, "
   "GLX_GRAY_SCALE, or GLX_STATIC_GRAY, indicating the desired X visual type. "
   "Not all frame buffer configurations have an associated X visual. If GLX_DRAWABLE_TYPE is specified in attrib_list and the "
   "mask that follows does not have GLX_WINDOW_BIT set, then this value is ignored. It is also ignored if "
   "GLX_X_RENDERABLE is specified as False. RGBA rendering may be supported for visuals of type "
   "GLX_TRUE_COLOR, GLX_DIRECT_COLOR, GLX_PSEUDO_COLOR, or GLX_STATIC_COLOR, "
   "but color index rendering is only supported for visuals of type GLX_PSEUDO_COLOR or GLX_STATIC_COLOR "
   "(i.e., single-channel visuals). The tokens GLX_GRAY_SCALE and GLX_STATIC_GRAY will "
   "not match current OpenGL enabled visuals, but are included for future use."
   "The default value for GLX_X_VISUAL_TYPE is GLX_DONT_CARE.",
   GLX_DONT_CARE },
   { GLX_CONFIG_CAVEAT, "GLX_CONFIG_CAVEAT",
   "Must be followed by one of GLX_NONE, GLX_SLOW_CONFIG, GLX_NON_CONFORMANT_CONFIG. "
   "If GLX_NONE is specified, then only frame buffer configurations with "
   "no caveats will be considered; if GLX_SLOW_CONFIG is specified, then only slow frame buffer configurations will be considered; if "
   "GLX_NON_CONFORMANT_CONFIG is specified, then only nonconformant frame buffer configurations will be considered."
   "The default value is GLX_DONT_CARE.",
   GLX_DONT_CARE },
   { GLX_TRANSPARENT_TYPE, "GLX_TRANSPARENT_TYPE",
   "Must be followed by one of GLX_NONE, GLX_TRANSPARENT_RGB, GLX_TRANSPARENT_INDEX. "
   "If GLX_NONE is specified, then only opaque frame buffer configurations will be considered; "
   "if GLX_TRANSPARENT_RGB is specified, then only transparent frame buffer configurations that support RGBA rendering will be considered; "
   "if GLX_TRANSPARENT_INDEX is specified, then only transparent frame buffer configurations that support color index rendering will be considered."
   "The default value is GLX_NONE.",
    GLX_NONE },
   { GLX_TRANSPARENT_INDEX_VALUE, "GLX_TRANSPARENT_INDEX_VALUE",
   "Must be followed by an integer value indicating the transparent index value; the value must be between 0 and the maximum "
   "frame buffer value for indices. Only frame buffer configurations that use the "
   "specified transparent index value will be considered. The default value is GLX_DONT_CARE. "
   "This attribute is ignored unless GLX_TRANSPARENT_TYPE is included in attrib_list and specified as GLX_TRANSPARENT_INDEX.",
   GLX_DONT_CARE },
   { GLX_TRANSPARENT_RED_VALUE, "GLX_TRANSPARENT_RED_VALUE",
   "Must be followed by an integer value indicating the transparent red value; the value must be between 0 and the maximum "
   "frame buffer value for red. Only frame buffer configurations that use the specified transparent red value will be considered. "
   "The default value is GLX_DONT_CARE. This attribute is ignored unless GLX_TRANSPARENT_TYPE is included in "
   "attrib_list and specified as GLX_TRANSPARENT_RGB.",
   GLX_DONT_CARE },
   { GLX_TRANSPARENT_GREEN_VALUE, "GLX_TRANSPARENT_GREEN_VALUE",
   "Must be followed by an integer value indicating the transparent green value; the value must be between 0 and the maximum "
   "frame buffer value for green. Only frame buffer configurations that use the specified transparent green value will be considered."
   "The default value is GLX_DONT_CARE. This attribute is "
   "ignored unless GLX_TRANSPARENT_TYPE is included in attrib_list and specified as GLX_TRANSPARENT_RGB.",
   GLX_DONT_CARE },
   { GLX_TRANSPARENT_BLUE_VALUE, "GLX_TRANSPARENT_BLUE_VALUE",
   "Must be followed by an integer value indicating the transparent blue value; the value must be between 0 and the maximum "
   "frame buffer value for blue. Only frame buffer configurations that use the specified transparent blue value will be considered."
   "The default value is GLX_DONT_CARE. This attribute is ignored unless GLX_TRANSPARENT_TYPE is included in "
   "attrib_list and specified as GLX_TRANSPARENT_RGB. ",
   GLX_DONT_CARE },
   { GLX_TRANSPARENT_ALPHA_VALUE, "GLX_TRANSPARENT_ALPHA_VALUE",
   "Must be followed by an integer value indicating the transparent alpha value; the value must be between 0 and the maximum "
   "frame buffer value for alpha. Only frame buffer configurations that use the "
   "specified transparent alpha value will be considered. The default value is GLX_DONT_CARE.",
   GLX_DONT_CARE },
   { 0, NULL, NULL, -1 }
};

const char * get_attr_name( int val, int * pdef )
{
   PPXATTS pat = &pxAtts[0];
   while( pat->name )
   {
      if ( pat->attr == val ) {
         *pdef = pat->def;
         return pat->name;
      }
      pat++;
   }
   *pdef = -1;
   return "VALUE NOT IN LIST";
}

#endif
// eof - RenderTexture.cpp
