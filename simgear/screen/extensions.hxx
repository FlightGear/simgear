/*
 *
 * Copyright (c) 2001 César Blecua Udías    All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * CESAR BLECUA UDIAS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */
#ifndef __SG_EXTENSIONS_HXX
#define __SG_EXTENSIONS_HXX 1

#if !defined(WIN32)
# include <dlfcn.h>
#endif

#if defined(__CYGWIN__)  /* && !defined(USING_X) */
#define WIN32
#endif

#if defined(WIN32)  /* MINGW and MSC predefine WIN32 */
# include <windows.h>
#endif

#include <GL/gl.h>


#if defined(__cplusplus)
extern "C" {
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

    // static bool SGSearchExtensionsString(char *extString, char *extName);
bool SGIsOpenGLExtensionSupported(char *extName);

#ifdef __APPLE__
  // don't use an inline function for symbol lookup, since it is too big
  void* macosxGetGLProcAddress(const char *func);
#endif

inline void (*SGLookupFunction(const char *func))()
{
#if defined( WIN32 ) && !defined(__CYGWIN__) && !defined(__MINGW32__)
    return (void (*)()) wglGetProcAddress(func);

#elif defined( __APPLE__ )
    return (void (*)()) macosxGetGLProcAddress(func);

#else // UNIX

    // If the target system s UNIX and the ARB_get_proc_address
    // GLX extension is *not* guaranteed to be supported. An alternative
    // dlsym-based approach will be used instead.

    void *libHandle;
    void (*fptr)();
    libHandle = dlopen("libGL.so", RTLD_LAZY);
    fptr = (void (*)()) dlsym(libHandle, func);
    dlclose(libHandle);
    return fptr;
#endif
}



/* OpenGL extension declarations */

/*
 * glPointParameterf and glPointParameterfv
 */
#ifndef GL_EXT_point_parameters
#define GL_EXT_point_parameters 1
#define GL_POINT_SIZE_MIN_EXT                                   0x8126
#define GL_DISTANCE_ATTENUATION_EXT                             0x8129
#endif

#ifndef GL_ARB_point_parameters
#define GL_ARB_point_parameters 1
#define GL_POINT_SIZE_MIN_ARB					0x8126
#define GL_DISTANCE_ATTENUATION_ARB				0x8129
#endif

typedef void (APIENTRY * glPointParameterfProc)(GLenum pname, GLfloat param);
typedef void (APIENTRY * glPointParameterfvProc)(GLenum pname, const GLfloat *params);

/*
 * glActiveTextureARB
 */

#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture 1
#define GL_TEXTURE0_ARB                                         0x84C0
#define GL_TEXTURE1_ARB                                         0x84C1
#define GL_TEXTURE2_ARB                                         0x84C2
#define GL_TEXTURE3_ARB                                         0x84C3
#define GL_TEXTURE4_ARB                                         0x84C4
#define GL_TEXTURE5_ARB                                         0x84C5
#define GL_TEXTURE6_ARB                                         0x84C6
#define GL_TEXTURE7_ARB                                         0x84C7
#define GL_TEXTURE8_ARB                                         0x84C8
#define GL_TEXTURE9_ARB                                         0x84C9
#define GL_TEXTURE10_ARB                                        0x84CA
#define GL_TEXTURE11_ARB                                        0x84CB
#define GL_TEXTURE12_ARB                                        0x84CC
#define GL_TEXTURE13_ARB                                        0x84CD
#define GL_TEXTURE14_ARB                                        0x84CE
#define GL_TEXTURE15_ARB                                        0x84CF
#define GL_TEXTURE16_ARB                                        0x84D0
#define GL_TEXTURE17_ARB                                        0x84D1
#define GL_TEXTURE18_ARB                                        0x84D2
#define GL_TEXTURE19_ARB                                        0x84D3
#define GL_TEXTURE20_ARB                                        0x84D4
#define GL_TEXTURE21_ARB                                        0x84D5
#define GL_TEXTURE22_ARB                                        0x84D6
#define GL_TEXTURE23_ARB                                        0x84D7
#define GL_TEXTURE24_ARB                                        0x84D8
#define GL_TEXTURE25_ARB                                        0x84D9
#define GL_TEXTURE26_ARB                                        0x84DA
#define GL_TEXTURE27_ARB                                        0x84DB
#define GL_TEXTURE28_ARB                                        0x84DC
#define GL_TEXTURE29_ARB                                        0x84DD
#define GL_TEXTURE30_ARB                                        0x84DE
#define GL_TEXTURE31_ARB                                        0x84DF
#define GL_ACTIVE_TEXTURE_ARB                                   0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB                            0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB                                0x84E2
#endif

typedef void (APIENTRY * glActiveTextureProc)(GLenum texture);

/*
 * GL_EXT_separate_specular_color
 */

#ifndef GL_LIGHT_MODEL_COLOR_CONTROL
#define GL_LIGHT_MODEL_COLOR_CONTROL                            0x81F8
#define GL_SINGLE_COLOR                                         0x81F9
#define GL_SEPARATE_SPECULAR_COLOR                              0x81FA
#endif

#if defined(__cplusplus)
}
#endif

#endif // !__SG_EXTENSIONS_HXX

