/* -*-c++-*-
 *
 * Copyright (C) 2005-2009,2011 Mathias Froehlich 
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

#if defined(__GNUC__) && ((4 < __GNUC__)||(4 == __GNUC__ && 1 <= __GNUC_MINOR__)) && \
    defined(__x86_64__)
// No need to include something. Is a Compiler API ...
# define SGATOMIC_USE_GCC4_BUILTINS
#elif defined(__GNUC__) && defined(__i386__)
# define SGATOMIC_USE_LIBRARY_FUNCTIONS
#elif defined(__sgi) && defined(_COMPILER_VERSION) && (_COMPILER_VERSION>=730)
// No need to include something. Is a Compiler API ...
# define SGATOMIC_USE_MIPSPRO_BUILTINS
#elif defined(_WIN32)
# define SGATOMIC_USE_LIBRARY_FUNCTIONS
#else
// The sledge hammer ...
# define SGATOMIC_USE_LIBRARY_FUNCTIONS
# define SGATOMIC_USE_MUTEX
# include <simgear/threads/SGThread.hxx>
#endif

class SGAtomic {
public:
  SGAtomic(unsigned value = 0) : mValue(value)
  { }

#if defined(SGATOMIC_USE_LIBRARY_FUNCTIONS)
  unsigned operator++();
#else
  unsigned operator++()
  {
# if defined(SGATOMIC_USE_GCC4_BUILTINS)
    return __sync_add_and_fetch(&mValue, 1);
# elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
    return __add_and_fetch(&mValue, 1);
# else
#  error
# endif
  }
#endif

#if defined(SGATOMIC_USE_LIBRARY_FUNCTIONS)
  unsigned operator--();
#else
  unsigned operator--()
  {
# if defined(SGATOMIC_USE_GCC4_BUILTINS)
    return __sync_sub_and_fetch(&mValue, 1);
# elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
    return __sub_and_fetch(&mValue, 1);
# else
#  error
# endif
  }
#endif

#if defined(SGATOMIC_USE_LIBRARY_FUNCTIONS)
  operator unsigned() const;
#else
  operator unsigned() const
  {
# if defined(SGATOMIC_USE_GCC4_BUILTINS)
    __sync_synchronize();
    return mValue;
# elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
    __synchronize();
    return mValue;
# else
#  error
# endif
  }
#endif

#if defined(SGATOMIC_USE_LIBRARY_FUNCTIONS)
  bool compareAndExchange(unsigned oldValue, unsigned newValue);
#else
  bool compareAndExchange(unsigned oldValue, unsigned newValue)
  {
# if defined(SGATOMIC_USE_GCC4_BUILTINS)
    return __sync_bool_compare_and_swap(&mValue, oldValue, newValue);
# elif defined(SGATOMIC_USE_MIPOSPRO_BUILTINS)
    return __compare_and_swap(&mValue, oldValue, newValue);
# else
#  error
# endif
  }
#endif

private:
  SGAtomic(const SGAtomic&);
  SGAtomic& operator=(const SGAtomic&);

#if defined(SGATOMIC_USE_MUTEX)
  mutable SGMutex mMutex;
#endif
  unsigned mValue;
};

namespace simgear
{
// Typesafe wrapper around SGSwappable
template <typename T>
class Swappable : private SGAtomic
{
public:
    Swappable(const T& value) : SGAtomic(static_cast<unsigned>(value))
    {
    }
    T operator() () const
    {
        return static_cast<T>(SGAtomic::operator unsigned ());
    }
    Swappable& operator=(const Swappable& rhs)
    {
        for (unsigned oldval = unsigned(*this);
             !compareAndExchange(oldval, unsigned(rhs));
             oldval = unsigned(*this))
            ;
        return *this;
    }
    bool compareAndSwap(const T& oldVal, const T& newVal)
    {
        return SGAtomic::compareAndExchange(static_cast<unsigned>(oldVal),
                                           static_cast<unsigned>(newVal));
    }
};
}
#endif
