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

#define VG_API_EXPORT
#include <vg/openvg.h>
#include <stdio.h>
#include <string.h>
#include "shDefs.h"
#include "shExtensions.h"
#include "shContext.h"


/*-----------------------------------------------------
 * OpenGL core profile
 *-----------------------------------------------------*/
#if defined(_WIN32)
   PFNGLUNIFORM1IPROC                glUniform1i;
   PFNGLUNIFORM2FVPROC               glUniform2fv;
   PFNGLUNIFORMMATRIX3FVPROC         glUniformMatrix3fv;
   PFNGLUNIFORM2FPROC                glUniform2f;
   PFNGLUNIFORM4FVPROC               glUniform4fv;
   PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray;
   PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer;
   PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
   PFNGLUSEPROGRAMPROC               glUseProgram;
   PFNGLUNIFORMMATRIX4FVPROC         glUniformMatrix4fv;
   PFNGLCREATESHADERPROC             glCreateShader;
   PFNGLSHADERSOURCEPROC             glShaderSource;
   PFNGLCOMPILESHADERPROC            glCompileShader;
   PFNGLGETSHADERIVPROC              glGetShaderiv;
   PFNGLATTACHSHADERPROC             glAttachShader;
   PFNGLLINKPROGRAMPROC              glLinkProgram;
   PFNGLGETATTRIBLOCATIONPROC        glGetAttribLocation;
   PFNGLGETUNIFORMLOCATIONPROC       glGetUniformLocation;
   PFNGLDELETESHADERPROC             glDeleteShader;
   PFNGLDELETEPROGRAMPROC            glDeleteProgram;
   PFNGLUNIFORM1FPROC                glUniform1f;
   PFNGLUNIFORM3FPROC                glUniform3f;
   PFNGLUNIFORM4FPROC                glUniform4f;
   PFNGLUNIFORM1FVPROC               glUniform1fv;
   PFNGLUNIFORM3FVPROC               glUniform3fv;
   PFNGLUNIFORMMATRIX2FVPROC         glUniformMatrix2fv;
   PFNGLGETUNIFORMFVPROC             glGetUniformfv;
   PFNGLCREATEPROGRAMPROC            glCreateProgram;
   PFNGLACTIVETEXTUREPROC            glActiveTexture;
#endif

typedef void (*PFVOID) (void);

PFVOID
shGetProcAddress(const char *name)
{
   SH_ASSERT(name != NULL);
#if defined(_WIN32)
   return (PFVOID) wglGetProcAddress(name);
#elif defined(__APPLE__)
   /* TODO: Mac OS glGetProcAddress implementation */
   return (PFVOID) NULL;
#else
   return (PFVOID) glXGetProcAddress((const unsigned char *) name);
#endif
}



void shLoadExtensions(VGContext *c)
{
   SH_ASSERT(c != NULL);

#if defined(_WIN32)
   wglMakeCurrent(NULL, NULL);
#endif

   glewInit();

   if (!GL_VERSION_2_1) {
      SH_LOG_ERR("ShivaVG require OpenGL 2.1");
      exit(EXIT_FAILURE);
   }

   /* GL_TEXTURE_CLAMP_TO_EDGE */
   if (glewIsSupported("GL_VERSION_2_1 GL_EXT_texture_edge_clamp")
       || glewIsSupported("GL_VERSION_2_1 GL_SGIS_texture_edge_clamp"))
      c->isGLAvailable_ClampToEdge = 1;
   else                         /* Unavailable */
      c->isGLAvailable_ClampToEdge = 0;

   SH_DEBUG("Clamp to Edge extension  = %d", c->isGLAvailable_ClampToEdge);

   /* GL_TEXTURE_MIRRORED_REPEAT */
   if (glewIsSupported("GL_VERSION_2_1 GL_ARB_texture_mirrored_repeat")
      || glewIsSupported("GL_VERSION_2_1 GL_IBM_texture_mirrored_repeat"))
      c->isGLAvailable_MirroredRepeat = 1;
   else                         /* Unavailable */
      c->isGLAvailable_MirroredRepeat = 0;

   SH_DEBUG("Mirrored Repeat extension  = %d", c->isGLAvailable_MirroredRepeat);

   /* Non-power-of-two textures */
   if (glewIsSupported("GL_VERSION_2_1 GL_ARB_texture_non_power_of_two"))
      c->isGLAvailable_TextureNonPowerOfTwo = 1;
   else
      c->isGLAvailable_TextureNonPowerOfTwo = 0;

   SH_DEBUG("Texture Non Power of two = %d", c->isGLAvailable_TextureNonPowerOfTwo);

   /* GL_EXT_pixel_buffer_object */
   if (glewIsSupported("GL_EXT_pixel_buffer_object") || glewIsSupported("GL_ARB_pixel_buffer_object") || glewIsSupported("GL_NV_pixel_buffer_object"))
      c->isGLAvailable_PixelBufferObject = 1;
   else
      c->isGLAvailable_PixelBufferObject = 0;

   SH_DEBUG("Pixel Buffer Object extension = %d", c->isGLAvailable_PixelBufferObject);

   if(shGetProcAddress == NULL) return;
 
   #if defined(_WIN32)
     glUniform1i                = shGetProcAddress("glUniform1i");
     glUniform2fv               = shGetProcAddress("glUniform2fv");
     glUniformMatrix3fv         = shGetProcAddress("glUniformMatrix3fv");
     glUniform2f                = shGetProcAddress("glUniform2f");
     glUniform4fv               = shGetProcAddress("glUniform4fv");
     glEnableVertexAttribArray  = shGetProcAddress("glEnableVertexAttribArray");
     glVertexAttribPointer      = shGetProcAddress("glVertexAttribPointer");
     glDisableVertexAttribArray = shGetProcAddress("glDisableVertexAttribArray");
     glUseProgram               = shGetProcAddress("glUseProgram");
     glUniformMatrix4fv         = shGetProcAddress("glUniformMatrix4fv");
     glCreateShader             = shGetProcAddress("glCreateShader");
     glShaderSource             = shGetProcAddress("glShaderSource");
     glCompileShader            = shGetProcAddress("glCompileShader");
     glGetShaderiv              = shGetProcAddress("glGetShaderiv");
     glAttachShader             = shGetProcAddress("glAttachShader");
     glLinkProgram              = shGetProcAddress("glLinkProgram");
     glGetAttribLocation        = shGetProcAddress("glGetAttribLocation");
     glGetUniformLocation       = shGetProcAddress("glGetUniformLocation");
     glDeleteShader             = shGetProcAddress("glDeleteShader");
     glDeleteProgram            = shGetProcAddress("glDeleteProgram");
     glUniform1f                = shGetProcAddress("glUniform1f");
     glUniform3f                = shGetProcAddress("glUniform3f");
     glUniform4f                = shGetProcAddress("glUniform4f");
     glUniform1fv               = shGetProcAddress("glUniform1fv");
     glUniform3fv               = shGetProcAddress("glUniform3fv");
     glUniformMatrix2fv         = shGetProcAddress("glUniformMatrix2fv");
     glGetUniformfv             = shGetProcAddress("glGetUniformfv");
     glCreateProgram            = shGetProcAddress("glCreateProgram");
     glActiveTexture            = shGetProcAddress("glActiveTexture");
   #endif
}
