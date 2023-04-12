/*
 * Copyright (c) 2019 Vincenzo Pupillo
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

#ifndef __SH_COMMONS_H
#define __SH_COMMONS_H

#include "shDefs.h"

/*-----------------------------------------------------------
 * Draws a GL_QUADS using glDrawArrays
 *-----------------------------------------------------------*/
void shDrawQuads(GLfloat v1x, GLfloat v1y, GLfloat v2x, GLfloat v2y, GLfloat v3x, GLfloat v3y, GLfloat v4x, GLfloat v4y);

/*-----------------------------------------------------------
 * Draws a GL_QUADS using glDrawArrays
 *-----------------------------------------------------------*/
void shDrawQuadsInt(GLint v1x, GLint v1y, GLint v2x, GLint v2y, GLint v3x, GLint v3y, GLint v4x, GLint v4y);

/*-----------------------------------------------------------
 * Draws a GL_QUADS using glDrawArrays
 *-----------------------------------------------------------*/
void shDrawQuadsArray(GLfloat v[8]);

#endif /* __SH_COMMONS_H */
