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

#include "shVectors.h"
#include <stdbool.h>

#define _ITEM_T SHVector2
#define _ARRAY_T SHVector2Array
#define _FUNC_T shVector2Array
#define _COMPARE_T(v1,v2) EQ2V(v1,v2)
#define _ARRAY_DEFINE
#include "shArrayBase.h"

void
SHVector2_ctor(SHVector2 * v)
{
   SH_ASSERT(v != NULL);
   v->x = 0.0f;
   v->y = 0.0f;
}

void
SHVector2_dtor(SHVector2 * v)
{
}

void
SHVector3_ctor(SHVector3 * v)
{
   SH_ASSERT(v != NULL);
   v->x = 0.0f;
   v->y = 0.0f;
   v->z = 0.0f;
}

void
SHVector3_dtor(SHVector3 * v)
{
}

void
SHVector4_ctor(SHVector4 * v)
{
   SH_ASSERT(v != NULL);
   v->x = 0.0f;
   v->y = 0.0f;
   v->z = 0.0f;
   v->w = 0.0f;
}

void
SHVector4_dtor(SHVector4 * v)
{
}

void
SHRectangle_ctor(SHRectangle * r)
{
   SH_ASSERT(r != NULL);
   r->x = 0.0f;
   r->y = 0.0f;
   r->w = 0.0f;
   r->h = 0.0f;
}

void
SHRectangle_dtor(SHRectangle * r)
{
}

void
shRectangleSet(SHRectangle * r, SHfloat x, SHfloat y, SHfloat w, SHfloat h)
{
   SH_ASSERT(r != NULL);
   r->x = x;
   r->y = y;
   r->w = w;
   r->h = h;
}

void
SHMatrix3x3_ctor(SHMatrix3x3 * mt)
{
   SH_ASSERT(mt != NULL);
   IDMAT((*mt));
}

void
SHMatrix3x3_dtor(SHMatrix3x3 * mt)
{
}

inline void
shMatrixToGL(SHMatrix3x3 * restrict m, SHfloat mgl[16])
{
   SH_ASSERT(m != NULL);
   /* When 2D vectors are specified OpenGL defaults Z to 0.0f so we
      have to shift the third column of our 3x3 matrix to right */
   mgl[0] = m->m[0][0];
   mgl[1] = m->m[1][0];
   mgl[2] = m->m[2][0];
   mgl[3] = 0.0f;
   mgl[4] = m->m[0][1];
   mgl[5] = m->m[1][1];
   mgl[6] = m->m[2][1];
   mgl[7] = 0.0f;
   mgl[8] = 0.0f;
   mgl[9] = 0.0f;
   mgl[10] = 1.0f;
   mgl[11] = 0.0f;
   mgl[12] = m->m[0][2];
   mgl[13] = m->m[1][2];
   mgl[14] = m->m[2][1];
   mgl[15] = 1.0f;
}

SHint
shInvertMatrix(SHMatrix3x3 * restrict m, SHMatrix3x3 * restrict mout)
{
   SH_ASSERT(m != NULL && mout != NULL);
   SHfloat m00 = m->m[0][0];
   SHfloat m01 = m->m[0][1];
   SHfloat m02 = m->m[0][2];
   SHfloat m10 = m->m[1][0];
   SHfloat m11 = m->m[1][1];
   SHfloat m12 = m->m[1][2];
   SHfloat m20 = m->m[2][0];
   SHfloat m21 = m->m[2][1];
   SHfloat m22 = m->m[2][2];

   /* Calculate determinant */
   SHfloat D0 = m11 * m22 - m21 * m12;
   SHfloat D1 = m20 * m12 - m10 * m22;
   SHfloat D2 = m10 * m21 - m20 * m11;
   SHfloat D = m00 * D0 + m01 * D1 + m02 * D2;

   /* Check if singular */
   if (D == 0.0f)
      return 0;
   D = 1.0f / D;

   /* Calculate inverse */
   mout->m[0][0] = D * D0;
   mout->m[0][1] = D * (m21 * m02 - m01 * m22);
   mout->m[0][2] = D * (m01 * m12 - m11 * m02);
   mout->m[1][0] = D * D1;
   mout->m[1][1] = D * (m00 * m22 - m20 * m02);
   mout->m[1][2] = D * (m10 * m02 - m00 * m12);
   mout->m[2][0] = D * D2;
   mout->m[2][1] = D * (m20 * m01 - m00 * m21);
   mout->m[2][2] = D * (m00 * m11 - m10 * m01);

   return 1;
}

inline SHfloat
shVectorOrientation(SHVector2 * restrict v)
{
   SH_ASSERT(v != NULL);
   SHfloat norm = (SHfloat) NORM2((*v));
   SHfloat cosa = v->x / norm;
   SHfloat sina = v->y / norm;
   return (SHfloat) (sina >= 0.0f ? SH_ACOS(cosa) : 2.0f * PI - SH_ACOS(cosa));
}

inline SHint
shLineLineXsection(SHVector2 * restrict o1, SHVector2 * restrict v1,
                   SHVector2 * restrict o2, SHVector2 * restrict v2, SHVector2 * restrict xsection)
{
   SH_ASSERT(o1 != NULL && o2 != NULL && v1 != NULL && v2 != NULL);
   SHfloat rightU = o2->x - o1->x;
   SHfloat rightD = o2->y - o1->y;

   SHfloat D = v1->x * (-v2->y) - v1->y * (-v2->x);
   SHfloat DX = rightU * (-v2->y) - rightD * (-v2->x);
/*SHfloat DY = v1.x   * rightD  - v1.y   * rightU;*/

   SHfloat t1 = DX / D;

   if (D == 0.0f)
      return 0;

   xsection->x = o1->x + t1 * v1->x;
   xsection->y = o1->y + t1 * v1->y;
   return 1;
}
