/**
 * \file sg_zlib.h
 * A file IO wrapper system that can select at compile time if the system
 * should use zlib calls or normal uncompressed calls for systems that have
 * problems building zlib.
 * Define AVOID_USING_ZLIB to use standard uncompressed calls.
 */

/*
 * Written by Curtis Olson, started April 1998.
 *
 * Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * $Id$
 **************************************************************************/


#ifndef _SG_ZLIB_H
#define _SG_ZLIB_H


#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif


#ifdef AVOID_USING_ZLIB

  #include <stdio.h>

  #define sgFile FILE *

  /* sgFile sgopen(char *filename, const char *flags) */
  #define sgopen(P, F)  (fopen((P), (F)))

  /* int sgseek(sgFile *file, long offset, int whence) */
  #define sgseek(F, O, W)  (fseek((F), (O), (W)))

  /* sgread(sgFile file, void *buf, int size); */
  #define sgread(F, B, S)  (fread((B), (S), 1, (F)))

  /* int sggets(sgFile fd, char *buffer, int len) */
  #define sggets(F, B, L)  (sgets((B), (L), (F)))

  /* int sgclose(sgFile fd) */
  #define sgclose(F)  (fclose((F)))
#else

  #include <zlib.h>

  #define sgFile gzFile

  /* sgFile sgopen(char *filename, const char *flags) */
  #define sgopen(P, F)  (gzopen((P), (F)))

  /* int sgseek(sgFile *file, long offset, int whence) */
  #define sgseek(F, O, W)  (gzseek((F), (O), (W)))

  /* sgread(sgFile file, void *buf, int size); */
  #define sgread(F, B, S)  (gzread((F), (B), (S)))

  /* int sggets(sgFile fd, char *buffer, int len) */
  #define sggets(F, B, L)  (gzgets((F), (B), (L)))

  /* int sgclose(sgFile fd) */
  #define sgclose(F)  (gzclose((F)))

#endif /* #ifdef AVOID_USING_ZLIB #else #endif */


#endif /* _SG_ZLIB_H */


