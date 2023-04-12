/*
 * Copyright (c) 2018 Vincenzo Pupillo
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

#include "shMath.h"

/*--------------------------------------------------
 * Saturated sum of two numbers.
 *--------------------------------------------------*/
inline VGint shAddSaturate(VGint a, VGint b)
{
   SH_ASSERT(b >= 0);
   return (a + b >= a) ? a + b : SH_MAX_INT;
}

/*--------------------------------------------------
 * Ceiling to the next power of 2
 *--------------------------------------------------*/
inline VGint shClp2(VGint v)
{
   // from Hacker's Delight
   SH_ASSERT(v >= 0);
   --v;
   v |= v >> 1;
   v |= v >> 2;
   v |= v >> 4;
   v |= v >> 8;
   v |= v >> 16;
   return ++v;

}

inline VGint shIntMod(VGint a, VGint b)
{
  SH_ASSERT(b >= 0);

  if (!b) return 0;

  VGint m = a % b;

  if (m < 0) m += b;

  SH_ASSERT(m >= 0 && m < b);
  return m;

}


