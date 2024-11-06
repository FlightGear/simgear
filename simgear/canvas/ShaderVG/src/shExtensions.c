/*
 * Copyright (c) 2007 Ivan Leben
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library in the file COPYING;
 * if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdio.h>
#include <string.h>

#if defined( __APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/glcorearb.h>
#endif

#define VG_API_EXPORT
#include "shExtensions.h"

/*-----------------------------------------------------
 * OpenGL core profile
 *-----------------------------------------------------*/
#if defined(_WIN32)
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM3FPROC glUniform3f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORM1FVPROC glUniform1fv;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
PFNGLGETUNIFORMFVPROC glGetUniformfv;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLACTIVETEXTUREPROC glActiveTexture;
/* FlightGear additions to use VAOs and VBOs */
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
#endif

// FlightGear: Extension checking is not needed for the OpenGL core profile
#if 0
/*-----------------------------------------------------
 * Extensions check
 *-----------------------------------------------------*/
static int checkExtension(const char* extensions, const char* name)
{
    int nlen = (int)strlen(name);
    int elen = (int)strlen(extensions);
    const char* e = extensions;
    if (nlen <= 0) return 0;

    while (1) {
        /* Try to find sub-string */
        e = strstr(e, name);
        if (e == NULL) return 0;
        /* Check if last */
        if (e == extensions + elen - nlen)
            return 1;
        /* Check if space follows (avoid same names with a suffix) */
        if (*(e + nlen) == ' ')
            return 1;

        e += nlen;
    }

    return 0;
}
#endif

typedef void (*PFVOID)();

PFVOID shGetProcAddress(const char* name)
{
#if defined(_WIN32)
    return (PFVOID)wglGetProcAddress(name);
#else
    return (PFVOID)NULL;
#endif
}

void shLoadExtensions(void* c)
{
#if defined(_WIN32)
    glUniform1i = shGetProcAddress("glUniform1i");
    glUniform2fv = shGetProcAddress("glUniform2fv");
    glUniformMatrix3fv = shGetProcAddress("glUniformMatrix3fv");
    glUniform2f = shGetProcAddress("glUniform2f");
    glUniform4fv = shGetProcAddress("glUniform4fv");
    glEnableVertexAttribArray = shGetProcAddress("glEnableVertexAttribArray");
    glVertexAttribPointer = shGetProcAddress("glVertexAttribPointer");
    glDisableVertexAttribArray = shGetProcAddress("glDisableVertexAttribArray");
    glUseProgram = shGetProcAddress("glUseProgram");
    glUniformMatrix4fv = shGetProcAddress("glUniformMatrix4fv");
    glCreateShader = shGetProcAddress("glCreateShader");
    glShaderSource = shGetProcAddress("glShaderSource");
    glCompileShader = shGetProcAddress("glCompileShader");
    glGetShaderiv = shGetProcAddress("glGetShaderiv");
    glAttachShader = shGetProcAddress("glAttachShader");
    glLinkProgram = shGetProcAddress("glLinkProgram");
    glGetAttribLocation = shGetProcAddress("glGetAttribLocation");
    glGetUniformLocation = shGetProcAddress("glGetUniformLocation");
    glDeleteShader = shGetProcAddress("glDeleteShader");
    glDeleteProgram = shGetProcAddress("glDeleteProgram");
    glUniform1f = shGetProcAddress("glUniform1f");
    glUniform3f = shGetProcAddress("glUniform3f");
    glUniform4f = shGetProcAddress("glUniform4f");
    glUniform1fv = shGetProcAddress("glUniform1fv");
    glUniform3fv = shGetProcAddress("glUniform3fv");
    glUniformMatrix2fv = shGetProcAddress("glUniformMatrix2fv");
    glGetUniformfv = shGetProcAddress("glGetUniformfv");
    glCreateProgram = shGetProcAddress("glCreateProgram");
    glActiveTexture = shGetProcAddress("glActiveTexture");
    /* FlightGear additions to use VAOs and VBOs */
    glGenVertexArrays = shGetProcAddress("glGenVertexArrays");
    glDeleteVertexArrays = shGetProcAddress("glDeleteVertexArrays");
    glBindVertexArray = shGetProcAddress("glBindVertexArray");
    glGenBuffers = shGetProcAddress("glGenBuffers");
    glDeleteBuffers = shGetProcAddress("glDeleteBuffers");
    glBindBuffer = shGetProcAddress("glBindBuffer");
    glBufferData = shGetProcAddress("glBufferData");
#endif
}
