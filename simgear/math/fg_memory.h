// fg_memory.h -- memcpy/bcopy portability declarations
//
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _FG_MEMORY_H
#define _FG_MEMORY_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_MEMCPY

#  ifdef HAVE_MEMORY_H
#    include <memory.h>
#  endif

#  define fgmemcmp            memcmp
#  define fgmemcpy            memcpy
#  define fgmemzero(dest,len) memset(dest,0,len)

#elif defined(HAVE_BCOPY)

#  define fgmemcmp              bcmp
#  define fgmemcpy(dest,src,n)  bcopy(src,dest,n)
#  define fgmemzero             bzero

#else

/* 
 * Neither memcpy() or bcopy() available.
 * Use substitutes provided be zlib.
 */

#  include <zutil.h>
#  define fgmemcmp zmemcmp
#  define fgmemcpy zmemcpy
#  define fgmemzero zmemzero

#endif

#endif // _FG_MEMORY_H


