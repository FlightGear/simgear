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

#ifndef __SH_MATH_H
#define __SH_MATH_H

#include "shDefs.h"
#include <VG/openvg.h>

VGint shAddSaturate(VGint a, VGint b);


VGint shClp2(VGint v);

VGint shIntMod(VGint a, VGint b);

#endif /* __SH_GEOMETRY_H */
