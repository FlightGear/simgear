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

#include "shExtensions.h"

#if defined(VG_API_WINDOWS)
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
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
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
PFNGLUNIFORM2IPROC glUniform2i;
PFNGLUNIFORM3IPROC glUniform3i;
PFNGLUNIFORM4IPROC glUniform4i;
PFNGLUNIFORM1IVPROC glUniform1iv;
PFNGLUNIFORM2IVPROC glUniform2iv;
PFNGLUNIFORM3IVPROC glUniform3iv;
PFNGLUNIFORM4IVPROC glUniform4iv;
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

/*
 * Query a function pointer on Windows.
 *
 * This only works in the presence of a valid OpenGL context.
 * wglGetProcAddress() will not return function pointers from any OpenGL
 * functions that are directly exported by opengl32.dll. Use GetProcAddress()
 * for those when wglGetProcAddress() fails.
 */
static void* shGetProcAddress(const char* name)
{
    void* p = (void*)wglGetProcAddress(name);
    if (p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1)) {
        HMODULE module = LoadLibraryA("opengl32.dll");
        p = (void*)GetProcAddress(module, name);
    }
    return p;
}

#endif

void shLoadExtensions()
{
#if defined(VG_API_WINDOWS)
    glUniform1i = (PFNGLUNIFORM1IPROC)shGetProcAddress("glUniform1i");
    glUniform2fv = (PFNGLUNIFORM2FVPROC)shGetProcAddress("glUniform2fv");
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)shGetProcAddress("glUniformMatrix3fv");
    glUniform2f = (PFNGLUNIFORM2FPROC)shGetProcAddress("glUniform2f");
    glUniform4fv = (PFNGLUNIFORM4FVPROC)shGetProcAddress("glUniform4fv");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)shGetProcAddress("glEnableVertexAttribArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)shGetProcAddress("glVertexAttribPointer");
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)shGetProcAddress("glDisableVertexAttribArray");
    glUseProgram = (PFNGLUSEPROGRAMPROC)shGetProcAddress("glUseProgram");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)shGetProcAddress("glUniformMatrix4fv");
    glCreateShader = (PFNGLCREATESHADERPROC)shGetProcAddress("glCreateShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)shGetProcAddress("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)shGetProcAddress("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)shGetProcAddress("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)shGetProcAddress("glGetShaderInfoLog");
    glAttachShader = (PFNGLATTACHSHADERPROC)shGetProcAddress("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)shGetProcAddress("glLinkProgram");
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)shGetProcAddress("glGetAttribLocation");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)shGetProcAddress("glGetUniformLocation");
    glDeleteShader = (PFNGLDELETESHADERPROC)shGetProcAddress("glDeleteShader");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)shGetProcAddress("glDeleteProgram");
    glUniform1f = (PFNGLUNIFORM1FPROC)shGetProcAddress("glUniform1f");
    glUniform3f = (PFNGLUNIFORM3FPROC)shGetProcAddress("glUniform3f");
    glUniform4f = (PFNGLUNIFORM4FPROC)shGetProcAddress("glUniform4f");
    glUniform1fv = (PFNGLUNIFORM1FVPROC)shGetProcAddress("glUniform1fv");
    glUniform3fv = (PFNGLUNIFORM3FVPROC)shGetProcAddress("glUniform3fv");
    glUniform2i = (PFNGLUNIFORM2IPROC)shGetProcAddress("glUniform2i");
    glUniform3i = (PFNGLUNIFORM3IPROC)shGetProcAddress("glUniform3i");
    glUniform4i = (PFNGLUNIFORM4IPROC)shGetProcAddress("glUniform4i");
    glUniform1iv = (PFNGLUNIFORM1IVPROC)shGetProcAddress("glUniform1iv");
    glUniform2iv = (PFNGLUNIFORM2IVPROC)shGetProcAddress("glUniform2iv");
    glUniform3iv = (PFNGLUNIFORM3IVPROC)shGetProcAddress("glUniform3iv");
    glUniform4iv = (PFNGLUNIFORM4IVPROC)shGetProcAddress("glUniform4iv");
    glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)shGetProcAddress("glUniformMatrix2fv");
    glGetUniformfv = (PFNGLGETUNIFORMFVPROC)shGetProcAddress("glGetUniformfv");
    glCreateProgram = (PFNGLCREATEPROGRAMPROC)shGetProcAddress("glCreateProgram");
    glActiveTexture = (PFNGLACTIVETEXTUREPROC)shGetProcAddress("glActiveTexture");
    /* FlightGear additions to use VAOs and VBOs */
    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)shGetProcAddress("glGenVertexArrays");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)shGetProcAddress("glDeleteVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)shGetProcAddress("glBindVertexArray");
    glGenBuffers = (PFNGLGENBUFFERSPROC)shGetProcAddress("glGenBuffers");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)shGetProcAddress("glDeleteBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)shGetProcAddress("glBindBuffer");
    glBufferData = (PFNGLBUFFERDATAPROC)shGetProcAddress("glBufferData");
#endif
}
