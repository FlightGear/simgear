#ifndef _NAREF_H
#define _NAREF_H

/* Rather than play elaborate and complicated games with
 * platform-dependent endianness headers, just detect the platforms we
 * support.  This list is simpler and smaller, yet still quite
 * complete. */
#if (defined(__x86_64) && defined(__linux__)) || defined(__sparcv9) || \
    defined(__powerpc64__)
/* Win64 and Irix should work with this too, but have not been
 * tested */
# define NASAL_NAN64
#elif defined(_M_IX86) || defined(i386) || defined(__x86_64) || \
      defined(__ia64__) || defined(_M_IA64) || defined(__ARMEL__) || \
      defined(_M_X64) || defined(__alpha__) || \
      (defined(__sh__) && defined(__LITTLE_ENDIAN__))
# define NASAL_LE
#elif defined(__sparc) || defined(__ppc__) || defined(__PPC) || \
      defined (__powerpc__) || defined (__powerpc64__) || defined (__alpha__) || \
      defined(__mips) || defined(__ARMEB__) || \
      defined(__hppa__) || defined(__s390__) || defined(__s390x__) || \
      (defined(__sh__) && !defined(__LITTLE_ENDIAN__))
# define NASAL_BE
#else
# error Unrecognized CPU architecture
#endif

typedef union {
    struct naObj* obj;
    struct naStr* str;
    struct naVec* vec;
    struct naHash* hash;
    struct naCode* code;
    struct naFunc* func;
    struct naCCode* ccode;
    struct naGhost* ghost;
} naPtr;

/* On supported 64 bit platforms (those where all memory returned from
 * naAlloc() is guaranteed to lie between 0 and 2^48-1) we union the
 * double with the pointer, and use fancy tricks (see data.h) to make
 * sure all pointers are stored as NaNs.  32 bit layouts (and 64 bit
 * platforms where we haven't tested the trick above) need
 * endianness-dependent ordering to make sure that the reftag lies in
 * the top bits of the double */

#if defined(NASAL_LE)
typedef struct { naPtr ptr; int reftag; } naRefPart;
#elif defined(NASAL_BE)
typedef struct { int reftag; naPtr ptr; } naRefPart;
#endif

#if defined(NASAL_NAN64)
typedef union { double num; void* ptr; } naRef;
#else
typedef union { double num; naRefPart ref; } naRef;
#endif

#endif // _NAREF_H
