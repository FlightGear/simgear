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
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "shImage.h"
#include "shContext.h"
#include "shMath.h"
#include "shDefs.h"

#define _ITEM_T SHColor
#define _ARRAY_T SHColorArray
#define _FUNC_T shColorArray
#define _ARRAY_DEFINE
#define _COMPARE_T(c1,c2) 0
#include "shArrayBase.h"

#define _ITEM_T SHImage*
#define _ARRAY_T SHImageArray
#define _FUNC_T shImageArray
#define _ARRAY_DEFINE
#include "shArrayBase.h"

#define SH_BASE_IMAGE_FORMAT(format) ((format) & 0x1F)

static inline SHfloat
shInt2ColorComponent(SHuint c)
{
   return (SHfloat) ((c) & 255) / 255.0f;
}

static inline SHuint
shColorComponent2Int(SHfloat c)
{
   SHint t = (SHint)SH_FLOOR((c) * 255.0f + 0.5f);
   if (t > 255)
      return 255;
   if (t < 0)
      return 0;
   return t;
}


static inline bool
shOverlaps(SHImage * restrict src, SHImage * restrict dst, SHint sx, SHint sy, SHint dx, SHint dy, SHint width, SHint height)
{
   SH_ASSERT(src != NULL && dst != NULL && src->data != NULL && dst->data != NULL);
   if (src->data != dst->data)
      return false; // images don't share data

   SHint ws = (src->width < width ? src->width : width);
   SHint wd = (dst->width < width ? dst->width : width);
   SHint hs = (src->height < height ? src->height : height);
   SHint hd = (dst->height < height ? dst->height : height);

   SHint left = (sx > dx ? sx: dx); // max(sx,dx)
   SHint right = (sx + ws < dx + wd ? sx + ws: dx + wd); // min(sx + ws, dx + wd)
   SHint bottom = (sy > dy ? sy: dy); // max(sy,dy)
   SHint top = (sy + hs < dy + hd ? sy + hs: dy + hd); // min(sy + hs, dy + hd)

   if (top - bottom < 0 && right - left < 0)
      return false; // no overlaps

   return true;

}

/*-----------------------------------------------------------
 * Prepares the proper pixel pack/unpack info for the given
 * OpenVG image format.
 *-----------------------------------------------------------*/

static void
shSetupImageFormat(VGImageFormat vg, SHImageFormatDesc * f)
{
   SHuint8 abits = 0;
   SHuint8 tshift = 0;
   SHuint8 tmask = 0;
   SHuint32 amsbBit = 0;
   SHuint32 bgrBit = 0;

   SH_ASSERT(f != NULL);

   /* Store VG format name */
   f->vgformat = vg;

   /* Check if alpha on MSB or colors in BGR order */
   amsbBit = ((vg & (1 << 6)) >> 6);
   bgrBit = ((vg & (1 << 7)) >> 7);

   /* Find component ordering and size */
   VGImageFormat baseFormat = SH_BASE_IMAGE_FORMAT(vg);
   switch (baseFormat) {
   case VG_sRGBX_8888:
   case VG_lRGBX_8888:
      f->bytes = 4;
      f->rmask = 0xFF000000;
      f->rshift = 24;
      f->rmax = 255;
      f->gmask = 0x00FF0000;
      f->gshift = 16;
      f->gmax = 255;
      f->bmask = 0x0000FF00;
      f->bshift = 8;
      f->bmax = 255;
      f->amask = 0x0;
      f->ashift = 0;
      f->amax = 1;
      f->linear = baseFormat & 7;
      f->premultiplied = true;
      break;
   case VG_sRGBA_8888:
   case VG_sRGBA_8888_PRE:
   case VG_lRGBA_8888:
   case VG_lRGBA_8888_PRE:
      f->bytes = 4;
      f->rmask = 0xFF000000;
      f->rshift = 24;
      f->rmax = 255;
      f->gmask = 0x00FF0000;
      f->gshift = 16;
      f->gmax = 255;
      f->bmask = 0x0000FF00;
      f->bshift = 8;
      f->bmax = 255;
      f->amask = 0x000000FF;
      f->ashift = 0;
      f->amax = 255;
      f->linear = (baseFormat & 8) | (baseFormat & 9);
      f->premultiplied = false;
      break;
   case VG_sRGB_565:
      f->bytes = 2;
      f->rmask = 0xF800;
      f->rshift = 11;
      f->rmax = 31;
      f->gmask = 0x07E0;
      f->gshift = 5;
      f->gmax = 63;
      f->bmask = 0x001F;
      f->bshift = 0;
      f->bmax = 31;
      f->amask = 0x0;
      f->ashift = 0;
      f->amax = 1;
      f->linear = false;
      f->premultiplied = true;
      break;
   case VG_sRGBA_5551:
      f->bytes = 2;
      f->rmask = 0xF800;
      f->rshift = 11;
      f->rmax = 31;
      f->gmask = 0x07C0;
      f->gshift = 6;
      f->gmax = 31;
      f->bmask = 0x003E;
      f->bshift = 1;
      f->bmax = 31;
      f->amask = 0x0001;
      f->ashift = 0;
      f->amax = 1;
      f->linear = false;
      f->premultiplied = false;
      break;
   case VG_sRGBA_4444:
      f->bytes = 2;
      f->rmask = 0xF000;
      f->rshift = 12;
      f->rmax = 15;
      f->gmask = 0x0F00;
      f->gshift = 8;
      f->gmax = 15;
      f->bmask = 0x00F0;
      f->bshift = 4;
      f->bmax = 15;
      f->amask = 0x000F;
      f->ashift = 0;
      f->amax = 15;
      f->linear = false;
      f->premultiplied = false;
      break;
   case VG_sL_8:
   case VG_lL_8:
      f->bytes = 1;
      f->rmask = 0xFF;
      f->rshift = 0;
      f->rmax = 255;
      f->gmask = 0xFF;
      f->gshift = 0;
      f->gmax = 255;
      f->bmask = 0xFF;
      f->bshift = 0;
      f->bmax = 255;
      f->amask = 0x0;
      f->ashift = 0;
      f->amax = 1;
      f->linear = baseFormat & 10;
      f->premultiplied = false;
      break;
   case VG_A_8:
      f->bytes = 1;
      f->rmask = 0x0;
      f->rshift = 0;
      f->rmax = 1;
      f->gmask = 0x0;
      f->gshift = 0;
      f->gmax = 1;
      f->bmask = 0x0;
      f->bshift = 0;
      f->bmax = 1;
      f->amask = 0xFF;
      f->ashift = 0;
      f->amax = 255;
      f->linear = false;
      f->premultiplied = false;
      break;
   case VG_BW_1:
      f->bytes = 1;
      f->rmask = 0x0;
      f->rshift = 0;
      f->rmax = 1;
      f->gmask = 0x0;
      f->gshift = 0;
      f->gmax = 1;
      f->bmask = 0x0;
      f->bshift = 0;
      f->bmax = 1;
      f->amask = 0x0;
      f->ashift = 0;
      f->amax = 1;
      f->linear = false;
      f->premultiplied = false;
      break;
   default:
   }

   /* Check for A,X at MSB */
   if (amsbBit) {

      abits = f->bshift;

      f->rshift -= abits;
      f->gshift -= abits;
      f->bshift -= abits;
      f->ashift = f->bytes * 8 - abits;

      f->rmask >>= abits;
      f->gmask >>= abits;
      f->bmask >>= abits;
      f->amask <<= f->bytes * 8 - abits;
   }

   /* Check for BGR ordering */
   if (bgrBit) {

      tshift = f->bshift;
      f->bshift = f->rshift;
      f->rshift = tshift;

      tmask = f->bmask;
      f->bmask = f->rmask;
      f->rmask = tmask;
   }

   SH_DEBUG("shSetupImageFormat bgrBit %d, amsbBit %d\n ", bgrBit, amsbBit);

   /* Find proper mapping to OpenGL formats */
   // TODO: fix this! The endianness calc seems not correct.
   // https://www.opengl.org/discussion_boards/showthread.php/151639-GL_UNSIGNED_INT_8_8_8_8-%21-GL_UNSIGNED_BYTE
   // OpenGLES support only GL_RGB{A} internal format. The library must handle byte swap
   switch (SH_BASE_IMAGE_FORMAT(vg)) {
   case VG_sRGBX_8888:
   case VG_lRGBX_8888:
   case VG_sRGBA_8888:
   case VG_sRGBA_8888_PRE:
   case VG_lRGBA_8888:
   case VG_lRGBA_8888_PRE:

      f->glintformat = GL_RGBA8;

      if (amsbBit == 0 && bgrBit == 0) {
         f->glformat = GL_RGBA;
         f->gltype = GL_UNSIGNED_INT_8_8_8_8; // probabilmente l'endianness dei test è sbagliata (quelle koreane tipo test_gaussian test_eb) bisogna fare un test per verificare questo caso
      } else if (amsbBit == 1 && bgrBit == 0) {
         f->glformat = GL_BGRA; // should be GL_RGBA ?
         f->gltype = GL_UNSIGNED_INT_8_8_8_8_REV; // TODO: might be REV ?

      } else if (amsbBit == 0 && bgrBit == 1) {
         f->glformat = GL_BGRA;
         f->gltype = GL_UNSIGNED_INT_8_8_8_8;

      } else if (amsbBit == 1 && bgrBit == 1) {
         f->glformat = GL_RGBA;
         f->gltype = GL_UNSIGNED_INT_8_8_8_8_REV;
      }
      break;
   case VG_sRGBA_5551:

      f->glintformat = GL_RGBA8;

      if (amsbBit == 0 && bgrBit == 0) {
         f->glformat = GL_RGBA;
         f->gltype = GL_UNSIGNED_SHORT_5_5_5_1;

      } else if (amsbBit == 1 && bgrBit == 0) {
         f->glformat = GL_BGRA;
         f->gltype = GL_UNSIGNED_SHORT_1_5_5_5_REV;

      } else if (amsbBit == 0 && bgrBit == 1) {
         f->glformat = GL_BGRA;
         f->gltype = GL_UNSIGNED_SHORT_5_5_5_1;

      } else if (amsbBit == 1 && bgrBit == 1) {
         f->glformat = GL_RGBA;
         f->gltype = GL_UNSIGNED_SHORT_1_5_5_5_REV;
      }

      break;
   case VG_sRGBA_4444:

      f->glintformat = GL_RGBA8;

      if (amsbBit == 0 && bgrBit == 0) {
         f->glformat = GL_RGBA;
         f->gltype = GL_UNSIGNED_SHORT_4_4_4_4; // GL_UNSIGNED_SHORT_4_4_4_4;

      } else if (amsbBit == 1 && bgrBit == 0) {
         f->glformat = GL_BGRA;
         f->gltype = GL_UNSIGNED_SHORT_4_4_4_4_REV; // GL_UNSIGNED_SHORT_4_4_4_4_REV

      } else if (amsbBit == 0 && bgrBit == 1) {
         f->glformat = GL_BGRA;
         f->gltype = GL_UNSIGNED_SHORT_4_4_4_4; // GL_UNSIGNED_SHORT_4_4_4_4;

      } else if (amsbBit == 1 && bgrBit == 1) {
         f->glformat = GL_RGBA;
         f->gltype = GL_UNSIGNED_SHORT_4_4_4_4_REV;
      }

      break;
   case VG_sRGB_565:

      f->glintformat = GL_RGB;

      if (bgrBit == 0) {
         f->glformat = GL_RGB;
         f->gltype = GL_UNSIGNED_SHORT_5_6_5;

      } else if (bgrBit == 1) {
         f->glformat = GL_RGB;
         f->gltype = GL_UNSIGNED_SHORT_5_6_5;
      }

      break;
   case VG_sL_8:
   case VG_lL_8:

      f->glintformat = GL_LUMINANCE;
      f->glformat = GL_LUMINANCE;
      f->gltype = GL_UNSIGNED_BYTE;

      break;
   case VG_A_8:

      f->glintformat = GL_ALPHA;
      f->glformat = GL_ALPHA;
      f->gltype = GL_UNSIGNED_BYTE;

      break;
   case VG_BW_1:

      f->glintformat = 0;
      f->glformat = 0;
      f->gltype = 0;
      break;
   }
}

/*-----------------------------------------------------
 * Returns 1 if the given format is valid according to
 * the OpenVG specification, else 0.
 *-----------------------------------------------------*/

static int
shIsValidImageFormat(VGImageFormat format)
{
   SHint aOrderBit = (1 << 6);
   SHint rgbOrderBit = (1 << 7);
   SHint baseFormat = SH_BASE_IMAGE_FORMAT(format);
   SHint unorderedRgba = format & (~(aOrderBit | rgbOrderBit));
   SHint isRgba = (baseFormat == VG_sRGBX_8888 ||
                   baseFormat == VG_sRGBA_8888 ||
                   baseFormat == VG_sRGBA_8888_PRE ||
                   baseFormat == VG_sRGBA_5551 ||
                   baseFormat == VG_sRGBA_4444 ||
                   baseFormat == VG_lRGBX_8888 ||
                   baseFormat == VG_lRGBA_8888 ||
                   baseFormat == VG_lRGBA_8888_PRE);

   SHint check = isRgba ? unorderedRgba : format;
   return check >= VG_sRGBX_8888 && check <= VG_BW_1;
}

/*-----------------------------------------------------
 * Returns 1 if the given format is supported by this
 * implementation
 *-----------------------------------------------------*/

static inline SHint
shIsSupportedImageFormat(VGImageFormat format)
{
   SHuint32 baseFormat = SH_BASE_IMAGE_FORMAT(format);
   if (baseFormat == VG_sRGBA_8888_PRE ||
       baseFormat == VG_lRGBA_8888_PRE ||
       baseFormat == VG_BW_1)  return 0;

   return 1;
}


/*--------------------------------------------------------
 * Packed color according to given color format
 *--------------------------------------------------------*/

static inline SHuint32
shPackColor(SHColor * restrict c, const SHImageFormatDesc * restrict f)
{
   /*
     TODO: unsupported formats:
     - s and l both behave linearly
     - 1-bit black & white (BW_1)
   */
   SH_ASSERT(c != NULL && f != NULL);

   SHuint32 out = 0x0;
   if (f->vgformat != VG_lL_8 && f->vgformat != VG_sL_8) {
      /* Pack color components */
      out += (((SHuint32) (c->r * (SHfloat) f->rmax + 0.5f)) << f->rshift) & f->rmask;
      out += (((SHuint32) (c->g * (SHfloat) f->gmax + 0.5f)) << f->gshift) & f->gmask;
      out += (((SHuint32) (c->b * (SHfloat) f->bmax + 0.5f)) << f->bshift) & f->bmask;
      out += (((SHuint32) (c->a * (SHfloat) f->amax + 0.5f)) << f->ashift) & f->amask;
   } else {
      /* Grayscale (luminosity) conversion as defined by the spec */
      SHfloat l = 0.2126f * c->r + 0.7152f * c->g + 0.0722f * c->r;
      out = (SHuint32) (l * (SHfloat) f->rmax + 0.5f);
   }
   return out;
}


/*--------------------------------------------------------
 * Store packed color into memory at given
 * address according to given color format size
 *--------------------------------------------------------*/

static inline void
shStorePackedColor(void * restrict data, SHuint8 colorFormatSize, SHuint32 packedColor)
{
   SH_ASSERT(data != NULL);

   /* Store to buffer */
   switch (colorFormatSize) {
   case 4:
      *((SHuint32 *) data) = (SHuint32) (packedColor & 0xFFFFFFFF);
      break;
   case 2:
      *((SHuint16 *) data) = (SHuint16) (packedColor & 0x0000FFFF);
      break;
   case 1:
      *((SHuint8 *) data) = (SHuint8) (packedColor & 0x000000FF);
      break;
   }
}

/*--------------------------------------------------------
 * Packs the pixel color components into memory at given
 * address according to given format
 *--------------------------------------------------------*/

static inline void
shStoreColor(SHColor * restrict c, void * restrict data, const SHImageFormatDesc * restrict f)
{
   SHuint32 packedColor = shPackColor(c, f);
   shStorePackedColor(data, f->bytes, packedColor);
}

/*---------------------------------------------------------
 * Unpacks the pixel color components from memory at given
 * address according to the given format
 *---------------------------------------------------------*/

static inline void
shLoadColor(SHColor * restrict c, const void * restrict data, const SHImageFormatDesc * restrict f)
{
   /*
     TODO: unsupported formats:
     - s and l both behave linearly
     - 1-bit black & white (BW_1)
   */

   SHuint32 in = 0x0;

   SH_ASSERT(c != NULL && data != NULL);

   /* Load from buffer */
   switch (f->bytes) {
   case 4:
      in = (SHuint32) * ((SHuint32 *) data);
      break;
   case 2:
      in = (SHuint32) * ((SHuint16 *) data);
      break;
   case 1:
      in = (SHuint32) * ((SHuint8 *) data);
      break;
   }

   /* Unpack color components */
   c->r = (SHfloat) ((in & f->rmask) >> f->rshift) / (SHfloat) f->rmax;
   c->g = (SHfloat) ((in & f->gmask) >> f->gshift) / (SHfloat) f->gmax;
   c->b = (SHfloat) ((in & f->bmask) >> f->bshift) / (SHfloat) f->bmax;
   c->a = (SHfloat) ((in & f->amask) >> f->ashift) / (SHfloat) f->amax;

   /* Initialize unused components to 1 */
   if (f->amask == 0x0) {
      c->a = 1.0f;
   }
   if (f->rmask == 0x0) {
      c->r = 1.0f;
      c->g = 1.0f;
      c->b = 1.0f;
   }
}

/*---------------------------------------------------------
 * Unpacks the pixel color components from image at
 * given coordinates
 *---------------------------------------------------------*/

static inline void
shLoadPixelColor(SHColor * restrict c, const void * restrict data, const SHImageFormatDesc * restrict f, SHint x, SHint y, SHint32 stride)
{
   SHuint8 *px = (SHuint8 *) data + y * stride + x * f->bytes;
   shLoadColor(c,  px, f);
}

static inline void
shStorePixelColor(SHColor * restrict c, const void * restrict data, const SHImageFormatDesc * restrict f, SHint x, SHint y, SHint32 stride)
{
   SHuint8 *px = (SHuint8 *) data + y * stride + x * f->bytes;
   shStoreColor(c,  px, f);
}

/*----------------------------------------------
 * Color and Image constructors and destructors
 *----------------------------------------------*/

void
SHColor_ctor(SHColor * c)
{
   SH_ASSERT(c != NULL);

   c->r = 0.0f;
   c->g = 0.0f;
   c->b = 0.0f;
   c->a = 0.0f;
}

void
SHColor_dtor(SHColor * c)
{

}

void
SHImage_ctor(SHImage * i)
{
   SH_ASSERT(i != NULL);

   i->data = NULL;
   i->width = 0;
   i->height = 0;
   glGenTextures(1, &i->texture);
}

void
SHImage_dtor(SHImage * i)
{
   SH_ASSERT(i != NULL);

   if (i->data != NULL)
      free(i->data);

   if (glIsTexture(i->texture))
      glDeleteTextures(1, &i->texture);
}

/*--------------------------------------------------------
 * Finds appropriate OpenGL texture size for the size of
 * the given image 
 *--------------------------------------------------------*/

void
shUpdateImageTextureSize(SHImage * i)
{
   SH_ASSERT(i != NULL);

   i->texwidth = i->width;
   i->texheight = i->height;
   i->texwidthK = 1.0f;
   i->texheightK = 1.0f;

   // update stride
   i->stride = i->texwidth * i->fd.bytes;

   /* Round size to nearest power of 2 */
   /* TODO: might be dropped out if it works without  */
/*
  i->texwidth = shClp2(i->width);

  i->texheight = shClp2(i->height);

  i->texwidthK  = (SHfloat)i->width  / i->texwidth;
  i->texheightK = (SHfloat)i->height / i->texheight;
*/
}

/*--------------------------------------------------
 * Downloads the image data from OpenVG into
 * an OpenGL texture
 *--------------------------------------------------*/

void
shUpdateImageTexture(SHImage * restrict i, VGContext * restrict context)
{
   SH_ASSERT(i != NULL && context != NULL);

   /* Find nearest power of two size */
   SHint potwidth = shClp2(i->width);
   SHint potheight = shClp2(i->height);

   /* Scale into a temp buffer if image not a power-of-two size (pot)
      and non-power-of-two textures are not supported by OpenGL */

   if ((i->width < potwidth || i->height < potheight) &&
       !context->isGLAvailable_TextureNonPowerOfTwo) {

      SHint8 *potdata = (SHint8 *) malloc(potwidth * potheight * i->fd.bytes);
      SH_RETURN_ERR_IF(!potdata, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

      SH_DEBUG("shUpdateImageTexture: scale texture as power-of-two for OpenGL\n");
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glPixelStorei(GL_PACK_ALIGNMENT, 1);
      glBindTexture(GL_TEXTURE_2D, i->texture);

      gluScaleImage(i->fd.glformat, i->width, i->height, i->fd.gltype,
                    i->data, potwidth, potheight, i->fd.gltype, potdata);

      glTexImage2D(GL_TEXTURE_2D, 0, i->fd.glintformat, potwidth, potheight,
                   0, i->fd.glformat, i->fd.gltype, potdata);

      free(potdata);
      return;
   }

   /* Store pixels to texture */
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glBindTexture(GL_TEXTURE_2D, i->texture);

   // TODO: check this!
   glTexImage2D(GL_TEXTURE_2D, 0, i->fd.glintformat, i->texwidth, i->texheight, 0, i->fd.glformat, i->fd.gltype , i->data);

#ifdef DEBUG
   short center = (i->texwidth*i->texheight)*2+2*i->texwidth;
   SH_DEBUG("shUpdateImageTexture: 0x%x 0x%x 0x%x\n",i->fd.glintformat,i->fd.glformat, i->fd.gltype);
   SH_DEBUG("shUpdateImageTexture: %d %d %d %d\n",i->data[center],i->data[center+1],i->data[center+2],i->data[center+3]);
#endif
}

/*----------------------------------------------------------
 * Creates a new image object and returns the handle to it
 *----------------------------------------------------------*/

VG_API_CALL VGImage
vgCreateImage(VGImageFormat format,
              VGint width, VGint height, VGbitfield allowedQuality)
{
   VG_GETCONTEXT(VG_INVALID_HANDLE);

   /* Reject invalid formats */
   VG_RETURN_ERR_IF(!shIsValidImageFormat(format),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_INVALID_HANDLE);

   /* Reject unsupported formats */
   VG_RETURN_ERR_IF(!shIsSupportedImageFormat(format),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_INVALID_HANDLE);

   /* Reject invalid sizes */
   VG_RETURN_ERR_IF(width <= 0 || width > SH_MAX_IMAGE_WIDTH ||
                    height <= 0 || height > SH_MAX_IMAGE_HEIGHT ||
                    width * height > SH_MAX_IMAGE_PIXELS,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);

   /* Check if byte size exceeds SH_MAX_IMAGE_BYTES */
   SHImageFormatDesc fd;
   shSetupImageFormat(format, &fd);
   VG_RETURN_ERR_IF(width * height * fd.bytes > SH_MAX_IMAGE_BYTES,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);

   /* Reject invalid quality bits */
   VG_RETURN_ERR_IF(allowedQuality &
                    ~(VG_IMAGE_QUALITY_NONANTIALIASED |
                      VG_IMAGE_QUALITY_FASTER | VG_IMAGE_QUALITY_BETTER),
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);

   SH_DEBUG("VGImageformat 0x%x, in decimal %d\n ", format, format);

   /* Create new image object */
   SHImage *i = NULL;
   SH_NEWOBJ(SHImage, i);
   VG_RETURN_ERR_IF(!i, VG_OUT_OF_MEMORY_ERROR, VG_INVALID_HANDLE);
   i->width = width;
   i->height = height;
   i->fd = fd;

   /* Allocate data memory */
   shUpdateImageTextureSize(i);
   i->data = (SHuint8 *) malloc(i->stride * i->texheight);

   if (i->data == NULL) {
      SH_DELETEOBJ(SHImage, i);
      VG_RETURN_ERR(VG_OUT_OF_MEMORY_ERROR, VG_INVALID_HANDLE);
   }

   /* Initialize data by zeroing-out */
   memset(i->data, 1, i->stride * i->texheight);
   shUpdateImageTexture(i, context);

   /* Add to resource list */
   shImageArrayPushBack(&context->images, i);

   VG_RETURN((VGImage) i);
}

VG_API_CALL void
vgDestroyImage(VGImage image)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Check if valid resource */
   SHint index = shImageArrayFind(&context->images, (SHImage *) image);
   VG_RETURN_ERR_IF(index == -1, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* Delete object and remove resource */
   SH_DELETEOBJ(SHImage, (SHImage *) image);
   shImageArrayRemoveAt(&context->images, index);

   VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------
 * Clear given rectangle area in the image data with
 * color set via vgSetfv(VG_CLEAR_COLOR, ...)
 *---------------------------------------------------*/

VG_API_CALL void
vgClearImage(VGImage image, VGint x, VGint y, VGint width, VGint height)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!shIsValidImage(context, image),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* TODO: check if image current render target */

   SHImage *i = (SHImage *) image;
   VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* Nothing to do if target rectangle out of bounds */
   if (x >= i->width || y >= i->height)
      VG_RETURN(VG_NO_RETVAL);
   if (x + width < 0 || y + height < 0)
      VG_RETURN(VG_NO_RETVAL);

   /* Clamp to image bounds */
   SHint ix = SH_MAX(x, 0);
   SHint dx = ix - x;
   SHint iy = SH_MAX(y, 0);
   SHint dy = iy - y;
   width = SH_MIN(width - dx, i->width - ix);
   height = SH_MIN(height - dy, i->height - iy);

   /* Walk pixels and clear */
   SHuint32 packedColor = shPackColor(&context->clearColor, &i->fd);
   SHuint8 fdBytes = i->fd.bytes;
   SHuint8 *data;
   for (SHint Y = iy; Y < iy + height; ++Y) {
      data = i->data + (Y * i->stride + ix * fdBytes);

      for (SHint X = ix; X < ix + width; ++X) {
         shStorePackedColor(data, fdBytes, packedColor);
         data += fdBytes;
      }
   }

   /*
    * SHColor *clearColor = &context->clearColor;
    * for (SHint Y = iy; Y < iy + height; ++Y) {
    *    for (SHint X = ix; X < ix + width; ++X) {
    *       shStorePixelColor(clearColor, i->data, &(i->fd), X, Y, i->stride);
    *    }
    * }
    */

   shUpdateImageTexture(i, context);
   VG_RETURN(VG_NO_RETVAL);
}

/*------------------------------------------------------------
 * Generic function for copying a rectangle area of pixels
 * of size (width,height) among two data buffers. The size of
 * source (swidth,sheight) and destination (dwidth,dheight)
 * images may vary as well as the source coordinates (sx,sy)
 * and destination coordinates(dx, dy).
 *------------------------------------------------------------*/

void
shCopyPixels(SHuint8 * dst, VGImageFormat dstFormat, SHint dstStride,
             const SHuint8 * src, VGImageFormat srcFormat, SHint srcStride,
             SHint dwidth, SHint dheight, SHint swidth, SHint sheight,
             SHint dx, SHint dy, SHint sx, SHint sy,
             SHint width, SHint height)
{
   SHint dxold, dyold;
   const SHuint8 *SD;
   SHuint8 *DD;
   SHColor c;

   SHImageFormatDesc dfd;
   SHImageFormatDesc sfd;

   SH_ASSERT(src != NULL && dst != NULL);

   /* Setup image format descriptors */
   SH_ASSERT(shIsSupportedImageFormat(dstFormat));
   SH_ASSERT(shIsSupportedImageFormat(srcFormat));
   shSetupImageFormat(dstFormat, &dfd);
   shSetupImageFormat(srcFormat, &sfd);

   /*
     In order to optimize the copying loop and remove the
     if statements from it to check whether target pixel
     is in the source and destination surface, we clamp
     copy rectangle in advance. This is quite a tedious
     task though. Here is a picture of the scene. Note that
     (dx,dy) is actually an offset of the copy rectangle
     (clamped to src surface) from the (0,0) point on dst
     surface. A negative (dx,dy) (as in this picture) also
     affects src coords of the copy rectangle which have
     to be readjusted again (sx,sy,width,height).

     src
     *----------------------*
     | (sx,sy)  copy rect   |
     | *-----------*        |
     | |\(dx, dy)  |        |          dst
     | | *------------------------------*
     | | |xxxxxxxxx|        |           |
     | | |xxxxxxxxx|        |           |
     | *-----------*        |           |
     |   |   (width,height) |           |
     *----------------------*           |
     |   |           (swidth,sheight)   |
     *----------------------------------*
     (dwidth,dheight)
   */

   /* Cancel if copy rect out of src bounds */
   if (sx >= swidth || sy >= sheight)
      return;
   if (sx + width < 0 || sy + height < 0)
      return;

   /* Clamp copy rectangle to src bounds */
   sx = SH_MAX(sx, 0);
   sy = SH_MAX(sy, 0);
   width = SH_MIN(width, swidth - sx);
   height = SH_MIN(height, sheight - sy);

   /* Cancel if copy rect out of dst bounds */
   if (dx >= dwidth || dy >= dheight)
      return;
   if (dx + width < 0 || dy + height < 0)
      return;

   /* Clamp copy rectangle to dst bounds */
   dxold = dx;
   dyold = dy;
   dx = SH_MAX(dx, 0);
   dy = SH_MAX(dy, 0);
   sx += dx - dxold;
   sy += dy - dyold;
   width -= dx - dxold;
   height -= dy - dyold;
   width = SH_MIN(width, dwidth - dx);
   height = SH_MIN(height, dheight - dy);

   /* Calculate stride from format if not given */
   if (dstStride == -1)
      dstStride = dwidth * dfd.bytes;
   if (srcStride == -1)
      srcStride = swidth * sfd.bytes;

   if (srcFormat == dstFormat) {

      /* If the stride are the same, we can copy the whole block */
      if (srcStride == dstStride) {
         memcpy(dst, src, width * height * sfd.bytes);
         return ;
      }

      /* Walk pixels and copy */
      for (SHint SY = sy, DY = dy; SY < sy + height; ++SY, ++DY) {
         SD = src + SY * srcStride + sx * sfd.bytes;
         DD = dst + DY * dstStride + dx * dfd.bytes;
         memcpy(DD, SD, width * sfd.bytes);
      }
      return ;
   }

   /* Walk pixels and copy */
   for (SHint SY = sy, DY = dy; SY < sy + height; ++SY, ++DY) {
      SD = src + SY * srcStride + sx * sfd.bytes;
      DD = dst + DY * dstStride + dx * dfd.bytes;

      for (SHint SX = sx, DX = dx; SX < sx + width; ++SX, ++DX) {
         shLoadColor(&c, SD, &sfd);
         shStoreColor(&c, DD, &dfd);
         SD += sfd.bytes;
         DD += dfd.bytes;
      }
   }
}

/*---------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from given data buffer to image surface at destination
 * coordinates (x,y)
 *---------------------------------------------------------*/

VG_API_CALL void
vgImageSubData(VGImage image,
               const void *data, VGint dataStride,
               VGImageFormat dataFormat,
               VGint x, VGint y, VGint width, VGint height)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!shIsValidImage(context, image),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* TODO: check if image current render target */
   SHImage *i = (SHImage *) image;

   /* Reject invalid formats */
   VG_RETURN_ERR_IF(!shIsValidImageFormat(dataFormat),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_NO_RETVAL);

   /* Reject unsupported image formats */
   VG_RETURN_ERR_IF(!shIsSupportedImageFormat(dataFormat),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_NO_RETVAL);

   VG_RETURN_ERR_IF(width <= 0 || height <= 0 || !data,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   VG_RETURN_ERR_IF(SH_IS_NOT_ALIGNED(data), VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   shCopyPixels(i->data, i->fd.vgformat, i->stride,
                data, dataFormat, dataStride,
                i->width, i->height, width, height,
                x, y, 0, 0, width, height);

   shUpdateImageTexture(i, context);
   VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from image surface at source coordinates (x,y) to given
 * data buffer
 *---------------------------------------------------------*/

VG_API_CALL void
vgGetImageSubData(VGImage image,
                  void *data, VGint dataStride,
                  VGImageFormat dataFormat,
                  VGint x, VGint y, VGint width, VGint height)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!shIsValidImage(context, image),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* Reject invalid formats */
   VG_RETURN_ERR_IF(!shIsValidImageFormat(dataFormat),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_NO_RETVAL);

   /* Reject unsupported formats */
   VG_RETURN_ERR_IF(!shIsSupportedImageFormat(dataFormat),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_NO_RETVAL);

   VG_RETURN_ERR_IF(width <= 0 || height <= 0 || !data,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   VG_RETURN_ERR_IF(SH_IS_NOT_ALIGNED(data), VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* TODO: check if image current render target */
   SHImage *img = (SHImage *) image;

   shCopyPixels(data, dataFormat, dataStride,
                img->data, img->fd.vgformat, img->stride,
                width, height, img->width, img->height,
                0, 0, x, x, width, height);

   VG_RETURN(VG_NO_RETVAL);
}

/*----------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from src image surface at source coordinates (sx,sy) to
 * dst image surface at destination cordinates (dx,dy)
 *---------------------------------------------------------*/

VG_API_CALL void
vgCopyImage(VGImage dst, VGint dx, VGint dy,
            VGImage src, VGint sx, VGint sy,
            VGint width, VGint height, VGboolean dither)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!shIsValidImage(context, src) ||
                    !shIsValidImage(context, dst),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* TODO: check if images current render target */

   SHImage *s = (SHImage *) src;
   SHImage *d = (SHImage *) dst;
   VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* In order to perform copying in a cosistent fashion
      we first copy to a temporary buffer and only then to
      destination image */

   /* TODO: rather check first if the image is really
      the same and whether the regions overlap. if not
      we can copy directly */
   if (!shOverlaps(s, d, sx, sy, dx, dy, width, height)) {
      // TODO: CHECK this!!!!
      shCopyPixels(d->data, d->fd.vgformat, d->stride,
                   s->data, s->fd.vgformat, s->stride,
                   d->width, d->height, width, height,
                   dx, dy, 0, 0, width, height);
   } else {
      SHuint8 *pixels = (SHuint8 *) malloc(width * height * s->fd.bytes);
      SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

      shCopyPixels(pixels, s->fd.vgformat, s->stride,
                   s->data, s->fd.vgformat, s->stride,
                   width, height, s->width, s->height,
                   0, 0, sx, sy, width, height);

      shCopyPixels(d->data, d->fd.vgformat, d->stride,
                   pixels, s->fd.vgformat, s->stride,
                   d->width, d->height, width, height,
                   dx, dy, 0, 0, width, height);

      free(pixels);
   }

   shUpdateImageTexture(d, context);
   VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from src image surface at source coordinates (sx,sy) to
 * window surface at destination coordinates (dx,dy)
 *---------------------------------------------------------*/

VG_API_CALL void
vgSetPixels(VGint dx, VGint dy,
            VGImage src, VGint sx, VGint sy, VGint width, VGint height)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!shIsValidImage(context, src),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* TODO: check if image current render target (requires EGL) */

   SHImage *i = (SHImage *) src;
   VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* Setup window image format descriptor */
   /* TODO: this actually depends on the target framebuffer type
      if we really want the copy to be optimized */
   SHImageFormatDesc winfd;
   shSetupImageFormat(VG_sRGBA_8888, &winfd);

   /* OpenGL doesn't allow us to use random stride. We have to
      manually copy the image data and write from a copy with
      normal row length (without power-of-two roundup pixels) */

   SHuint8 *pixels = (SHuint8 *) malloc(width * height * winfd.bytes);
   SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

   shCopyPixels(pixels, winfd.vgformat, -1,
                i->data, i->fd.vgformat, i->stride,
                width, height, i->width, i->height,
                0, 0, sx, sy, width, height);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glRasterPos2i(dx, dy);
   glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
   glRasterPos2i(0, 0);


   /* TODO: try to replace the previous block with this
    * glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    * glPixelStorei(GL_PACK_ALIGNMENT, 1);
    * glBindTexture(GL_TEXTURE_2D, i->texture);
    * glTexSubImage2D(GL_TEXTURE_2D, 0, dx, dy, width, height, i->fd.glintformat, i->fd.gltype, pixels);
    */

   free(pixels);

   VG_RETURN(VG_NO_RETVAL);
}

/*---------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from given data buffer at source coordinates (sx,sy) to
 * window surface at destination coordinates (dx,dy)
 *---------------------------------------------------------*/

VG_API_CALL void
vgWritePixels(const void *data, VGint dataStride,
              VGImageFormat dataFormat,
              VGint dx, VGint dy, VGint width, VGint height)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Reject invalid formats */
   VG_RETURN_ERR_IF(!shIsValidImageFormat(dataFormat),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_NO_RETVAL);

   /* Reject unsupported formats */
   VG_RETURN_ERR_IF(!shIsSupportedImageFormat(dataFormat),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_NO_RETVAL);

   VG_RETURN_ERR_IF(width <= 0 || height <= 0 || !data,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   VG_RETURN_ERR_IF(SH_IS_NOT_ALIGNED(data), VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* Setup window image format descriptor */
   /* TODO: this actually depends on the target framebuffer type
      if we really want the copy to be optimized */
   SHImageFormatDesc winfd;
   shSetupImageFormat(VG_sRGBA_8888, &winfd);

   /* OpenGL doesn't allow us to use random stride. We have to
      manually copy the image data and write from a copy with
      normal row length */

   SHuint8 *pixels = (SHuint8 *) malloc(width * height * winfd.bytes);
   SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

   shCopyPixels(pixels, winfd.vgformat, -1,
                (SHuint8 *) data, dataFormat, dataStride,
                width, height, width, height, 0, 0, 0, 0, width, height);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
   glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
   glPixelStorei(GL_PACK_SWAP_BYTES, 1);
#else
   glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
   glPixelStorei(GL_PACK_SWAP_BYTES, 0);
#endif

   glRasterPos2i(dx, dy);
   glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
   glRasterPos2i(0, 0);

   free(pixels);

   VG_RETURN(VG_NO_RETVAL);
}

/*-----------------------------------------------------------
 * Copies a rectangle area of pixels of size (width, height)
 * from window surface at source coordinates (sx, sy) to
 * image surface at destination coordinates (dx, dy)
 *-----------------------------------------------------------*/

VG_API_CALL void
vgGetPixels(VGImage dst, VGint dx, VGint dy,
            VGint sx, VGint sy, VGint width, VGint height)
{

   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!shIsValidImage(context, dst),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* TODO: check if image current render target */

   SHImage *i = (SHImage *) dst;
   VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* Setup window image format descriptor */
   /* TODO: this actually depends on the target framebuffer type
      if we really want the copy to be optimized */
   SHImageFormatDesc winfd;
   shSetupImageFormat(VG_sRGBA_8888, &winfd);

   /* OpenGL doesn't allow us to read to random destination
      coordinates nor using random stride. We have to
      read first and then manually copy to the image data */

   SHuint8 *pixels = (SHuint8 *) malloc(width * height * winfd.bytes);
   SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

   glPixelStorei(GL_PACK_ALIGNMENT, 1);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
   glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
   glPixelStorei(GL_PACK_SWAP_BYTES, 1);
#else
   glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
   glPixelStorei(GL_PACK_SWAP_BYTES, 0);
#endif

   glReadPixels(sx, sy, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

   /* FIXME: we shouldnt be reading alpha */
   for (SHint k = 3; k < i->width * i->height * 4; k += 4)
      pixels[k] = 255;

   shCopyPixels(i->data, i->fd.vgformat, i->stride,
                pixels, winfd.vgformat, -1,
                i->width, i->height, width, height,
                dx, dy, 0, 0, width, height);

   free(pixels);

   shUpdateImageTexture(i, context);
   VG_RETURN(VG_NO_RETVAL);
}

/*-----------------------------------------------------------
 * Copies a rectangle area of pixels of size (width, height)
 * from window surface at source coordinates (sx, sy) to
 * to given output data buffer.
 *-----------------------------------------------------------*/

VG_API_CALL void
vgReadPixels(void *data, VGint dataStride,
             VGImageFormat dataFormat,
             VGint sx, VGint sy, VGint width, VGint height)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Reject invalid formats */
   VG_RETURN_ERR_IF(!shIsValidImageFormat(dataFormat),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_NO_RETVAL);

   /* Reject unsupported image formats */
   VG_RETURN_ERR_IF(!shIsSupportedImageFormat(dataFormat),
                    VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_NO_RETVAL);

   VG_RETURN_ERR_IF(width <= 0 || height <= 0 || !data,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   VG_RETURN_ERR_IF(SH_IS_NOT_ALIGNED(data), VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* Setup window image format descriptor */
   /* TODO: this actually depends on the target framebuffer type
      if we really want the copy to be optimized */
   SHImageFormatDesc winfd;
   shSetupImageFormat(VG_sRGBA_8888, &winfd);

   /* OpenGL doesn't allow random data stride. We have to
      read first and then manually copy to the output buffer */

   SHuint8 *pixels = (SHuint8 *) malloc(width * height * winfd.bytes);
   SH_RETURN_ERR_IF(!pixels, VG_OUT_OF_MEMORY_ERROR, SH_NO_RETVAL);

   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glReadPixels(sx, sy, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

   shCopyPixels(data, dataFormat, dataStride,
                pixels, winfd.vgformat, -1,
                width, height, width, height, 0, 0, 0, 0, width, height);

   free(pixels);

   VG_RETURN(VG_NO_RETVAL);
}

/*----------------------------------------------------------
 * Copies a rectangle area of pixels of size (width,height)
 * from window surface at source coordinates (sx,sy) to
 * windows surface at destination cordinates (dx,dy)
 *---------------------------------------------------------*/

VG_API_CALL void
vgCopyPixels(VGint dx, VGint dy,
             VGint sx, VGint sy, VGint width, VGint height)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(width <= 0 || height <= 0,
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
   glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
   glPixelStorei(GL_PACK_SWAP_BYTES, 1);
#else
   glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
   glPixelStorei(GL_PACK_SWAP_BYTES, 0);
#endif

   glRasterPos2i(dx, dy);
   glCopyPixels(sx, sy, width, height, GL_COLOR);
   glRasterPos2i(0, 0);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGImage
vgChildImage(VGImage parent, VGint x, VGint y, VGint width, VGint height)
{
   return VG_INVALID_HANDLE;
}

VG_API_CALL VGImage
vgGetParent(VGImage image)
{
   return VG_INVALID_HANDLE;
}

VG_API_CALL void
vgColorMatrix(VGImage dst, VGImage src, const VGfloat * matrix)
{
   VG_GETCONTEXT(VG_NO_RETVAL);
   VG_RETURN_ERR_IF(!shIsValidImage(context, src) || !shIsValidImage(context, dst), VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   SHImage* d = (SHImage*) dst;
   SHImage* s = (SHImage*) src;

   VG_RETURN_ERR_IF(shOverlaps(s, d, 0, 0, 0, 0, s->width, s->height ), VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
   VG_RETURN_ERR_IF(!matrix || SH_IS_NOT_ALIGNED(matrix), VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   SHint32 w = SH_MIN(d->width, s->width);
   SHint32 h = SH_MIN(d->height, s->height);
   SH_ASSERT(w > 0 && h > 0);

   SHColor sc; // source color
   SHColor dc; // destination color
   SHColor tmpcolor;
   SHImageFormatDesc sfd = s->fd;
   SHImageFormatDesc dfd = d->fd;

   SHuint8 *SD;
   SHuint8 *DD;

   SHint stride = s->stride;

   VGbitfield channelMask = context->filterChannelMask;

   if (sfd.glformat == GL_LUMINANCE) {
      for (SHint SY = 0, DY = 0; SY < h; ++SY, ++DY) {
         SD = s->data + SY * stride + sfd.bytes;
         DD = d->data + DY * stride + dfd.bytes;

         for (SHint SX = 0, DX = 0; SX < w; ++SX, ++DX) {
            shLoadColor(&sc, SD, &sfd);

            dc.r = matrix[0] * sc.r + matrix[1] * sc.g + matrix[2] * sc.b + matrix[3] * sc.a + matrix[4];
            dc.g = matrix[5] * sc.r + matrix[6] * sc.g + matrix[7] * sc.b + matrix[8] * sc.a + matrix[9];
            dc.b = matrix[10] * sc.r + matrix[11] * sc.g + matrix[12] * sc.b + matrix[13] * sc.a + matrix[14];
            dc.a = matrix[15] * sc.r + matrix[16] * sc.g + matrix[17] * sc.b + matrix[18] * sc.a + matrix[19];

            CCLAMP(dc);
            shStoreColor(&dc, DD, &dfd);

            SD += sfd.bytes;
            DD += dfd.bytes;
         }
      }
   } else {
      for (SHint SY = 0, DY = 0; SY < h; ++SY, ++DY) {
         SD = s->data + SY * stride + sfd.bytes;
         DD = d->data + DY * stride + dfd.bytes;

         for (SHint SX = 0, DX = 0; SX < w; ++SX, ++DX) {
            shLoadColor(&sc, SD, &sfd);

            tmpcolor.r = matrix[0] * sc.r + matrix[1] * sc.g + matrix[2] * sc.b + matrix[3] * sc.a + matrix[4];
            tmpcolor.g = matrix[5] * sc.r + matrix[6] * sc.g + matrix[7] * sc.b + matrix[8] * sc.a + matrix[9];
            tmpcolor.b = matrix[10] * sc.r + matrix[11] * sc.g + matrix[12] * sc.b + matrix[13] * sc.a + matrix[14];
            tmpcolor.a = matrix[15] * sc.r + matrix[16] * sc.g + matrix[17] * sc.b + matrix[18] * sc.a + matrix[19];

            CCLAMP(tmpcolor);

            dc = sc;

            if (channelMask & VG_RED)
               dc.r = tmpcolor.r;
            if (channelMask & VG_GREEN)
               dc.g = tmpcolor.g;
            if (channelMask & VG_BLUE)
               dc.b = tmpcolor.b;
            if (channelMask & VG_ALPHA)
               dc.a = tmpcolor.a;

            shStoreColor(&dc, DD, &dfd);

            SD += sfd.bytes;
            DD += dfd.bytes;
         }
      }
   }

   shUpdateImageTexture(d, context);
   VG_RETURN(VG_NO_RETVAL);
}


/*
 * Return a pointer to a color into "data" or a pointe to the same color of *edge
 */
static inline const SHColor *
shGetTiledPixel(SHint x, SHint y, SHint w, SHint h, VGTilingMode tilingMode, const SHColor * restrict data , const SHColor * restrict edge)
{
   const SHColor *c;
   if  (x >= 0 && x < w && y >= 0 && y < h) {
      c = &(data[y * w + x]);
      return c;
   }

   switch (tilingMode) {
      case VG_TILE_FILL:
         c = edge;
         break;

      case VG_TILE_PAD:
         x = SH_MIN(SH_MAX(x, 0), w - 1);
         y = SH_MIN(SH_MAX(y, 0), h - 1);
         SH_ASSERT(x >= 0 && x < w && y >= 0 && y < h);
         c = &(data[y * w + x]);
         break;

      case VG_TILE_REPEAT:
         x = shIntMod(x, w);
         y = shIntMod(y, h);
         SH_ASSERT(x >= 0 && x < w && y >= 0 && y < h);
         c = &(data[y * w + x]);
         break;

      case VG_TILE_REFLECT:
      default:
         x = shIntMod(x, w * 2);
         y = shIntMod(y, h * 2);
         if (x >= w) {
            x = w * 2 - 1 - x;
         }
         if (y >= h) {
            y = h * 2 - 1 - y;
         }
         SH_ASSERT(x >= 0 && x < w && y >= 0 && y < h);
         c = &(data[y * w + x]);
         break;
      }
   return c;
}


VG_API_CALL void
vgConvolve(VGImage dst, VGImage src,
           VGint kernelWidth, VGint kernelHeight,
           VGint shiftX, VGint shiftY,
           const VGshort * kernel,
           VGfloat scale, VGfloat bias, VGTilingMode tilingMode)
{
   VG_GETCONTEXT(VG_NO_RETVAL);
   VG_RETURN_ERR_IF(!shIsValidImage(context, src) || !shIsValidImage(context, dst), VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
   VG_RETURN_ERR_IF(kernel == NULL ||
                    SH_IS_NOT_ALIGNED(kernel) ||
                    kernelWidth <= 0 ||
                    kernelHeight <= 0 ||
                    kernelWidth > VG_MAX_KERNEL_SIZE ||
                    kernelHeight > VG_MAX_KERNEL_SIZE,
                    VG_ILLEGAL_ARGUMENT_ERROR,
                    VG_NO_RETVAL);
   VG_RETURN_ERR_IF(tilingMode < VG_TILE_FILL || tilingMode > VG_TILE_REFLECT, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   SHImage *d = (SHImage*) dst;
   SHImage *s = (SHImage*) src;
   VG_RETURN_ERR_IF(
      shOverlaps(s, d, 0, 0, 0, 0, s->width, s->height),
      VG_ILLEGAL_ARGUMENT_ERROR,
      VG_NO_RETVAL);

   SHint w = SH_MIN(d->width, s->width);
   SHint h = SH_MIN(d->height, s->height);
   SH_ASSERT(w > 0 && h > 0);

   VGbitfield channelMask = context->filterChannelMask;
   SHColor edge = context->tileFillColor;
   // TODO: replace malloc with a "safe" malloc that always test return pointer with assert
   SHColor *tmpColors = (SHColor *) malloc(s->width * s->height * sizeof(SHColor));
   SH_ASSERT(tmpColors != NULL);

   // copy source image region to tmp buffer
   SHColor c;
   for(SHint y = 0; y < h; y++) {
      for(SHint x = 0; x < w; x++) {
         shLoadPixelColor(&c, s->data, &(s->fd), x, y, s->stride);
         tmpColors[y * w + x] = c;
      }
   }

   SHint x, y, kx, ky;
   const SHColor *tmpc;
   SHfloat kernelValue;
   SHColor cs; // color to store in the destination image
   for (SHint i = 0; i < h; ++i) {
      for (SHint j = 0; j < w; ++j) {
         SHColor sum = {.r = 0.0f, .g = 0.0f, .b =0.0f, .a =0.0f};
         for (SHint ki = 0; ki < kernelHeight; ++ki) {
            for (SHint kj = 0; kj < kernelWidth; ++kj) {
               x = j + kj - shiftX;
               y = i + ki - shiftY;
               tmpc = shGetTiledPixel(x, y, w, h, tilingMode, tmpColors, &edge);
               kx = kernelWidth - kj - 1;
               ky = kernelHeight - ki - 1;
               SH_ASSERT(kx >= 0 && kx < kernelWidth && ky >= 0 && ky < kernelHeight);
               kernelValue = (SHfloat) kernel[ky * kernelWidth + kx];
               CMULANDADDC(sum, *tmpc, kernelValue );
            }
         }
         CMUL(sum, scale);
         CADD(sum, bias, bias, bias, bias);
         CCLAMP(sum);

         cs = tmpColors[i * w + j];

         if (channelMask & (VG_RED | VG_GREEN | VG_RED | VG_ALPHA)) {
            cs = sum;
         } else {

            if (channelMask & VG_RED)
               cs.r = sum.r;

            if (channelMask & VG_GREEN)
               cs.g = sum.g;

            if (channelMask & VG_BLUE)
               cs.b = sum.b;

            if (channelMask & VG_ALPHA)
               cs.a = sum.a;
         }

         shStorePixelColor(&cs, d->data, &(d->fd), j, i, d->stride);
      }
   }
   free(tmpColors);

   shUpdateImageTexture(d, context);
   VG_RETURN(VG_NO_RETVAL);

}

VG_API_CALL void
vgSeparableConvolve(VGImage dst, VGImage src,
                    VGint kernelWidth,
                    VGint kernelHeight,
                    VGint shiftX, VGint shiftY,
                    const VGshort * kernelX,
                    const VGshort * kernelY,
                    VGfloat scale, VGfloat bias, VGTilingMode tilingMode)
{
   VG_GETCONTEXT(VG_NO_RETVAL);
   VG_RETURN_ERR_IF(!shIsValidImage(context, src) || !shIsValidImage(context, dst), VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
   VG_RETURN_ERR_IF(kernelX == NULL || kernelX == NULL ||
                    SH_IS_NOT_ALIGNED(kernelX) || SH_IS_NOT_ALIGNED(kernelY) ||
                    kernelWidth <= 0 ||
                    kernelHeight <= 0 ||
                    kernelWidth > SH_MAX_KERNEL_SIZE ||
                    kernelHeight > SH_MAX_KERNEL_SIZE,
                    VG_ILLEGAL_ARGUMENT_ERROR,
                    VG_NO_RETVAL);
   VG_RETURN_ERR_IF(tilingMode < VG_TILE_FILL || tilingMode > VG_TILE_REFLECT, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   SHImage *d = (SHImage*) dst;
   SHImage *s = (SHImage*) src;
   VG_RETURN_ERR_IF(
      shOverlaps(s, d, 0, 0, 0, 0, s->width, s->height),
      VG_ILLEGAL_ARGUMENT_ERROR,
      VG_NO_RETVAL);

   SHint w = SH_MIN(d->width, s->width);
   SHint h = SH_MIN(d->height, s->height);
   SH_ASSERT(w > 0 && h > 0);

   VGbitfield channelMask = context->filterChannelMask;
   SHColor edge = context->tileFillColor;

   SHColor *tmpColors = (SHColor *) malloc(w * h * sizeof(SHColor));
   SH_ASSERT(tmpColors != NULL);
   // copy source image region to tmp buffer
   SHColor c;
   for (SHint y = 0; y < h; ++y) {
      for (SHint x = 0; x < w; ++x) {
         shLoadPixelColor(&c, s->data, &(s->fd), x, y, s->stride);
         tmpColors[y * w + x] = c;
      }
   }

   // TODO: mmh, I guess this second buffer can be removed.
   SHColor *tmpColors2 = (SHColor *) malloc(w * h * sizeof(SHColor));
   SH_ASSERT(tmpColors2 != NULL);

   const SHColor *tmpc; // pointer to a color from tempColors array
   SHint x, kx;
   for (SHint i = 0; i < h; ++i) {
      for (SHint j = 0; j < w; ++j) {
         SHColor sum = {.r = 0.0f, .g = 0.0f, .b =0.0f, .a =0.0f};
         for (SHint k = 0; k < kernelWidth; ++k) {
            x = j + k - shiftX;
            tmpc = shGetTiledPixel(x, i, w, h, tilingMode, tmpColors, &edge);
            kx = kernelWidth - k - 1;
            SH_ASSERT(kx >= 0 && kx < kernelWidth);
            CMULANDADDC(sum, *tmpc, (SHfloat) kernelX[kx]);
         }
         CCLAMP(sum);
         tmpColors2[i * w + j] = sum;
      }
   }

   // convolve the edge color if needed
   if (tilingMode == VG_TILE_FILL) {
      SHColor sum = {.r = 0.0f, .g = 0.0f, .b =0.0f, .a =0.0f};
      for (SHint i = 0; i < kernelWidth; ++i) {
         CMULANDADDC(sum, edge, (SHfloat) kernelX[i]);
      }
      edge = sum;
      CCLAMP(edge);
   }

   // vertical pass
   SHint y, ky;
   SHColor cs; // color to store in the destination image
   for (SHint i = 0; i < h; ++i) {
      for (SHint j = 0; j < w; ++j) {
         SHColor sum = {.r = 0.0f, .g = 0.0f, .b =0.0f, .a =0.0f};
         for (SHint k = 0; k < kernelHeight; ++k) {
            y = i + k - shiftY;
            tmpc = shGetTiledPixel(j, y, w, h, tilingMode, tmpColors2, &edge);
            ky = kernelHeight - k - 1;
            SH_ASSERT(ky >= 0 && ky < kernelHeight);
            CMULANDADDC(sum, *tmpc, (SHfloat) kernelY[ky]);
         }
         CMUL(sum, scale);
         CADD(sum, bias, bias, bias, bias);
         CCLAMP(sum);
         cs = tmpColors[i * w + j];

         if (channelMask & (VG_RED | VG_GREEN | VG_RED | VG_ALPHA)) {
            cs = sum;
         } else {

            if (channelMask & VG_RED)
               cs.r = sum.r;

            if (channelMask & VG_GREEN)
               cs.g = sum.g;

            if (channelMask & VG_BLUE)
               cs.b = sum.b;

            if (channelMask & VG_ALPHA)
               cs.a = sum.a;
         }

         shStorePixelColor(&cs, d->data, &(d->fd), j, i, d->stride);
      }
   }

   free(tmpColors);
   free(tmpColors2);
   shUpdateImageTexture(d, context);
   VG_RETURN(VG_NO_RETVAL);
}

static inline SHfloat *
shMakeGaussianBlurKernel(int kernelElement, SHfloat expScale, SHfloat * restrict scale, int * restrict kernelSize)
{
   *kernelSize = kernelElement * 2 + 1;
   SHfloat *kernel = (SHfloat *) malloc((*kernelSize) * sizeof(SHfloat));
   SH_ASSERT(kernel != NULL); // Should return an error ?

   SHfloat32 tmp = *scale;
   for (SHint i = 0; i < *kernelSize; ++i) {
      kernel[i] = expf(((i - kernelElement) * (i - kernelElement)) * expScale);
      tmp += kernel[i];
   }
   *scale = 1.0f / tmp;
   return kernel;
}

VG_API_CALL void
vgGaussianBlur(VGImage dst, VGImage src,
               VGfloat stdDeviationX,
               VGfloat stdDeviationY, VGTilingMode tilingMode)
{
   VG_GETCONTEXT(VG_NO_RETVAL);
   VG_RETURN_ERR_IF(!shIsValidImage(context, src) || !shIsValidImage(context, dst), VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);
   VG_RETURN_ERR_IF(tilingMode < VG_TILE_FILL || tilingMode > VG_TILE_REFLECT, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);
   VG_RETURN_ERR_IF(stdDeviationX > SH_MAX_GAUSSIAN_STD_DEVIATION || stdDeviationY > SH_MAX_GAUSSIAN_STD_DEVIATION, VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   SHImage *d = (SHImage*) dst;
   SHImage *s = (SHImage*) src;
   VG_RETURN_ERR_IF(
      shOverlaps(s, d, 0, 0, 0, 0, s->width, s->height),
      VG_ILLEGAL_ARGUMENT_ERROR,
      VG_NO_RETVAL);

   // TODO: mask color with channelMask
   VGbitfield channelMask = context->filterChannelMask;

   // gaussian blur only of the images intersection
   SHint w = SH_MIN(d->width, s->width);
   SHint h = SH_MIN(d->height, s->height);
   SH_ASSERT(w > 0 && h > 0);

   // for now we assume that both image have the same image formats
   SHfloat expScaleX = -1.0f / (2.0f * (stdDeviationX * stdDeviationX));
   SHfloat expScaleY = -1.0f / (2.0f * (stdDeviationY * stdDeviationY));

   SHint kernelWidth = (SHint)(stdDeviationX *  4.0f + 1.0f);
   SHint kernelHeight = (SHint)(stdDeviationY * 4.0f + 1.0f);

   SHfloat scaleX = 0.0f , scaleY = 0.0f;
   SHfloat *kernelX, *kernelY;
   SHint kernelXSize, kernelYSize;

   kernelX = shMakeGaussianBlurKernel(kernelWidth, expScaleX, &scaleX, &kernelXSize);
   kernelY = shMakeGaussianBlurKernel(kernelHeight, expScaleY, &scaleY, &kernelYSize);

   SHColor edge = context->tileFillColor;
   SHColor *tmpColors = (SHColor *) malloc(w * h * sizeof(SHColor));
   SH_ASSERT(tmpColors != NULL);

   // copy source image region to tmp buffer
   SHColor c;
   for (SHint y = 0; y < h; ++y) {
      for (SHint x = 0; x < w; ++x) {
         shLoadPixelColor(&c, s->data, &(s->fd), x, y, s->stride);
         tmpColors[y * w + x] = c;
      }
   }

   const SHColor *tmpc;
   // horizontal pass
   SHint x;
   for (SHint i = 0; i < h; ++i) {
      for (SHint j = 0; j < w; ++j) {
         SHColor sum = {.r = 0.0f, .g = 0.0f, .b =0.0f, .a =0.0f};
         for (SHint k = 0; k < kernelXSize; ++k) {
            x = j + k - kernelWidth;
            // TODO: can I merge shGetTiledPixel with the previous tmp buffer copy?
            tmpc = shGetTiledPixel(x, i, w, h, tilingMode, tmpColors, &edge);
            // Multiply tmpc with kernelX[k] then add with color "sum"
            CMULANDADDC(sum, *tmpc, kernelX[k]);
         }
         CMUL(sum, scaleX);
         tmpColors[i * w + j] = sum;
      }
   }

   // vertical pass
   SHint y;
   for (SHint i = 0; i < h; ++i) {
      for (SHint j = 0; j < w; ++j) {
         SHColor sum = {.r = 0.0f, .g = 0.0f, .b =0.0f, .a =0.0f};
         for (SHint k = 0; k < kernelYSize; ++k) {
            y = i + k - kernelHeight;
            tmpc = shGetTiledPixel(j, y, w, h, tilingMode, tmpColors, &edge);
            // Multiply tmpc with kernelY[k] then add with color "sum"
            CMULANDADDC(sum, *tmpc, kernelY[k]);
         }
         CMUL(sum, scaleY);
         CCLAMP(sum);
         shStorePixelColor(&sum, d->data, &(d->fd), j, i, d->stride);
      }
   }
   free(tmpColors);
   free(kernelX);
   free(kernelY);

   shUpdateImageTexture(d, context);
   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgLookup(VGImage dst, VGImage src,
         const VGubyte * redLUT,
         const VGubyte * greenLUT,
         const VGubyte * blueLUT,
         const VGubyte * alphaLUT,
         VGboolean outputLinear, VGboolean outputPremultiplied)
{
   VG_GETCONTEXT(VG_NO_RETVAL);
   VG_RETURN_ERR_IF(!shIsValidImage(context, src) || !shIsValidImage(context, dst), VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   SHImage *d = (SHImage*) dst;
   SHImage *s = (SHImage*) src;
   VG_RETURN_ERR_IF(
      shOverlaps(s, d, 0, 0, 0, 0, s->width, s->height),
      VG_ILLEGAL_ARGUMENT_ERROR,
      VG_NO_RETVAL);

   VG_RETURN_ERR_IF(
      redLUT == NULL && greenLUT == NULL && blueLUT == NULL && alphaLUT == NULL,
      VG_ILLEGAL_ARGUMENT_ERROR,
      VG_NO_RETVAL);

   SHint w = SH_MIN(d->width, s->width);
   SHint h = SH_MIN(d->height, s->height);
   SH_ASSERT(w > 0 && h > 0);

   SHColor cl, cs;
   if (s->fd.glformat == GL_LUMINANCE) {
      for (SHint y = 0; y < h; y++) {
         for (SHint x = 0; x < w; x++) {
            shLoadPixelColor(&cl, s->data, &(s->fd), x, y, s->stride);

            cs.r = shInt2ColorComponent(redLUT[shColorComponent2Int(cl.r)]);
            cs.g = shInt2ColorComponent(greenLUT[shColorComponent2Int(cl.g)]);
            cs.b = shInt2ColorComponent(blueLUT[shColorComponent2Int(cl.b)]);
            cs.a = shInt2ColorComponent(alphaLUT[shColorComponent2Int(cl.a)]);

            CCLAMP(cs);
            shStorePixelColor(&cs, d->data, &(d->fd), x, y, d->stride);
         }
      }
   } else {
      VGbitfield channelMask = context->filterChannelMask;
      for (SHint y = 0; y < h; y++) {
         for (SHint x = 0; x < w; x++) {
            shLoadPixelColor(&cl, s->data, &(s->fd), x, y, s->stride);
            cs = cl;

            if (channelMask & VG_RED)
               cs.r = shInt2ColorComponent(redLUT[shColorComponent2Int(cl.r)]);

            if (channelMask & VG_GREEN)
               cs.g = shInt2ColorComponent(greenLUT[shColorComponent2Int(cl.g)]);

            if (channelMask & VG_BLUE)
               cs.b = shInt2ColorComponent(blueLUT[shColorComponent2Int(cl.b)]);

            if (channelMask & VG_ALPHA)
               cs.a = shInt2ColorComponent(alphaLUT[shColorComponent2Int(cl.a)]);

            CCLAMP(cs);
            shStorePixelColor(&cs, d->data, &(d->fd), x, y, d->stride);
         }
      }
   }
   shUpdateImageTexture(d, context);
   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgLookupSingle(VGImage dst, VGImage src,
               const VGuint * lookupTable,
               VGImageChannel sourceChannel,
               VGboolean outputLinear, VGboolean outputPremultiplied)
{
   VG_GETCONTEXT(VG_NO_RETVAL);
   VG_RETURN_ERR_IF(!shIsValidImage(context, src) || !shIsValidImage(context, dst), VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   SHImage *d = (SHImage*) dst;
   SHImage *s = (SHImage*) src;
   VG_RETURN_ERR_IF(
      shOverlaps(s, d, 0, 0, 0, 0, s->width, s->height),
      VG_ILLEGAL_ARGUMENT_ERROR,
      VG_NO_RETVAL);

   VG_RETURN_ERR_IF(
      lookupTable == NULL,
      VG_ILLEGAL_ARGUMENT_ERROR,
      VG_NO_RETVAL);

   SHint w = SH_MIN(d->width, s->width);
   SHint h = SH_MIN(d->height, s->height);
   SH_ASSERT(w > 0 && h > 0);

   // TODO: check this
   if (s->fd.glformat == GL_LUMINANCE) {
      sourceChannel = VG_RED;
   } else if (s->fd.bytes == 1) {
      if (s->fd.vgformat != VG_A_8) {
         sourceChannel = VG_RED;
      } else {
         sourceChannel = VG_ALPHA;
      }
   }

   VGbitfield channelMask = context->filterChannelMask;
   SHColor cl; // color loaded from source image
   SHColor cs; // color to store in the destination image
   SHuint32 tmp;
   SHint e; // element to get from lookupTable
   for (SHint y = 0; y < h; y++) {
      for (SHint x = 0; x < w; x++) {
         shLoadPixelColor(&cl, s->data, &(s->fd), x, y, s->stride);
         switch(sourceChannel) {
         case VG_RED:
            e = shColorComponent2Int(cl.r);
            break;
         case VG_GREEN:
            e = shColorComponent2Int(cl.g);
            break;
         case VG_BLUE:
            e = shColorComponent2Int(cl.b);
            break;
         default:
            SH_ASSERT(sourceChannel == VG_ALPHA);
            e = shColorComponent2Int(cl.a);
            break;
         }

         tmp = lookupTable[e];
         cs = cl;

         if (channelMask & VG_RED)
            cs.r = shInt2ColorComponent(tmp >> 24);

         if (channelMask & VG_GREEN)
            cs.g = shInt2ColorComponent(tmp >> 16);

         if (channelMask & VG_BLUE)
            cs.b = shInt2ColorComponent(tmp >> 8);

         if (channelMask & VG_ALPHA)
            cs.a = shInt2ColorComponent(tmp);

         CCLAMP(cs);
         shStorePixelColor(&cs, d->data, &(d->fd), x, y, d->stride);
      }
   }
   shUpdateImageTexture(d, context);
   VG_RETURN(VG_NO_RETVAL);
}
