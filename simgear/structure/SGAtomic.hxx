/* -*-c++-*-
 *
 * Copyright (C) 2005-2009 Mathias Froehlich 
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

  bool compareAndExchange(unsigned oldValue, unsigned newValue)
  {
#if defined(SGATOMIC_USE_GCC4_BUILTINS)
    return __sync_bool_compare_and_swap(&mValue, oldValue, newValue);
#elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
    return __compare_and_swap(&mValue, oldValue, newValue);
#elif defined(SGATOMIC_USE_WIN32_INTERLOCKED)
    long volatile* lvPtr = reinterpret_cast<long volatile*>(&mValue);
    return oldValue == InterlockedCompareExchange(lvPtr, newValue, oldValue);
#else
    SGGuard<SGMutex> lock(mMutex);
    if (mValue != oldValue)
      return false;
    mValue = newValue;
    return true;
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
  unsigned mValue;
};

// Value that can be atomically compared and swapped.
class SGSwappable
{
public:
    typedef unsigned long value_type;
    SGSwappable(unsigned long value = 0) : mValue(value) {}
    operator unsigned long() const
    {
#if defined(SGATOMIC_USE_GCC4_BUILTINS)
        __sync_synchronize();
        return mValue;
#elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
        __synchronize();
        return mValue;
#elif defined(SGATOMIC_USE_WIN32_INTERLOCKED)
        return static_cast<long const volatile &>(mValue);
#else
        SGGuard<SGMutex> lock(mMutex);
        return mValue;
#endif
    }

    bool compareAndSwap(unsigned long oldVal, unsigned long newVal)
    {
#if defined(SGATOMIC_USE_GCC4_BUILTINS)
        return __sync_bool_compare_and_swap(&mValue, oldVal, newVal);
#elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
        return __compare_and_swap(&mValue, oldVal, newVal);
#elif defined(SGATOMIC_USE_WIN32_INTERLOCKED)
        long previous
            = InterlockedCompareExchange(reinterpret_cast<long volatile*>(&mValue),
                                         (long)newVal,
                                         (long)oldVal);
        return previous == (long)oldVal;
#else
        SGGuard<SGMutex> lock(mMutex);
        if (oldVal == mValue) {
            mValue = newVal;
            return true;
        } else {
            return false;
        }
#endif
    }

private:
    SGSwappable(const SGAtomic&);
    SGSwappable& operator=(const SGAtomic&);

#if !defined(SGATOMIC_USE_GCC4_BUILTINS)        \
    && !defined(SGATOMIC_USE_MIPOSPRO_BUILTINS) \
    && !defined(SGATOMIC_USE_WIN32_INTERLOCKED)
    mutable SGMutex mMutex;
#endif
#ifdef SGATOMIC_USE_WIN32_INTERLOCKED
    __declspec(align(32))
#endif
    value_type mValue;

};

namespace simgear
{
// Typesafe wrapper around SGSwappable
template <typename T>
class Swappable : private SGSwappable
{
public:
    Swappable(const T& value) : SGSwappable(static_cast<value_type>(value))
    {
    }
    T operator() () const
    {
        return static_cast<T>(SGSwappable::operator unsigned long ());
    }
    bool compareAndSwap(const T& oldVal, const T& newVal)
    {
        return SGSwappable::compareAndSwap(static_cast<value_type>(oldVal),
                                           static_cast<value_type>(newVal));
    }
};
}
#endif
