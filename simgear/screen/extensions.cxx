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

#include <string.h>

#include "extensions.hxx"
#include <simgear/debug/logstream.hxx>

bool SGSearchExtensionsString(const char *extString, const char *extName) {
    // Returns GL_TRUE if the *extName string appears in the *extString string,
    // surrounded by white spaces, or GL_FALSE otherwise.

    const char *p, *end;
    int n, extNameLen;

    if ((extString == NULL) || (extName == NULL))
        return false;

    extNameLen = strlen(extName);

    p=extString;
    end = p + strlen(p);

    while (p < end) {
        n = strcspn(p, " ");
        if ((extNameLen == n) && (strncmp(extName, p, n) == 0))
            return GL_TRUE;

        p += (n + 1);
    }

    return GL_FALSE;
}

bool SGIsOpenGLExtensionSupported(char *extName) {
   // Returns GL_TRUE if the OpenGL Extension whose name is *extName
   // is supported by the system, or GL_FALSE otherwise.
   //
   // The *extName string must follow the OpenGL extensions naming scheme
   // (ie: "GL_type_extension", like GL_EXT_convolution)

    return SGSearchExtensionsString((const char *)glGetString(GL_EXTENSIONS),extName);
}

#ifdef __APPLE__

#include <CoreFoundation/CoreFoundation.h>

void* macosxGetGLProcAddress(const char *func) {

  /* We may want to cache the bundleRef at some point */
  static CFBundleRef bundle = 0;

  if (!bundle) {

    CFURLRef bundleURL = CFURLCreateWithFileSystemPath (kCFAllocatorDefault,
							CFSTR("/System/Library/Frameworks/OpenGL.framework"), kCFURLPOSIXPathStyle, true);

    bundle = CFBundleCreate (kCFAllocatorDefault, bundleURL);
    CFRelease (bundleURL);
  }

  if (!bundle)
    return 0;

  CFStringRef functionName = CFStringCreateWithCString
    (kCFAllocatorDefault, func, kCFStringEncodingASCII);
  
  void *function;
  
  function = CFBundleGetFunctionPointerForName (bundle, functionName);

  CFRelease (functionName);

  return function;
}

#elif !defined( WIN32 )

void *SGGetGLProcAddress(const char *func) {
    static void *libHandle = NULL;
    void *fptr = NULL;

    /*
     * Clear the error buffer
     */
    dlerror();

    if (libHandle == NULL)
        libHandle = dlopen("libGL.so", RTLD_LAZY);

    if (libHandle != NULL) {
        fptr = dlsym(libHandle, func);

#if defined (__FreeBSD__)
        const char *error = dlerror();
#else
        char *error = dlerror();
#endif
        if (error)
            SG_LOG(SG_GENERAL, SG_INFO, error);
    }

    return fptr;
}

#endif

