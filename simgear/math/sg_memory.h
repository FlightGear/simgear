/**
 * \file sg_memory.h
 * memcpy/bcopy portability declarations (not actually used by anything
 * as best as I can tell.
 */

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


#ifndef _SG_MEMORY_H
#define _SG_MEMORY_H

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#ifdef HAVE_MEMCPY

#  ifdef HAVE_MEMORY_H
#    include <memory.h>
#  endif

#  define sgmemcmp            memcmp
#  define sgmemcpy            memcpy
#  define sgmemzero(dest,len) memset(dest,0,len)

#elif defined(HAVE_BCOPY)

#  define sgmemcmp              bcmp
#  define sgmemcpy(dest,src,n)  bcopy(src,dest,n)
#  define sgmemzero             bzero

#else

/* 
 * Neither memcpy() or bcopy() available.
 * Use substitutes provided be zlib.
 */

#  include <zutil.h>
#  define sgmemcmp zmemcmp
#  define sgmemcpy zmemcpy
#  define sgmemzero zmemzero

#endif

#endif // _SG_MEMORY_H


