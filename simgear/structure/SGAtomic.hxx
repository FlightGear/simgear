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

#ifndef SGAtomic_HXX
#define SGAtomic_HXX

#if defined(__GNUC__) && (4 <= __GNUC__) && (1 <= __GNUC_MINOR__) \
  && (defined(__i386__) || defined(__x86_64__))
// No need to include something. Is a Compiler API ...
# define SGATOMIC_USE_GCC4_BUILTINS
#elif defined(__sgi) && defined(_COMPILER_VERSION) && (_COMPILER_VERSION>=730)
// No need to include something. Is a Compiler API ...
# define SGATOMIC_USE_MIPSPRO_BUILTINS
#elif defined(WIN32)  && !defined ( __CYGWIN__ )
# include <windows.h>
# define SGATOMIC_USE_WIN32_INTERLOCKED
#else
// The sledge hammer ...
# include <simgear/threads/SGThread.hxx>
# include <simgear/threads/SGGuard.hxx>
#endif

class SGAtomic {
public:
  SGAtomic(unsigned value = 0) : mValue(value)
  { }
  unsigned operator++()
  {
#if defined(SGATOMIC_USE_GCC4_BUILTINS)
    return __sync_add_and_fetch(&mValue, 1);
#elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
    return __add_and_fetch(&mValue, 1);
#elif defined(SGATOMIC_USE_WIN32_INTERLOCKED)
    return InterlockedIncrement(reinterpret_cast<long volatile*>(&mValue));
#else
    SGGuard<SGMutex> lock(mMutex);
    return ++mValue;
#endif
  }
  unsigned operator--()
  {
#if defined(SGATOMIC_USE_GCC4_BUILTINS)
    return __sync_sub_and_fetch(&mValue, 1);
#elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
    return __sub_and_fetch(&mValue, 1);
#elif defined(SGATOMIC_USE_WIN32_INTERLOCKED)
    return InterlockedDecrement(reinterpret_cast<long volatile*>(&mValue));
#else
    SGGuard<SGMutex> lock(mMutex);
    return --mValue;
#endif
  }
  operator unsigned() const
  {
#if defined(SGATOMIC_USE_GCC4_BUILTINS)
    __sync_synchronize();
    return mValue;
#elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
    __synchronize();
    return mValue;
#elif defined(SGATOMIC_USE_WIN32_INTERLOCKED)
    return static_cast<unsigned const volatile &>(mValue);
#else
    SGGuard<SGMutex> lock(mMutex);
    return mValue;
#endif
 }

private:
  SGAtomic(const SGAtomic&);
  SGAtomic& operator=(const SGAtomic&);

#if !defined(SGATOMIC_USE_GCC4_BUILTINS) \
  && !defined(SGATOMIC_USE_MIPOSPRO_BUILTINS) \
  && !defined(SGATOMIC_USE_WIN32_INTERLOCKED)
  mutable SGMutex mMutex;
#endif
#ifdef SGATOMIC_USE_WIN32_INTERLOCKED
  __declspec(align(32))
#endif
  unsigned mValue;
};

#endif
