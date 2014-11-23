#ifndef _DATA_H
#define _DATA_H

#include "nasal.h"

#if defined(NASAL_NAN64)

// On 64 bit systems, Nasal non-numeric references are stored with a
// bitmask that sets the top 16 bits.  As a double, this is a
// signalling NaN that cannot itself be produced by normal numerics
// code.  The pointer value can be reconstructed if (and only if) we
// are guaranteed that all memory that can be pointed to by a naRef
// (i.e. all memory returned by naAlloc) lives in the bottom 48 bits
// of memory.  Linux on x86_64, Win64, Solaris and Irix all have such
// policies with address spaces:
//
// http://msdn.microsoft.com/library/en-us/win64/win64/virtual_address_space.asp
// http://docs.sun.com/app/docs/doc/816-5138/6mba6ua5p?a=view
// http://techpubs.sgi.com/library/tpl/cgi-bin/getdoc.cgi/
//  ...   0650/bks/SGI_Developer/books/T_IRIX_Prog/sgi_html/ch01.html
//
// In the above, MS guarantees 44 bits of process address space, SGI
// 40, and Sun 43 (Solaris *does* place the stack in the "negative"
// address space at 0xffff..., but we don't care as naRefs will never
// point there).  Linux doesn't document this rigorously, but testing
// shows that it allows 47 bits of address space (and current x86_64
// implementations are limited to 48 bits of virtual space anyway). So
// we choose 48 as the conservative compromise.

#define REFMAGIC ((1UL<<48) - 1)

#define _ULP(r) ((unsigned long long)((r).ptr))
#define REFPTR(r) (_ULP(r) & REFMAGIC)
#define IS_REF(r) ((_ULP(r) & ~REFMAGIC) == ~REFMAGIC)

// Portability note: this cast from a pointer type to naPtr (a union)
// is not defined in ISO C, it's a GCC extention that doesn't work on
// (at least) either the SUNWspro or MSVC compilers.  Unfortunately,
// fixing this would require abandoning the naPtr union for a set of
// PTR_<type>() macros, which is a ton of work and a lot of extra
// code.  And as all enabled 64 bit platforms are gcc anyway, and the
// 32 bit fallback code works in any case, this is acceptable for now.
#define PTR(r) ((naPtr)((struct naObj*)(_ULP(r) & REFMAGIC)))

#define SETPTR(r, p) ((r).ptr = (void*)((unsigned long long)p | ~REFMAGIC))
#define SETNUM(r, n) ((r).num = n)

#else

// On 32 bit systems where the pointer is half the width of the
// double, we store a special magic number in the structure to make
// the double a qNaN.  This must appear in the top bits of the double,
// which is why the structure layout is endianness-dependent.
// qNaN (quiet NaNs) use range 0x7ff80000-0x7fffffff

#define NASAL_REFTAG 0x7fff6789 // == 2,147,444,617 decimal
#define IS_REF(r) ((r).ref.reftag == NASAL_REFTAG)
#define PTR(r) ((r).ref.ptr)

#define SETPTR(r, p) ((r).ref.ptr.obj = (void*)p, (r).ref.reftag = NASAL_REFTAG)
#define SETNUM(r, n) ((r).ref.reftag = ~NASAL_REFTAG, (r).num = n)

#endif /* platform stuff */

enum { T_STR, T_VEC, T_HASH, T_CODE, T_FUNC, T_CCODE, T_GHOST,
       NUM_NASAL_TYPES }; // V. important that this come last!

#define IS_NUM(r) (!IS_REF(r))
#define IS_OBJ(r) (IS_REF(r) && PTR(r).obj != 0)
#define IS_NIL(r) (IS_REF(r) && PTR(r).obj == 0)
#define IS_STR(r) (IS_OBJ(r) && PTR(r).obj->type == T_STR)
#define IS_VEC(r) (IS_OBJ(r) && PTR(r).obj->type == T_VEC)
#define IS_HASH(r) (IS_OBJ(r) && PTR(r).obj->type == T_HASH)
#define IS_CODE(r) (IS_OBJ(r) && PTR(r).obj->type == T_CODE)
#define IS_FUNC(r) (IS_OBJ(r) && PTR(r).obj->type == T_FUNC)
#define IS_CCODE(r) (IS_OBJ(r) && PTR(r).obj->type == T_CCODE)
#define IS_GHOST(r) (IS_OBJ(r) && PTR(r).obj->type == T_GHOST)
#define IS_CONTAINER(r) (IS_VEC(r)||IS_HASH(r))
#define IS_SCALAR(r) (IS_NUM(r) || IS_STR(r))
#define IDENTICAL(a, b) (IS_REF(a) && IS_REF(b) && PTR(a).obj == PTR(b).obj)

#define MUTABLE(r) (IS_STR(r) && PTR(r).str->hashcode == 0)

// This is a macro instead of a separate struct to allow compilers to
// avoid padding.  GCC on x86, at least, will always pad the size of
// an embedded struct up to 32 bits.  Doing it this way allows the
// implementing objects to pack in 16 bits worth of data "for free".
#define GC_HEADER \
    unsigned char mark; \
    unsigned char type

struct naObj {
    GC_HEADER;
};

#define MAX_STR_EMBLEN 15
struct naStr {
    GC_HEADER;
    signed char emblen; /* [0-15], or -1 to indicate "not embedded" */
    unsigned int hashcode;
    union {
        unsigned char buf[16];
        struct {
            int len;
            unsigned char* ptr;
        } ref;
    } data;
};

struct VecRec {
    int size;
    int alloced;
    naRef array[];
};

struct naVec {
    GC_HEADER;
    struct VecRec* rec;
};

struct HashNode {
    naRef key;
    naRef val;
    struct HashNode* next;
};

struct naHash {
    GC_HEADER;
    struct HashRec* rec;
};

struct naCode {
    GC_HEADER;
    unsigned int nArgs : 5;
    unsigned int nOptArgs : 5;
    unsigned int needArgVector : 1;
    unsigned short nConstants;
    unsigned short codesz;
    unsigned short restArgSym; // The "..." vector name, defaults to "arg"
    unsigned short nLines;
    naRef srcFile;
    naRef* constants;
};

/* naCode objects store their variable length arrays in a single block
 * starting with their constants table.  Compute indexes at runtime
 * for space efficiency: */
#define BYTECODE(c) ((unsigned short*)((c)->constants+(c)->nConstants))
#define ARGSYMS(c) (BYTECODE(c)+(c)->codesz)
#define OPTARGSYMS(c) (ARGSYMS(c)+(c)->nArgs)
#define OPTARGVALS(c) (OPTARGSYMS(c)+(c)->nOptArgs)
#define LINEIPS(c) (OPTARGVALS(c)+(c)->nOptArgs)

struct naFunc {
    GC_HEADER;
    naRef code;
    naRef namespace;
    naRef next; // parent closure
};

struct naCCode {
    GC_HEADER;
    union {
        naCFunction fptr; //!< pointer to simple callback function. Invalid if
                          //   fptru is not NULL.
        struct {
            void* user_data;
            void(*destroy)(void*);
            naCFunctionU fptru;
        };
    };
};

struct naGhost {
    GC_HEADER;
    naGhostType* gtype;
    void* ptr;
    naRef data; //!< Nasal data bound to the lifetime of the ghost.
};

struct naPool {
    int           type;
    int           elemsz;
    struct Block* blocks;
    void**   free0; // pointer to the alloced buffer
    int     freesz; // size of the alloced buffer
    void**    free; // current "free frame"
    int      nfree; // down-counting index within the free frame
    int    freetop; // curr. top of the free list
};

void naFree(void* m);
void* naAlloc(int n);
void* naRealloc(void* buf, int sz);
void naBZero(void* m, int n);

int naTypeSize(int type);
naRef naObj(int type, struct naObj* o);
naRef naNew(naContext c, int type);
naRef naNewCode(naContext c);

int naStr_equal(naRef s1, naRef s2);
naRef naStr_fromnum(naRef dest, double num);
int naStr_numeric(naRef str);
int naStr_parsenum(char* str, int len, double* result);
int naStr_tonum(naRef str, double* out);
naRef naStr_buf(naRef str, int len);

int naiHash_tryset(naRef hash, naRef key, naRef val); // sets if exists
int naiHash_sym(struct naHash* h, struct naStr* sym, naRef* out);
void naiHash_newsym(struct naHash* h, naRef* sym, naRef* val);

void naGC_init(struct naPool* p, int type);
struct naObj** naGC_get(struct naPool* p, int n, int* nout);
void naGC_swapfree(void** target, void* val);
void naGC_freedead();
void naiGCMark(naRef r);
void naiGCMarkHash(naRef h);

void naStr_gcclean(struct naStr* s);
void naVec_gcclean(struct naVec* s);
void naiGCHashClean(struct naHash* h);

#endif // _DATA_H
