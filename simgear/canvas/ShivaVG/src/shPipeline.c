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
#include "shDefs.h"
#include "shExtensions.h"
#include "shContext.h"
#include "shPath.h"
#include "shImage.h"
#include "shGeometry.h"
#include "shPaint.h"
#include "shCommons.h"

static void
shPremultiplyFramebuffer(void)
{
   /* Multiply target color with its own alpha */
   glBlendFunc(GL_ZERO, GL_DST_ALPHA);
}

static void
shUnpremultiplyFramebuffer(void)
{
   /* TODO: hmmmm..... any idea? */
}


/*-----------------------------------------------------------
 * Set the render quality.
 *-----------------------------------------------------------*/
static inline void
shSetRenderQualityGL(VGRenderingQuality quality)
{
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
   switch (quality) {
   case VG_RENDERING_QUALITY_NONANTIALIASED:
      glDisable(GL_LINE_SMOOTH);
      glDisable(GL_MULTISAMPLE);
      break;
   case VG_RENDERING_QUALITY_FASTER:
      glEnable(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
      glDisable(GL_MULTISAMPLE);
      break;
   case VG_RENDERING_QUALITY_BETTER:
      glEnable(GL_LINE_SMOOTH);
      glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
      glEnable(GL_MULTISAMPLE);
      break;
   default:
      glDisable(GL_LINE_SMOOTH);
      glEnable(GL_MULTISAMPLE);
      break;
   }
}

static void
updateBlendingStateGL(VGContext * restrict c, int alphaIsOne)
{
   // WORK ONLY WITH STRAIGHT COLOR
   /* Most common drawing mode (SRC_OVER with alpha=1)
      as well as SRC is optimized by turning OpenGL
      blending off. In other cases its turned on. */
   // Ok la roba giusta sta qui http://www.realtimerendering.com/blog/gpus-prefer-premultiplication/
   // e qui https://stackoverflow.com/questions/19674740/opengl-es2-premultiplied-vs-straight-alpha-blending#37869033
   // ad esempio per src over glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
   // bisogna memorizzare se la texture corrente è in formato pre-multiplied o meno. Oppure bisogna trasformare i premultiplied in unpremultiplied
   // meglio ancora sarebbe convertire tutti i formati unpremultiplied in premultiplied in modo da rendere più semplice sia il blending sia il filtering delle immagini
   // quello giusto è questo: http://apoorvaj.io/alpha-compositing-opengl-blending-and-premultiplied-alpha.html
   SH_DEBUG("updateBlendingStateGL(): alphaIsOne = %d\n", alphaIsOne);
   SH_ASSERT(c != NULL);
   switch (c->blendMode) {
   case VG_BLEND_SRC:
      // ensure blend equation set to default
      glEnable(GL_BLEND);
      // OK
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      // premultiplied and non-premultiplied are equals
      // OK
      glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);

      break;

   case VG_BLEND_SRC_IN:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      // OK
      glBlendFuncSeparate(GL_DST_ALPHA, GL_ZERO, GL_DST_ALPHA, GL_ZERO);

      break;

   case VG_BLEND_DST_IN:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      // OK
      glBlendFuncSeparate(GL_ZERO, GL_SRC_ALPHA, GL_ZERO, GL_SRC_ALPHA);
      break;

   case VG_BLEND_SRC_OUT_SH:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      /*
       * glBlendEquation(GL_FUNC_ADD);
       * glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ZERO);
       */
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_ZERO, GL_ONE_MINUS_DST_ALPHA, GL_ZERO);

      break;

   case VG_BLEND_DST_OUT_SH:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      /*
       * glBlendEquation(GL_FUNC_ADD);
       * glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
       */
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
      break;

   case VG_BLEND_SRC_ATOP_SH:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      /*
       * glBlendEquation(GL_FUNC_ADD);
       * glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
       */
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      break;

   case VG_BLEND_DST_ATOP_SH:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      /*
       * glBlendEquation(GL_FUNC_ADD);
       * glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA);
       */
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA);
      break;

   case VG_BLEND_SRC_OVER:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      // OK
      glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      break;

   case VG_BLEND_DST_OVER:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      /*
       * glBlendEquation(GL_FUNC_ADD);
       * glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
       */
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      // OK
      glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, GL_ONE);
      break;

   case VG_BLEND_ADDITIVE:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      /*
       * glBlendEquation(GL_FUNC_ADD);
       * /\*
       *  * glBlendFunc(GL_ONE, GL_ONE);
       *  *\/
       * glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
       */
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      glBlendFuncSeparate(GL_SRC_ALPHA, GL_DST_ALPHA, GL_SRC_ALPHA, GL_DST_ALPHA);
      break;

   case VG_BLEND_MULTIPLY:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      // almost OK
      glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      break;

   case VG_BLEND_SCREEN:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      // almost OK
      glBlendFuncSeparate(GL_ONE_MINUS_DST_COLOR, GL_ONE, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      break;

   case VG_BLEND_DARKEN:
      /*
       * glBlendEquationSeparate(GL_MIN, GL_MIN);
       */
      /*
       * glBlendFuncSeparate(GL_SRC_ALPHA, GL_DST_ALPHA, GL_SRC_ALPHA, GL_DST_ALPHA);
       */
      /*
       * glBlendFuncSeparate(GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
       */

      /*
       * glBlendFuncSeparate(GL_SRC_ALPHA, GL_DST_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
       */
      glEnable(GL_BLEND);
      glBlendFuncSeparate(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glBlendEquationSeparate(GL_MIN, GL_FUNC_ADD);
      break;

   case VG_BLEND_LIGHTEN:
      glEnable(GL_BLEND);
      glBlendEquationSeparate(GL_MAX, GL_MAX);
      // almost Ok
      glBlendFuncSeparate(GL_SRC_ALPHA, GL_DST_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      break;
   default:
      glEnable(GL_BLEND);
      // ensure blend equation set to default
      glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
      if (alphaIsOne) {
         glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
      } else {
         glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      }
      break;
   };

}

/*-----------------------------------------------------------
 * Draws the triangles representing the stroke of a path.
 *-----------------------------------------------------------*/

static inline void
shDrawStroke(SHPath * restrict p)
{
   SH_ASSERT(p != NULL);
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(2, GL_FLOAT, 0, p->stroke.items);
   glDrawArrays(GL_TRIANGLES, 0, p->stroke.size);
   glDisableClientState(GL_VERTEX_ARRAY);
}

/*-----------------------------------------------------------
 * Draws the subdivided vertices in the OpenGL mode given
 * (this could be VG_TRIANGLE_FAN or VG_LINE_STRIP).
 *-----------------------------------------------------------*/

static void
shDrawVertices(SHPath * restrict p, GLenum mode)
{
   SH_ASSERT(p != NULL);
   /* We separate vertex arrays by contours to properly
      handle the fill modes */
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(2, GL_FLOAT, sizeof(SHVertex), p->vertices.items);

   SHint start = 0;
   SHint size = 0;
   while (start < p->vertices.size) {
      size = p->vertices.items[start].flags;
      glDrawArrays(mode, start, size);
      start += size;
   }
   glDisableClientState(GL_VERTEX_ARRAY);
}

/*-------------------------------------------------------------
 * Draw a single quad that covers the bounding box of a path
 *-------------------------------------------------------------*/

static void
shDrawBoundBox(VGContext * restrict c, SHPath * restrict p, VGPaintMode mode)
{
   SH_ASSERT(c != NULL && p != NULL);
   SHfloat K = 1.0f;
   if (mode == VG_STROKE_PATH) {
      K = SH_CEIL(c->strokeMiterLimit * c->strokeLineWidth) + 1.0f;
   }

   shDrawQuads(p->min.x - K, p->min.y - K, p->max.x + K, p->min.y - K, p->max.x + K, p->max.y + K, p->min.x - K, p->max.y + K);

}

/*--------------------------------------------------------------
 * Constructs & draws colored OpenGL primitives that cover the
 * given bounding box to represent the currently selected
 * stroke or fill paint
 *--------------------------------------------------------------*/

static void
shDrawPaintMesh(VGContext * c, SHVector2 * min, SHVector2 * max,
                VGPaintMode mode, GLenum texUnit)
{
   SH_ASSERT(c != NULL && min != NULL && max != NULL);

   /* Pick the right paint */
   SHPaint *p = NULL;
   SHfloat K = 1.0f;
   switch (mode) {
   case VG_FILL_PATH:
      p = (c->fillPaint ? c->fillPaint : &c->defaultPaint);
      break;
   case VG_STROKE_PATH:
      p = (c->strokePaint ? c->strokePaint : &c->defaultPaint);
      K = SH_CEIL(c->strokeMiterLimit * c->strokeLineWidth) + 1.0f;
      break;
   }

   /* We want to be sure to cover every pixel of this path so better
      take a pixel more than leave some out (multisampling is tricky). */
   SHVector2 pmin, pmax;
   SET2V(pmin, (*min));
   SUB2(pmin, K, K);
   SET2V(pmax, (*max));
   ADD2(pmax, K, K);

   /* Construct appropriate OpenGL primitives so as
      to fill the stencil mask with select paint */

   switch (p->type) {
   case VG_PAINT_TYPE_LINEAR_GRADIENT:
      shDrawLinearGradientMesh(p, min, max, mode, texUnit);
      break;

   case VG_PAINT_TYPE_RADIAL_GRADIENT:
      shDrawRadialGradientMesh(p, min, max, mode, texUnit);
      break;

   case VG_PAINT_TYPE_PATTERN:
      if (p->pattern != VG_INVALID_HANDLE) {
         shDrawPatternMesh(p, min, max, mode, texUnit);
         break;
      }                         /* else behave as a color paint */

   case VG_PAINT_TYPE_COLOR:
      glColor4fv((GLfloat *) & p->color);
      shDrawQuads(pmin.x, pmin.y, pmax.x, pmin. y,pmax.x, pmax.y, pmin.x, pmax.y);
      break;
   }
}

static VGboolean
shIsTessCacheValid(VGContext * restrict c, SHPath * restrict p)
{
   SH_ASSERT(c != NULL && p != NULL);

   SHfloat nX, nY;
   SHVector2 X, Y;
   SHMatrix3x3 mi, mchange;
   VGboolean valid = VG_TRUE;

   if (p->cacheDataValid == VG_FALSE) {
      valid = VG_FALSE;
   } else if (p->cacheTransformInit == VG_FALSE) {
      valid = VG_FALSE;
   } else if (shInvertMatrix(&p->cacheTransform, &mi) == VG_FALSE) {
      valid = VG_FALSE;
   } else {
      /* TODO: Compare change matrix for any scale or shear  */
      /* mi contains the invert matrix of p->cacheTransform */
      MULMATMAT(c->pathTransform, mi, mchange);
      SET2(X, mchange.m[0][0], mchange.m[1][0]);
      SET2(Y, mchange.m[0][1], mchange.m[1][1]);
      nX = NORM2(X);
      nY = NORM2(Y);
      if (nX > 1.01f || nX < 0.99 || nY > 1.01f || nY < 0.99)
         valid = VG_FALSE;
   }

   if (valid == VG_FALSE) {
      /* Update cache */
      p->cacheDataValid = VG_TRUE;
      p->cacheTransformInit = VG_TRUE;
      p->cacheTransform = c->pathTransform;
      p->cacheStrokeTessValid = VG_FALSE;
   }

   return valid;
}

static VGboolean
shIsStrokeCacheValid(VGContext * restrict c, SHPath * restrict p)
{
   SH_ASSERT(c != NULL && p != NULL);

   VGboolean valid = VG_TRUE;
   if (p->cacheStrokeInit == VG_FALSE) {
      valid = VG_FALSE;
   } else if (p->cacheStrokeTessValid == VG_FALSE) {
      valid = VG_FALSE;
   } else if (c->strokeDashPattern.size > 0) {
      valid = VG_FALSE;
   } else if (p->cacheStrokeLineWidth != c->strokeLineWidth ||
            p->cacheStrokeCapStyle != c->strokeCapStyle ||
            p->cacheStrokeJoinStyle != c->strokeJoinStyle ||
            p->cacheStrokeMiterLimit != c->strokeMiterLimit) {
      valid = VG_FALSE;
   }

   if (valid == VG_FALSE) {
      /* Update cache */
      p->cacheStrokeInit = VG_TRUE;
      p->cacheStrokeTessValid = VG_TRUE;
      p->cacheStrokeLineWidth = c->strokeLineWidth;
      p->cacheStrokeCapStyle = c->strokeCapStyle;
      p->cacheStrokeJoinStyle = c->strokeJoinStyle;
      p->cacheStrokeMiterLimit = c->strokeMiterLimit;
   }

   return valid;
}

/*-----------------------------------------------------------
 * Tessellates / strokes the path and draws it according to
 * VGContext state.
 *-----------------------------------------------------------*/
// TODO: leggi https://stackoverflow.com/questions/31336454/draw-quadratic-curve-on-gpu
// http://www.glprogramming.com/red/chapter12.html
VG_API_CALL void
vgDrawPath(VGPath path, VGbitfield paintModes)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!shIsValidPath(context, path),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   VG_RETURN_ERR_IF(paintModes & (~(VG_STROKE_PATH | VG_FILL_PATH)),
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* Check whether scissoring is enabled and scissor
      rectangle is valid */

   if (context->scissoring == VG_TRUE) {
      SHRectangle *rect = &context->scissor.items[0];
      if (context->scissor.size == 0)
         VG_RETURN(VG_NO_RETVAL);
      if (rect->w <= 0.0f || rect->h <= 0.0f)
         VG_RETURN(VG_NO_RETVAL);
      glScissor((GLint) rect->x, (GLint) rect->y, (GLint) rect->w,
                (GLint) rect->h);
      glEnable(GL_SCISSOR_TEST);
   }

   SHPath *p = (SHPath *) path;

   /* If user-to-surface matrix invertible tessellate in
      surface space for better path resolution */

   if (shIsTessCacheValid(context, p) == VG_FALSE) {
      SHMatrix3x3 mi;
      if (shInvertMatrix(&context->pathTransform, &mi)) {
         // TODO: replace shFlattenPath with libtess2
         shFlattenPath(p, 1);
         shTransformVertices(&mi, p);
      } else {
         shFlattenPath(p, 0);
      }
      shFindBoundbox(p);
   }

   /* Change render quality according to the context */
   /* TODO: Turn antialiasing on/off */
   shSetRenderQualityGL(context->renderingQuality);

   /* Pick paint if available or default */
   SHPaint *fill = (context->fillPaint ? context->fillPaint : &context->defaultPaint);
   SHPaint *stroke =
      (context->strokePaint ? context->strokePaint : &context->defaultPaint);

   /* Apply transformation */
   SHfloat mgl[16];
   shMatrixToGL(&context->pathTransform, mgl);
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glMultMatrixf(mgl);

   // TODO: bisogna capire se serve sempre abilitare la scrittura nello stencil (sembra crei problemi a test_composition)
   if (paintModes & VG_FILL_PATH) {
      /* Tesselate into stencil */
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS, 0, 0);
      glStencilOp(GL_INVERT, GL_INVERT, GL_INVERT);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      shDrawVertices(p, GL_TRIANGLE_FAN);

      /* Setup blending */
      updateBlendingStateGL(context,
                            fill->type == VG_PAINT_TYPE_COLOR &&
                            fill->color.a == 1.0f);

      /* Draw paint where stencil odd */
      glStencilFunc(GL_EQUAL, 1, 1);
      glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      shDrawPaintMesh(context, &p->min, &p->max, VG_FILL_PATH, GL_TEXTURE0);

      /* Clear stencil for sure */
      /* TODO: Is there any way to do this safely along
         with the paint generation pass?? */
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      shDrawBoundBox(context, p, VG_FILL_PATH);

      /* Reset state */
      glDisable(GL_BLEND);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glDisable(GL_STENCIL_TEST);

   }

   /* TODO: Turn antialiasing on/off */
   shSetRenderQualityGL(context->renderingQuality);

   if ((paintModes & VG_STROKE_PATH) && context->strokeLineWidth > 0.0f) {

      if (1) {                  /*context->strokeLineWidth > 1.0f) { */
      /*
       * if (context->strokeLineWidth > 1.0f) {
       */

         if (shIsStrokeCacheValid(context, p) == VG_FALSE) {
            /* Generate stroke triangles in user space */
            shVector2ArrayClear(&p->stroke);
            shStrokePath(context, p);
         }

         /* Stroke into stencil */
         glEnable(GL_STENCIL_TEST);
         glStencilFunc(GL_NOTEQUAL, 1, 1);
         glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
         glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
         shDrawStroke(p);

         /* Setup blending */
         updateBlendingStateGL(context,
                               stroke->type == VG_PAINT_TYPE_COLOR &&
                               stroke->color.a == 1.0f);

         /* Draw paint where stencil odd */
         glStencilFunc(GL_EQUAL, 1, 1);
         glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
         glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
         shDrawPaintMesh(context, &p->min, &p->max, VG_STROKE_PATH,
                         GL_TEXTURE0);

         /* Clear stencil for sure */
         glDisable(GL_BLEND);
         glDisable(GL_MULTISAMPLE);
         glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
         shDrawBoundBox(context, p, VG_STROKE_PATH);

         /* Reset state */
         glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
         glDisable(GL_STENCIL_TEST);
         /*
          * glDisable(GL_BLEND);
          */

      }
      else {

         /* Simulate thin stroke by alpha */
         SHColor c = stroke->color;
         if (context->strokeLineWidth < 1.0f)
            c.a *= context->strokeLineWidth;

         /* Draw contour as a line */
         glDisable(GL_MULTISAMPLE);
         glBlendEquation(GL_FUNC_ADD);
         glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
         glEnable(GL_BLEND);
         glEnable(GL_LINE_SMOOTH);
         glColor4fv((GLfloat *) & c);
         shDrawVertices(p, GL_LINE_STRIP);

         glDisable(GL_BLEND);
         glDisable(GL_LINE_SMOOTH);
      }
   }

   glPopMatrix();

   if (context->scissoring == VG_TRUE)
      glDisable(GL_SCISSOR_TEST);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgDrawImage(VGImage image)
{
   SHfloat mgl[16];
   SHfloat texGenS[4] = { 0, 0, 0, 0 };
   SHfloat texGenT[4] = { 0, 0, 0, 0 };
   SHVector2 min, max;

   VG_GETCONTEXT(VG_NO_RETVAL);

   VG_RETURN_ERR_IF(!shIsValidImage(context, image),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* TODO: check if image is current render target */

   /* Check whether scissoring is enabled and scissor
      rectangle is valid */
   if (context->scissoring == VG_TRUE) {
      SHRectangle *rect = &context->scissor.items[0];
      if (context->scissor.size == 0)
         VG_RETURN(VG_NO_RETVAL);
      if (rect->w <= 0.0f || rect->h <= 0.0f)
         VG_RETURN(VG_NO_RETVAL);
      glScissor((GLint) rect->x, (GLint) rect->y, (GLint) rect->w, (GLint) rect->h);
      glEnable(GL_SCISSOR_TEST);
   }

   /* Apply image-user-to-surface transformation */
   SHImage *i = (SHImage *) image;
   shMatrixToGL(&context->imageTransform, mgl);
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glMultMatrixf(mgl);

   /* Clamp to edge for proper filtering, modulate for multiply mode */
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, i->texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   /* Adjust antialiasing to settings */
   switch (context->imageQuality) {
   case VG_IMAGE_QUALITY_NONANTIALIASED:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glDisable(GL_MULTISAMPLE);
      break;
   // TODO: How to make FASTER != BETTER ?
   case VG_IMAGE_QUALITY_FASTER:
   case VG_IMAGE_QUALITY_BETTER:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glEnable(GL_MULTISAMPLE);
      break;
   default:
      break;
   }

   /* Generate image texture coords automatically */
   texGenS[0] = 1.0f / i->texwidth;
   texGenT[1] = 1.0f / i->texheight;
   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
   glTexGenfv(GL_S, GL_OBJECT_PLANE, texGenS);
   glTexGenfv(GL_T, GL_OBJECT_PLANE, texGenT);
   glEnable(GL_TEXTURE_GEN_S);
   glEnable(GL_TEXTURE_GEN_T);

   /* Pick fill paint */
   SHPaint *fill = (context->fillPaint ? context->fillPaint : &context->defaultPaint);

   /* Use paint color when multiplying with a color-paint */
   if ((context->imageMode == VG_DRAW_IMAGE_MULTIPLY && fill->type == VG_PAINT_TYPE_COLOR)
       || context->imageMode == VG_DRAW_IMAGE_STENCIL)
      glColor4fv((GLfloat *) & fill->color);
   else
      glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

   /* Check image drawing mode */
   if (context->imageMode == VG_DRAW_IMAGE_MULTIPLY && fill->type != VG_PAINT_TYPE_COLOR) {

      /* Draw image quad into stencil */
      glDisable(GL_BLEND);
      glDisable(GL_TEXTURE_2D);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS, 1, 1);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

      shDrawQuadsInt(0, 0, i->width, 0 ,i->width, i->height , 0, i->height);

      /* Setup blending */
      updateBlendingStateGL(context, i->fd.premultiplied);

      /* Draw gradient mesh where stencil 1 */
      glEnable(GL_TEXTURE_2D);
      glStencilFunc(GL_EQUAL, 1, 1);
      glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

      SET2(min, 0, 0);
      SET2(max, (SHfloat) i->width, (SHfloat) i->height);

      switch (fill->type) {
      case VG_PAINT_TYPE_RADIAL_GRADIENT:
         shDrawRadialGradientMesh(fill, &min, &max, VG_FILL_PATH, GL_TEXTURE1);
         break;
      case VG_PAINT_TYPE_LINEAR_GRADIENT:
         shDrawLinearGradientMesh(fill, &min, &max, VG_FILL_PATH, GL_TEXTURE1);
         break;
      case VG_PAINT_TYPE_PATTERN:
         shDrawPatternMesh(fill, &min, &max, VG_FILL_PATH, GL_TEXTURE1);
         break;
      default:
         break;
      }

      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_STENCIL_TEST);

   } else if (context->imageMode == VG_DRAW_IMAGE_STENCIL) {

      /* Draw image quad into stencil */
      glDisable(GL_BLEND);
      glDisable(GL_TEXTURE_2D);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glClear(GL_STENCIL_BUFFER_BIT);
      glDisable(GL_DEPTH_TEST);
      glEnable(GL_STENCIL_TEST);

      glStencilFunc(GL_ALWAYS, 1, ~0U);
      glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

      shDrawQuadsInt(0, 0, i->width, 0 ,i->width, i->height , 0, i->height);

      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glStencilFunc(GL_EQUAL, 1, ~0U);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

      /* Setup blending */
      updateBlendingStateGL(context, i->fd.premultiplied);
      glEnable(GL_TEXTURE_2D);

      shDrawQuadsInt(0, 0, i->width, 0 ,i->width, i->height , 0, i->height);

      glDisable(GL_TEXTURE_2D);

   } else {
      /* Either normal mode or multiplying with a color-paint */

      /* Setup blending */
      updateBlendingStateGL(context, i->fd.premultiplied);

      /* Draw textured quad */
      glEnable(GL_TEXTURE_2D);

      shDrawQuadsInt(0, 0, i->width, 0 ,i->width, i->height , 0, i->height);

      glDisable(GL_TEXTURE_2D);
   }

   glDisable(GL_TEXTURE_GEN_S);
   glDisable(GL_TEXTURE_GEN_T);
   glPopMatrix();

   if (context->scissoring == VG_TRUE)
      glDisable(GL_SCISSOR_TEST);
   if (context->imageMode == VG_DRAW_IMAGE_STENCIL) {
      glDisable(GL_STENCIL_TEST);
   }

   glDisable(GL_BLEND);
   VG_RETURN(VG_NO_RETVAL);
}
