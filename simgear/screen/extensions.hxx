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

#include <GL/gl.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

static bool SGSearchExtensionsString(char *extString, char *extName);
bool SGIsOpenGLExtensionSupported(char *extName);

inline void (*SGLookupFunction(const char *func))() {

#if defined( WIN32 )
        return (void (*)()) wglGetProcAddress(func);

#else

  // If the target system s UNIX and the ARB_get_proc_address
  // GLX extension is *not* guaranteed to be supported. An alternative
  // dlsym-based approach will be used instead.
  #if defined( linux ) || defined ( sgi )
        void *libHandle;
        void (*fptr)();
        libHandle = dlopen("libGL.so", RTLD_LAZY);
        fptr = (void (*)()) dlsym(libHandle, func);
        dlclose(libHandle);
        return fptr;
  #else
        return glXGetProcAddressARB(func);
  #endif
#endif
}



/* OpenGL extension declarations */
typedef void (APIENTRY * glPointParameterfProc)(GLenum pname, GLfloat param);
typedef void (APIENTRY * glPointParameterfvProc)(GLenum pname, const GLfloat *params);

#if defined(__cplusplus)
}
#endif

#endif // !__SG_EXTENSIONS_HXX

