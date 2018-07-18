
#ifndef _STDINT_HXX
#define _STDINT_HXX 1

// Copyright (C) 1999  Curtis L. Olson - http://www.flightgear.org/~curt
//
// Written by Curtis Olson - http://www.flightgear.org/~curt
// Started September 2001.
//
// This file is in the Public Domain, and comes with no warranty.
//
// $Id$


#include <cstdint>
#include <cstdlib> // for _byteswap_foo on Win32

#if defined(_MSC_VER)
using ssize_t = int64_t; // this is a POSIX type, not a C one
#endif

inline uint16_t sg_bswap_16(uint16_t x) {
#if defined(__llvm__) || \
 (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)) && !defined(__ICC)
    return __builtin_bswap16(x);
#elif defined(_MSC_VER) && !defined(_DEBUG)
   return _byteswap_ushort(x);
#else
    x = (x >> 8) | (x << 8);
    return x;
#endif
}

inline uint32_t sg_bswap_32(uint32_t x) {
#if defined(__llvm__) || \
 (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && !defined(__ICC)
    return __builtin_bswap32(x);
#elif defined(_MSC_VER) && !defined(_DEBUG)
   return _byteswap_ulong(x);
#else
    x = ((x >>  8) & 0x00FF00FFL) | ((x <<  8) & 0xFF00FF00L);
    x = (x >> 16) | (x << 16);
    return x;
#endif
}

inline uint64_t sg_bswap_64(uint64_t x) {
#if defined(__llvm__) || \
 (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)) && !defined(__ICC)
   return __builtin_bswap64(x);
#elif defined(_MSC_VER) && !defined(_DEBUG)
   return _byteswap_uint64(x);
#else
    x = ((x >>  8) & 0x00FF00FF00FF00FFLL) | ((x <<  8) & 0xFF00FF00FF00FF00LL);
    x = ((x >> 16) & 0x0000FFFF0000FFFFLL) | ((x << 16) & 0xFFFF0000FFFF0000LL);
    x = (x >> 32) | (x << 32);
    return x;
#endif
}


inline bool sgIsLittleEndian() {
    static const int sgEndianTest = 1;
    return (*((char *) &sgEndianTest ) != 0);
}

inline bool sgIsBigEndian() {
    static const int sgEndianTest = 1;
    return (*((char *) &sgEndianTest ) == 0);
}

inline void sgEndianSwap(int32_t *x) { *x = (int32_t) sg_bswap_32((int32_t) *x); }
inline void sgEndianSwap(float *x) { *x = (float) sg_bswap_32((float) *x); }

inline void sgEndianSwap(uint16_t *x) { *x = sg_bswap_16(*x); }
inline void sgEndianSwap(uint32_t *x) { *x = sg_bswap_32(*x); }
inline void sgEndianSwap(uint64_t *x) { *x = sg_bswap_64(*x); }



#endif // !_STDINT_HXX

