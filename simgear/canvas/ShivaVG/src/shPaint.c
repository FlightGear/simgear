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
#include "shContext.h"
#include "shPaint.h"
#include <stdio.h>
#include "shCommons.h"

#define _ITEM_T SHStop
#define _ARRAY_T SHStopArray
#define _FUNC_T shStopArray
#define _COMPARE_T(s1,s2) 0
#define _ARRAY_DEFINE
#include "shArrayBase.h"

#define _ITEM_T SHPaint*
#define _ARRAY_T SHPaintArray
#define _FUNC_T shPaintArray
#define _ARRAY_DEFINE
#include "shArrayBase.h"


void
SHPaint_ctor(SHPaint * p)
{
   SH_ASSERT(p != NULL);

   p->type = VG_PAINT_TYPE_COLOR;
   CSET(p->color, 0, 0, 0, 1);

   SH_INITOBJ(SHStopArray, p->instops);
   SH_INITOBJ(SHStopArray, p->stops);

   p->premultiplied = VG_FALSE;
   p->spreadMode = VG_COLOR_RAMP_SPREAD_PAD;
   p->tilingMode = VG_TILE_FILL;

   for (int i = 0; i < 4; ++i)
      p->linearGradient[i] = 0.0f;
   for (int i = 0; i < 5; ++i)
      p->radialGradient[i] = 0.0f;

   p->pattern = VG_INVALID_HANDLE;

   glGenTextures(1, &p->texture);
   glBindTexture(GL_TEXTURE_1D, p->texture);
   glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, SH_GRADIENT_TEX_SIZE, 0,
                GL_RGBA, GL_FLOAT, NULL);
}

void
SHPaint_dtor(SHPaint * p)
{
   SH_ASSERT(p != NULL);

   SH_DEINITOBJ(SHStopArray, p->instops);
   SH_DEINITOBJ(SHStopArray, p->stops);

   if (glIsTexture(p->texture))
      glDeleteTextures(1, &p->texture);
}

VG_API_CALL VGPaint
vgCreatePaint(void)
{
   SHPaint *p = NULL;
   VG_GETCONTEXT(VG_INVALID_HANDLE);

   /* Create new paint object */
   SH_NEWOBJ(SHPaint, p);
   VG_RETURN_ERR_IF(!p, VG_OUT_OF_MEMORY_ERROR, VG_INVALID_HANDLE);

   /* Add to resource list */
   shPaintArrayPushBack(&context->paints, p);

   VG_RETURN((VGPaint) p);
}

VG_API_CALL void
vgDestroyPaint(VGPaint paint)
{
   SHint index;
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Check if handle valid */
   SH_ASSERT(paint != NULL);

   index = shPaintArrayFind(&context->paints, (SHPaint *) paint);
   VG_RETURN_ERR_IF(index == -1, VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* Delete object and remove resource */
   SH_DELETEOBJ(SHPaint, (SHPaint *) paint);
   shPaintArrayRemoveAt(&context->paints, index);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgSetPaint(VGPaint paint, VGbitfield paintModes)
{

   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Check if handle valid */
   VG_RETURN_ERR_IF(!shIsValidPaint(context, paint) &&
                    paint != VG_INVALID_HANDLE,
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* Check for invalid mode */
   VG_RETURN_ERR_IF(paintModes & ~(VG_STROKE_PATH | VG_FILL_PATH),
                    VG_ILLEGAL_ARGUMENT_ERROR, VG_NO_RETVAL);

   /* Set stroke / fill */
   if (paintModes & VG_STROKE_PATH)
      context->strokePaint = (SHPaint *) paint;
   if (paintModes & VG_FILL_PATH)
      context->fillPaint = (SHPaint *) paint;

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGPaint
vgGetPaint(VGPaintMode paintMode)
{
   VG_GETCONTEXT(VG_INVALID_HANDLE);

   if (paintMode & VG_STROKE_PATH)
      VG_RETURN((VGPaint) context->strokePaint);
   if (paintMode & VG_FILL_PATH)
      VG_RETURN((VGPaint) context->fillPaint);

   return (VGPaint) VG_ILLEGAL_ARGUMENT_ERROR;
}

VG_API_CALL void
vgPaintPattern(VGPaint paint, VGImage pattern)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Check if handle valid */
   VG_RETURN_ERR_IF(!shIsValidPaint(context, paint),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* Check if pattern image valid */
   VG_RETURN_ERR_IF(!shIsValidImage(context, pattern),
                    VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   /* TODO: Check if pattern image is current rendering target */

   /* Set pattern image */
   ((SHPaint *) paint)->pattern = pattern;

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL void
vgSetColor(VGPaint paint, VGuint rgba)
{
   VG_GETCONTEXT(VG_NO_RETVAL);

   /* Check if handle valid */
   VG_RETURN_ERR_IF(!shIsValidPaint(context, paint), VG_BAD_HANDLE_ERROR, VG_NO_RETVAL);

   // unpack rgba
   VGfloat color[4] = {
      ((rgba >> 24) & 0xff) / 255.0f,
      ((rgba >> 16) & 0xff) / 255.0f,
      ((rgba >> 8) & 0xff) / 255.0f,
      (rgba & 0xff) / 255.0f
   };

   vgSetParameterfv(paint, VG_PAINT_COLOR, 4, color);

   VG_RETURN(VG_NO_RETVAL);
}

VG_API_CALL VGuint
vgGetColor(VGPaint paint)
{
   VG_GETCONTEXT(0);

   /* Check if handle valid */
   VG_RETURN_ERR_IF(!shIsValidPaint(context, paint), VG_BAD_HANDLE_ERROR, 0);

   VGfloat color[4];

   vgGetParameterfv(paint, VG_PAINT_COLOR, 4, color);

   VGint red = (VGint)(SH_CLAMPF(color[0]) * 255.0f + 0.5f);
   VGint green = (VGint)(SH_CLAMPF(color[1]) * 255.0f + 0.5f);
   VGint blue = (VGint)(SH_CLAMPF(color[2]) * 255.0f + 0.5f);
   VGint alpha = (VGint)(SH_CLAMPF(color[3]) * 255.0f + 0.5f);

   // return packed rgba
   return (red << 24) | (green << 16) | (blue << 8) | alpha;
}

void
shUpdateColorRampTexture(SHPaint * p)
{
   SHint s = 0;
   SHStop *stop1, *stop2;
   SHfloat rgba[SH_GRADIENT_TEX_COORDSIZE];
   SHint x1 = 0, x2 = 0, dx, x;
   SHColor dc, c;
   SHfloat k;

   SH_ASSERT(p != NULL);

   /* Write first pixel color */
   stop1 = &p->stops.items[0];
   CSTORE_RGBA1D_F(stop1->color, rgba, x1);

   /* Walk stops */
   for (s = 1; s < p->stops.size; ++s, x1 = x2, stop1 = stop2) {

      /* Pick next stop */
      stop2 = &p->stops.items[s];
      x2 = (SHint) (stop2->offset * (SH_GRADIENT_TEX_SIZE - 1));

      SH_ASSERT(x1 >= 0 && x1 < SH_GRADIENT_TEX_SIZE &&
                x2 >= 0 && x2 < SH_GRADIENT_TEX_SIZE && x1 <= x2);

      dx = x2 - x1;
      CSUBCTO(stop2->color, stop1->color, dc);

      /* Interpolate inbetween */
      for (x = x1 + 1; x <= x2; ++x) {

         k = (SHfloat) (x - x1) / dx;
         CSETC(c, stop1->color);
         CADDCK(c, dc, k);
         CSTORE_RGBA1D_F(c, rgba, x);
      }
   }

   /* Update texture image */
   glBindTexture(GL_TEXTURE_1D, p->texture);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexSubImage1D(GL_TEXTURE_1D, 0, 0, SH_GRADIENT_TEX_SIZE,
                   GL_RGBA, GL_FLOAT, rgba);
}

void
shValidateInputStops(SHPaint * p)
{
   SHStop *instop, stop;
   SHfloat lastOffset = 0.0f;

   SH_ASSERT(p != NULL);
   shStopArrayClear(&p->stops);
   shStopArrayReserve(&p->stops, p->instops.size);

   /* Assure input stops are properly defined */
   for (SHint i = 0; i < p->instops.size; ++i) {

      /* Copy stop color */
      instop = &p->instops.items[i];
      stop.color = instop->color;

      /* Offset must be in [0,1] */
      if (instop->offset < 0.0f || instop->offset > 1.0f)
         continue;

      /* Discard whole sequence if not in ascending order */
      if (instop->offset < lastOffset) {
         shStopArrayClear(&p->stops);
         break;
      }

      /* Add stop at offset 0 with same color if first not at 0 */
      if (p->stops.size == 0 && instop->offset != 0.0f) {
         stop.offset = 0.0f;
         shStopArrayPushBackP(&p->stops, &stop);
      }

      /* Add current stop to array */
      stop.offset = instop->offset;
      shStopArrayPushBackP(&p->stops, &stop);

      /* Save last offset */
      lastOffset = instop->offset;
   }

   /* Add stop at offset 1 with same color if last not at 1 */
   if (p->stops.size > 0 && lastOffset != 1.0f) {
      stop.offset = 1.0f;
      shStopArrayPushBackP(&p->stops, &stop);
   }

   /* Add 2 default stops if no valid found */
   if (p->stops.size == 0) {
      /* First opaque black */
      stop.offset = 0.0f;
      CSET(stop.color, 0, 0, 0, 1);
      shStopArrayPushBackP(&p->stops, &stop);
      /* Last opaque white */
      stop.offset = 1.0f;
      CSET(stop.color, 1, 1, 1, 1);
      shStopArrayPushBackP(&p->stops, &stop);
   }

   /* Update texture */
   shUpdateColorRampTexture(p);
}

void
shGenerateStops(SHPaint * p, SHfloat minOffset, SHfloat maxOffset,
                SHStopArray * outStops)
{
   SHStop *s1, *s2;
   SHint i1, i2;
   SHfloat o = 0.0f;
   SHfloat ostep = 0.0f;
   SHint istep = 1;
   SHint istart = 0;

   SH_ASSERT(p != NULL);

   SHint iend = p->stops.size - 1;
   SHint minDone = 0;
   SHint maxDone = 0;
   SHStop outStop;

   /* Start below zero? */
   if (minOffset < 0.0f) {
      if (p->spreadMode == VG_COLOR_RAMP_SPREAD_PAD) {
         /* Add min offset stop */
         outStop = p->stops.items[0];
         outStop.offset = minOffset;
         shStopArrayPushBackP(outStops, &outStop);
         /* Add max offset stop and exit */
         if (maxOffset < 0.0f) {
            outStop.offset = maxOffset;
            shStopArrayPushBackP(outStops, &outStop);
            return;
         }
      } else {
         /* Pad starting offset to nearest factor of 2 */
         SHint ioff = (SHint) SH_FLOOR(minOffset);
         o = (SHfloat) (ioff - (ioff & 1));
      }
   }

   /* Construct stops until max offset reached */
   for (i1 = istart, i2 = istart + istep; maxDone != 1;
        i1 += istep, i2 += istep, o += ostep) {

      /* All stops consumed? */
      if (i1 == iend) {
         switch (p->spreadMode) {

         case VG_COLOR_RAMP_SPREAD_PAD:
            /* Pick last stop */
            outStop = p->stops.items[i1];
            if (!minDone) {
               /* Add min offset stop with last color */
               outStop.offset = minOffset;
               shStopArrayPushBackP(outStops, &outStop);
            }
            /* Add max offset stop with last color */
            outStop.offset = maxOffset;
            shStopArrayPushBackP(outStops, &outStop);
            return;

         case VG_COLOR_RAMP_SPREAD_REPEAT:
            /* Reset iteration */
            i1 = istart;
            i2 = istart + istep;
            /* Add stop1 if past min offset */
            if (minDone) {
               outStop = p->stops.items[0];
               outStop.offset = o;
               shStopArrayPushBackP(outStops, &outStop);
            }
            break;

         case VG_COLOR_RAMP_SPREAD_REFLECT:
            /* Reflect iteration direction */
            istep = -istep;
            i2 = i1 + istep;
            iend = (istep == 1) ? p->stops.size - 1 : 0;
            break;
         }
      }

      /* 2 stops and their offset distance */
      s1 = &p->stops.items[i1];
      s2 = &p->stops.items[i2];
      ostep = s2->offset - s1->offset;
      ostep = SH_ABS(ostep);

      /* Add stop1 if reached min offset */
      if (!minDone && o + ostep > minOffset) {
         minDone = 1;
         outStop = *s1;
         outStop.offset = o;
         shStopArrayPushBackP(outStops, &outStop);
      }

      /* Mark done if reached max offset */
      if (o + ostep > maxOffset)
         maxDone = 1;

      /* Add stop2 if past min offset */
      if (minDone) {
         outStop = *s2;
         outStop.offset = o + ostep;
         shStopArrayPushBackP(outStops, &outStop);
      }
   }
}

static void
shSetGradientTexGLState(SHPaint * restrict p)
{
   SH_ASSERT(p != NULL);

   glBindTexture(GL_TEXTURE_1D, p->texture);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   switch (p->spreadMode) {
   case VG_COLOR_RAMP_SPREAD_PAD:
      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      break;
   case VG_COLOR_RAMP_SPREAD_REPEAT:
      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      break;
   case VG_COLOR_RAMP_SPREAD_REFLECT:
      glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
      break;
   }

   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

static void
shSetPatternTexGLState(SHPaint * restrict p, VGContext * restrict c)
{
   SH_ASSERT(p != NULL && c != NULL);

   glBindTexture(GL_TEXTURE_2D, ((SHImage *) p->pattern)->texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   switch (p->tilingMode) {
   case VG_TILE_FILL:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR,
                       (GLfloat *) & c->tileFillColor);
      break;
   case VG_TILE_PAD:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      break;
   case VG_TILE_REPEAT:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      break;
   case VG_TILE_REFLECT:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
      break;
   }

   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glColor4f(1, 1, 1, 1);
}

int
shDrawLinearGradientMesh(SHPaint * p, SHVector2 * min, SHVector2 * max,
                         VGPaintMode mode, GLenum texUnit)
{
   SH_ASSERT(p != NULL && min != NULL && max != NULL);

   SHfloat x1 = p->linearGradient[0];
   SHfloat y1 = p->linearGradient[1];
   SHfloat x2 = p->linearGradient[2];
   SHfloat y2 = p->linearGradient[3];
   SHVector2 c, ux, uy;
   SHVector2 cc, uux, uuy;

   SHMatrix3x3 *m;
   SHMatrix3x3 mi;
   SHint invertible;
   SHVector2 corners[4];
   SHVector2 l1, r1, l2, r2;

   /* Pick paint transform matrix */
   SH_GETCONTEXT(0);
   if (mode == VG_FILL_PATH)
      m = &context->fillTransform;
   else                         // (mode == VG_STROKE_PATH)
      m = &context->strokeTransform;

   /* Gradient center and unit vectors */
   SET2(c, x1, y1);
   SET2(ux, x2 - x1, y2 - y1);
   SET2(uy, -ux.y, ux.x);

   SHfloat n = NORM2(ux);
   DIV2(ux, n);
   NORMALIZE2(uy);

   /* Apply paint-to-user transformation */
   ADD2V(ux, c);
   ADD2V(uy, c);
   TRANSFORM2TO(c, (*m), cc);
   TRANSFORM2TO(ux, (*m), uux);
   TRANSFORM2TO(uy, (*m), uuy);
   SUB2V(ux, c);
   SUB2V(uy, c);
   SUB2V(uux, cc);
   SUB2V(uuy, cc);

   /* Boundbox corners */
   SET2(corners[0], min->x, min->y);
   SET2(corners[1], max->x, min->y);
   SET2(corners[2], max->x, max->y);
   SET2(corners[3], min->x, max->y);

   /* Find inverse transformation (back to paint space) */
   invertible = shInvertMatrix(m, &mi);
   if (!invertible || n == 0.0f) {

      /* Fill boundbox with color at offset 1 */
      SHColor *c = &p->stops.items[p->stops.size - 1].color;
      glColor4fv((GLfloat *) c);
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(2, GL_FLOAT, 0, (GLfloat *) & corners);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDisableClientState(GL_VERTEX_ARRAY);

      return 1;
   }

   /*--------------------------------------------------------*/
   SHfloat minOffset = FLT_MAX;
   SHfloat maxOffset = FLT_MIN;
   SHfloat left = FLT_MAX;
   SHfloat right = FLT_MIN;

   for (SHint i = 0; i < 4; ++i) {

      /* Find min/max offset and perpendicular span */
      SHfloat o, s;
      TRANSFORM2(corners[i], mi);
      SUB2V(corners[i], c);
      o = DOT2(corners[i], ux) / n;
      s = DOT2(corners[i], uy);

      minOffset = (o < minOffset ? o : minOffset);
      maxOffset = (o > maxOffset ? o : maxOffset);
      left = (s < left ? s : left);
      right = (s > right ? s : right);
   }

   /*---------------------------------------------------------*/

   /* Corners of boundbox in gradient system */
   SET2V(l1, cc);
   SET2V(r1, cc);
   SET2V(l2, cc);
   SET2V(r2, cc);
   OFFSET2V(l1, uuy, left);
   OFFSET2V(l1, uux, minOffset * n);
   OFFSET2V(r1, uuy, right);
   OFFSET2V(r1, uux, minOffset * n);
   OFFSET2V(l2, uuy, left);
   OFFSET2V(l2, uux, maxOffset * n);
   OFFSET2V(r2, uuy, right);
   OFFSET2V(r2, uux, maxOffset * n);

   /* Draw quad using color-ramp texture */
   glActiveTexture(texUnit);
   shSetGradientTexGLState(p);

   glEnable(GL_TEXTURE_1D);

   SHVector2 cnrs[] = {r1,l1,r2,r2,l2,l1};
   SHfloat txt[] = {minOffset,minOffset,maxOffset,maxOffset,maxOffset,minOffset};

   glClientActiveTexture(texUnit);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(1, GL_FLOAT, 0, txt);
      glVertexPointer(2, GL_FLOAT, 0, cnrs);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisable(GL_TEXTURE_1D);

   return 1;
}

int
shDrawRadialGradientMesh(SHPaint * p, SHVector2 * min, SHVector2 * max,
                         VGPaintMode mode, GLenum texUnit)
{
   SH_ASSERT(p != NULL && min != NULL && max != NULL);

   // gradient center
   SHfloat cx = p->radialGradient[0];
   SHfloat cy = p->radialGradient[1];

   // gradient focal point
   SHfloat fx = p->radialGradient[2];
   SHfloat fy = p->radialGradient[3];

   // gradient radius
   SHfloat r = p->radialGradient[4];

   SHfloat fcx, fcy, rr, C;

   SHVector2 ux;
   SHVector2 uy;
   SHVector2 c, f;
   SHVector2 cf;

   SHMatrix3x3 *m;
   SHMatrix3x3 mi;
   SHint invertible;
   SHVector2 corners[4];
   SHVector2 fcorners[4];
   SHfloat minOffset = 0.0f;
   SHfloat maxOffset = 0.0f;

   SHint maxI = 0, maxJ = 0;
   SHfloat maxA = 0.0f;
   SHfloat startA = 0.0f;


   /* Pick paint transform matrix */
   SH_GETCONTEXT(0);
   if (mode == VG_FILL_PATH)
      m = &context->fillTransform;
   else                         // (mode == VG_STROKE_PATH)
      m = &context->strokeTransform;

   /* Move focus into circle if outside */
   SET2(cf, fx, fy);
   SUB2(cf, cx, cy);
   SHfloat n = NORM2(cf);
   if (n > r) {
      DIV2(cf, n);
      fx = cx + 0.995f * r * cf.x;
      fy = cy + 0.995f * r * cf.y;
   }

   /* Precalculations */
   rr = r * r;
   fcx = fx - cx;
   fcy = fy - cy;
   C = fcx * fcx + fcy * fcy - rr;

   /* Apply paint-to-user transformation
      to focus and unit vectors */
   SET2(f, fx, fy);
   SET2(c, cx, cy);
   SET2(ux, 1, 0);
   SET2(uy, 0, 1);
   ADD2(ux, cx, cy);
   ADD2(uy, cx, cy);
   TRANSFORM2(f, (*m));
   TRANSFORM2(c, (*m));
   TRANSFORM2(ux, (*m));
   TRANSFORM2(uy, (*m));
   SUB2V(ux, c);
   SUB2V(uy, c);

   /* Boundbox corners */
   SET2(corners[0], min->x, min->y);
   SET2(corners[1], max->x, min->y);
   SET2(corners[2], max->x, max->y);
   SET2(corners[3], min->x, max->y);

   /* Find inverse transformation (back to paint space) */
   invertible = shInvertMatrix(m, &mi);
   if (!invertible || r <= 0.0f) {

      /* Fill boundbox with color at offset 1 */
      SHColor *c = &p->stops.items[p->stops.size - 1].color;
      glColor4fv((GLfloat *) c);
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(2, GL_FLOAT, 0, (GLfloat *) & corners);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
      glDisableClientState(GL_VERTEX_ARRAY);

      return 1;
   }

   /*--------------------------------------------------------*/

   /* Find min/max offset */
   for (SHint i = 0; i < 4; ++i) {

      /* Transform to paint space */
      SHfloat ax, ay, A, B, D, t, off;
      TRANSFORM2TO(corners[i], mi, fcorners[i]);
      SUB2(fcorners[i], fx, fy);
      n = NORM2(fcorners[i]);
      if (n == 0.0f) {

         /* Avoid zero-length vectors */
         off = 0.0f;

      } else {

         /* Distance from focus to circle at corner angle */
         DIV2(fcorners[i], n);
         ax = fcorners[i].x;
         ay = fcorners[i].y;
         A = ax * ax + ay * ay;
         B = 2 * (fcx * ax + fcy * ay);
         D = B * B - 4 * A * C;
         t = (-B + SH_SQRT(D)) / (2 * A);

         /* Relative offset of boundbox corner */
         if (D <= 0.0f)
            off = 1.0f;
         else
            off = n / t;
      }

      /* Find smallest and largest offset */
      if (off < minOffset || i == 0)
         minOffset = off;
      if (off > maxOffset || i == 0)
         maxOffset = off;
   }

   /* Is transformed focus inside original boundbox? */
   if (f.x >= min->x && f.x <= max->x && f.y >= min->y && f.y <= max->y) {

      /* Draw whole circle */
      startA = 0.0f;
      maxA = 2 * PI;

   } else {

      /* Find most distant corner pair */
      SHfloat a;
      for (SHint i = 0; i < 3; ++i) {
         if (ISZERO2(fcorners[i]))
            continue;
         for (SHint j = i + 1; j < 4; ++j) {
            if (ISZERO2(fcorners[j]))
               continue;
            a = ANGLE2N(fcorners[i], fcorners[j]);
            if (a > maxA || maxA == 0.0f) {
               maxA = a;
               maxI = i;
               maxJ = j;
            }
         }
      }

      /* Pick starting angle */
      if (CROSS2(fcorners[maxI], fcorners[maxJ]) > 0.0f)
         startA = shVectorOrientation(&fcorners[maxI]);
      else
         startA = shVectorOrientation(&fcorners[maxJ]);
   }

   /*---------------------------------------------------------*/

   /* TODO: for minOffset we'd actually need to find minimum
      of the gradient function when X and Y are substitued
      with a line equation for each bound-box edge. As a
      workaround we use 0.0f for now. */
   minOffset = 0.0f;
   SHfloat step = PI / 50.0f;
   SHint numsteps = (SHint) SH_CEIL(maxA / step) + 1;

   glActiveTexture(texUnit);
   shSetGradientTexGLState(p);

   /* Walk the steps and draw gradient mesh */
   SHVector2 tmin, tmax;
   SHVector2 min1, max1, min2, max2;
   SHint i;
   SHfloat a;

   SHVector2 *cnrs = (SHVector2 *) malloc((numsteps - 1) * 6 * sizeof(SHVector2));
   SH_ASSERT(cnrs != NULL);
   SHfloat *txt = (SHfloat *) malloc((numsteps -1) * 6 * sizeof(SHfloat));
   SH_ASSERT(txt != NULL);

   SHint next = 0;
   for (i = 0, a = startA; i < numsteps; ++i, a += step) {

      /* Distance from focus to circle border
         at current angle (gradient space) */
      SHfloat ax = SH_COS(a);
      SHfloat ay = SH_SIN(a);
      SHfloat A = ax * ax + ay * ay;
      SHfloat B = 2 * (fcx * ax + fcy * ay);
      SHfloat D = B * B - 4 * A * C;
      SHfloat t = (-B + SH_SQRT(D)) / (2 * A);
      if (D <= 0.0f)
         t = 0.0f;

      /* Vectors pointing towards minimum and maximum
         offset at current angle (gradient space) */
      tmin.x = ax * t * minOffset;
      tmin.y = ay * t * minOffset;
      tmax.x = ax * t * maxOffset;
      tmax.y = ay * t * maxOffset;

      /* Transform back to user space */
      min2.x = f.x + tmin.x * ux.x + tmin.y * uy.x;
      min2.y = f.y + tmin.x * ux.y + tmin.y * uy.y;
      max2.x = f.x + tmax.x * ux.x + tmax.y * uy.x;
      max2.y = f.y + tmax.x * ux.y + tmax.y * uy.y;

      if (i > 0) {
         cnrs[next] = min1;
         cnrs[next+1] = min2;
         cnrs[next+2] = max2;
         cnrs[next+3] = max2;
         cnrs[next+4] = max1;
         cnrs[next+5] = min1;

         txt[next] = minOffset;
         txt[next+1] = minOffset;
         txt[next+2] = maxOffset;
         txt[next+3] = maxOffset;
         txt[next+4] = maxOffset;
         txt[next+5] = minOffset;

         next += 6;
      }

      /* Save prev points */
      min1 = min2;
      max1 = max2;
   }

   glEnable(GL_TEXTURE_1D);
   glClientActiveTexture(texUnit);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(1, GL_FLOAT, 0, txt);
      glVertexPointer(2, GL_FLOAT, 0, cnrs);
      glDrawArrays(GL_TRIANGLE_FAN, 0, (numsteps - 1) * 6);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   glDisable(GL_TEXTURE_1D);
   free(cnrs);
   free(txt);

   return 1;
}

int
shDrawPatternMesh(SHPaint * p, SHVector2 * min, SHVector2 * max,
                  VGPaintMode mode, GLenum texUnit)
{
   SHMatrix3x3 *m;
   SHMatrix3x3 mi;
   SHfloat migl[16];
   SHint invertible;
   SHVector2 corners[4];
   VGfloat sx, sy;
   SHImage *img;

   SH_ASSERT(p != NULL && min != NULL && max != NULL);

   /* Pick paint transform matrix */
   SH_GETCONTEXT(0);
   if (mode == VG_FILL_PATH)
      m = &context->fillTransform;
   else                         // (mode == VG_STROKE_PATH)
      m = &context->strokeTransform;

   /* Boundbox corners */
   SET2(corners[0], min->x, min->y);
   SET2(corners[1], max->x, min->y);
   SET2(corners[2], max->x, max->y);
   SET2(corners[3], min->x, max->y);

   /* Find inverse transformation (back to paint space) */
   invertible = shInvertMatrix(m, &mi);
   if (!invertible) {

      /* Fill boundbox with tile fill color */
      SHColor *c = &context->tileFillColor;
      glColor4fv((GLfloat *) c);
      shDrawQuadsArray((GLfloat *) corners);

      return 1;
   }


   /* Setup texture coordinate transform */
   img = (SHImage *) p->pattern;
   sx = 1.0f / (VGfloat) img->texwidth;
   sy = 1.0f / (VGfloat) img->texheight;

   glActiveTexture(texUnit);
   shMatrixToGL(&mi, migl);
   glMatrixMode(GL_TEXTURE);
   glPushMatrix();
   glScalef(sx, sy, 1.0f);
   glMultMatrixf(migl);

   /* Draw boundbox with same texture coordinates
      that will get transformed back to paint space */
   shSetPatternTexGLState(p, context);
   glEnable(GL_TEXTURE_2D);

   /*
    * glBegin(GL_TRIANGLES);
    *    glMultiTexCoord2f(texUnit, corners[0].x, corners[0].y);
    *    glVertex2fv((GLfloat *) & corners[0]);
    *    glMultiTexCoord2f(texUnit, corners[1].x, corners[1].y);
    *    glVertex2fv((GLfloat *) & corners[1]);
    *    glMultiTexCoord2f(texUnit, corners[2].x, corners[2].y);
    *    glVertex2fv((GLfloat *) & corners[2]);
    * 
    *    glMultiTexCoord2f(texUnit, corners[2].x, corners[2].y);
    *    glVertex2fv((GLfloat *) & corners[2]);
    *    glMultiTexCoord2f(texUnit, corners[3].x, corners[3].y);
    *    glVertex2fv((GLfloat *) & corners[3]);
    *    glMultiTexCoord2f(texUnit, corners[0].x, corners[0].y);
    *    glVertex2fv((GLfloat *) & corners[0]);
    * glEnd();
    */

   // TODO: è necessario studiare bene come funziano questa cosa del client state quando ci sono di mezzo le texture
   /*
    * SHVector2 c[6] = {corners[0], corners[1], corners[2], corners[2], corners[3], corners[0]};
    */

   glClientActiveTexture(texUnit);
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      /*
       * glTexCoordPointer(2, GL_FLOAT, 0, c);
       * glVertexPointer(2, GL_FLOAT, 0, c);
       * glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
       */
      glTexCoordPointer(2, GL_FLOAT, 0, corners);
      glVertexPointer(2, GL_FLOAT, 0, corners);
      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
   glDisableClientState(GL_VERTEX_ARRAY);
   glDisableClientState(GL_TEXTURE_COORD_ARRAY);


   glDisable(GL_TEXTURE_2D);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   return 1;
}
