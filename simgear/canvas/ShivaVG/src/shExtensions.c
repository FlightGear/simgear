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
#include <VG/openvg.h>
#include <stdio.h>
#include <string.h>
#include "shDefs.h"
#include "shExtensions.h"
#include "shContext.h"


/*-----------------------------------------------------
 * Extensions check
 *-----------------------------------------------------*/

void
fallbackActiveTexture(GLenum texture)
{
}

void
fallbackMultiTexCoord1f(GLenum target, GLfloat x)
{
   glTexCoord1f(x);
}

void
fallbackMultiTexCoord2f(GLenum target, GLfloat x, GLfloat y)
{
   glTexCoord2f(x, y);
}

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

   /* glActiveTexture, glMultiTexCoord1f */
   if (glewIsSupported("GL_VERSION_2_1 GL_ARB_multitexture")) {
      c->isGLAvailable_Multitexture = 1;
      c->pglActiveTexture = (SH_PGLACTIVETEXTURE)
         shGetProcAddress("glActiveTextureARB");
      c->pglMultiTexCoord1f = (SH_PGLMULTITEXCOORD1F)
         shGetProcAddress("glMultiTexCoord1fARB");
      c->pglMultiTexCoord2f = (SH_PGLMULTITEXCOORD2F)
         shGetProcAddress("glMultiTexCoord2fARB");
   } else {
      c->isGLAvailable_Multitexture = 0;
      c->pglActiveTexture = (SH_PGLACTIVETEXTURE) fallbackActiveTexture;
      c->pglMultiTexCoord1f = (SH_PGLMULTITEXCOORD1F) fallbackMultiTexCoord1f;
      c->pglMultiTexCoord2f = (SH_PGLMULTITEXCOORD2F) fallbackMultiTexCoord2f;
   }

   SH_DEBUG("Multi Texture extension = %d", c->isGLAvailable_Multitexture);

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
}
