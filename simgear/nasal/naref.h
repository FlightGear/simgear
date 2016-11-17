#ifndef _NAREF_H
#define _NAREF_H

#if (defined(__x86_64) && defined(__linux__))
/* NASAL_NAN64 mode requires 64 bit pointers that only use the
 * lower 48 bits; x86 Win64 and Irix should work with this too, but
 * have not been tested */
# define NASAL_NAN64
#elif defined(__BYTE_ORDER__)
/* GCC and Clang define these (as a builtin, while
* __LITTLE_ENDIAN__ requires a header), MSVC doesn't */
# if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
#  define NASAL_LE
# elif __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
#  define NASAL_BE
# else
#  error Unrecognized endianness
# endif
#elif defined(_M_IX86) || defined(__i386) || defined(__x86_64) || \
      defined(__ia64__) || defined(_M_IA64) || defined(__ARMEL__) || \
      defined(_M_X64) || defined(_M_ARM)
# define NASAL_LE
#elif defined(__sparc) || defined(__ARMEB__) || \
      defined(__hppa__) || defined(__s390__) || defined(__s390x__)
# define NASAL_BE
#else
# error Unknown endianness
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
