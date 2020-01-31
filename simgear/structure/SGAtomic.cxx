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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include "SGAtomic.hxx"

#if defined(SGATOMIC_USE_LIBRARY_FUNCTIONS)

#if defined(_WIN32)
# include <windows.h>
#elif defined(GCC_ATOMIC_BUILTINS_FOUND)
#elif defined(__GNUC__) && defined(__i386__)
#elif defined(SGATOMIC_USE_MUTEX)
# include <mutex>
#else
# error
#endif

unsigned
SGAtomic::operator++()
{
#if defined(_WIN32)
    return InterlockedIncrement(reinterpret_cast<long volatile*>(&mValue));
#elif defined(GCC_ATOMIC_BUILTINS_FOUND)
    return __sync_add_and_fetch(&mValue, 1);
#elif defined(__GNUC__) && defined(__i386__)
    register volatile unsigned* mem = reinterpret_cast<volatile unsigned*>(&mValue);
    register unsigned result;
    __asm__ __volatile__("lock; xadd{l} {%0,%1|%1,%0}"
                         : "=r" (result), "=m" (*mem)
                         : "0" (1), "m" (*mem)
                         : "memory");
    return result + 1;
#else
    std::lock_guard<std::mutex> lock(mMutex);
    return ++mValue;
#endif
}

unsigned
SGAtomic::operator--()
{
#if defined(_WIN32)
    return InterlockedDecrement(reinterpret_cast<long volatile*>(&mValue));
#elif defined(GCC_ATOMIC_BUILTINS_FOUND)
    return __sync_sub_and_fetch(&mValue, 1);
#elif defined(__GNUC__) && defined(__i386__)
    register volatile unsigned* mem = reinterpret_cast<volatile unsigned*>(&mValue);
    register unsigned result;
    __asm__ __volatile__("lock; xadd{l} {%0,%1|%1,%0}"
                         : "=r" (result), "=m" (*mem)
                         : "0" (-1), "m" (*mem)
                         : "memory");
    return result - 1;
#else
    std::lock_guard<std::mutex> lock(mMutex);
    return --mValue;
#endif
}

SGAtomic::operator unsigned() const
{
#if defined(_WIN32)
    return static_cast<unsigned const volatile &>(mValue);
#elif defined(GCC_ATOMIC_BUILTINS_FOUND)
    __sync_synchronize();
    return mValue;
#elif defined(__GNUC__) && defined(__i386__)
    __asm__ __volatile__("": : : "memory");
    return mValue;
#else
    std::lock_guard<std::mutex> lock(mMutex);
    return mValue;
#endif
}

bool
SGAtomic::compareAndExchange(unsigned oldValue, unsigned newValue)
{
#if defined(_WIN32)
    long volatile* lvPtr = reinterpret_cast<long volatile*>(&mValue);
    return oldValue == InterlockedCompareExchange(lvPtr, newValue, oldValue);
#elif defined(GCC_ATOMIC_BUILTINS_FOUND)
    return __sync_bool_compare_and_swap(&mValue, oldValue, newValue);
#elif defined(__GNUC__) && defined(__i386__)
    register volatile unsigned* mem = reinterpret_cast<volatile unsigned*>(&mValue);
    unsigned before;
    __asm__ __volatile__("lock; cmpxchg{l} {%1,%2|%1,%2}"
                         : "=a"(before)
                         : "q"(newValue), "m"(*mem), "0"(oldValue)
                         : "memory");
    return before == oldValue;
#else
    std::lock_guard<std::mutex> lock(mMutex);
    if (mValue != oldValue)
        return false;
    mValue = newValue;
    return true;
#endif
}

#endif
