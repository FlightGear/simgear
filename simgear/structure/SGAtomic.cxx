/* -*-c++-*-
 *
 * Copyright (C) 2005-2006 Mathias Froehlich 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "SGAtomic.hxx"

#if defined(SGATOMIC_USE_GCC4_BUILTINS) && defined(__i386__)

// Usually the apropriate functions are inlined by gcc.
// But if gcc is called with something aequivalent to -march=i386,
// it will not assume that there is a lock instruction and instead
// calls this pair of functions. We will provide them here in this case.
// Note that this assembler code will not work on a i386 chip anymore.
// But I hardly believe that we can assume to run at least on a i486 ...

extern "C" {

unsigned __sync_sub_and_fetch_4(volatile void *ptr, unsigned value)
{
  register volatile unsigned* mem = reinterpret_cast<volatile unsigned*>(ptr);
  register unsigned result;
  __asm__ __volatile__("lock; xadd{l} {%0,%1|%1,%0}"
                       : "=r" (result), "=m" (*mem)
                       : "0" (-value), "m" (*mem)
                       : "memory");
  return result - value;
}

unsigned __sync_add_and_fetch_4(volatile void *ptr, unsigned value)
{
  register volatile unsigned* mem = reinterpret_cast<volatile unsigned*>(ptr);
  register unsigned result;
  __asm__ __volatile__("lock; xadd{l} {%0,%1|%1,%0}"
                       : "=r" (result), "=m" (*mem)
                       : "0" (value), "m" (*mem)
                       : "memory");
  return result + value;
}

void __sync_synchronize()
{
  __asm__ __volatile__("": : : "memory");
}

} // extern "C"

#endif
