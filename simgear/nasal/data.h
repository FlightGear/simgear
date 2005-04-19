#ifndef _DATA_H
#define _DATA_H

#include "nasal.h"

// Notes: A CODE object is a compiled set of bytecode instructions.
// What actually gets executed at runtime is a bound FUNC object,
// which combines the raw code with a namespace and a pointer to
// parent function in the lexical closure.
enum { T_STR, T_VEC, T_HASH, T_CODE, T_FUNC, T_CCODE, T_GHOST,
       NUM_NASAL_TYPES }; // V. important that this come last!

#define IS_REF(r) ((r).ref.reftag == NASAL_REFTAG)
#define IS_NUM(r) ((r).ref.reftag != NASAL_REFTAG)
#define IS_OBJ(r) (IS_REF((r)) && (r).ref.ptr.obj != 0)
//#define IS_OBJ(r) (IS_REF((r)) && (r).ref.ptr.obj != 0 && (((r).ref.ptr.obj->type == 123) ? *(int*)0 : 1))
#define IS_NIL(r) (IS_REF((r)) && (r).ref.ptr.obj == 0)
#define IS_STR(r) (IS_OBJ((r)) && (r).ref.ptr.obj->type == T_STR)
#define IS_VEC(r) (IS_OBJ((r)) && (r).ref.ptr.obj->type == T_VEC)
#define IS_HASH(r) (IS_OBJ((r)) && (r).ref.ptr.obj->type == T_HASH)
#define IS_CODE(r) (IS_OBJ((r)) && (r).ref.ptr.obj->type == T_CODE)
#define IS_FUNC(r) (IS_OBJ((r)) && (r).ref.ptr.obj->type == T_FUNC)
#define IS_CCODE(r) (IS_OBJ((r)) && (r).ref.ptr.obj->type == T_CCODE)
#define IS_GHOST(r) (IS_OBJ((r)) && (r).ref.ptr.obj->type == T_GHOST)
#define IS_CONTAINER(r) (IS_VEC(r)||IS_HASH(r))
#define IS_SCALAR(r) (IS_NUM((r)) || IS_STR((r)))
#define IDENTICAL(a, b) (IS_REF(a) && IS_REF(b) \
                         && a.ref.ptr.obj == b.ref.ptr.obj)

// This is a macro instead of a separate struct to allow compilers to
// avoid padding.  GCC on x86, at least, will always padd the size of
// an embedded struct up to 32 bits.  Doing it this way allows the
// implementing objects to pack in 16 bits worth of data "for free".
#define GC_HEADER \
    unsigned char mark; \
    unsigned char type

struct naObj {
    GC_HEADER;
};

struct naStr {
    GC_HEADER;
    int len;
    unsigned char* data;
    unsigned int hashcode;
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

struct HashRec {
    int size;
    int dels;
    int lgalloced;
    struct HashNode* nodes;
    struct HashNode* table[];
};

struct naHash {
    GC_HEADER;
    struct HashRec* rec;
};

struct naCode {
    GC_HEADER;
    unsigned char nArgs;
    unsigned char nOptArgs;
    unsigned char needArgVector;
    unsigned short nConstants;
    unsigned short nLines;
    unsigned short codesz;
    unsigned short* byteCode;
    naRef* constants;
    int* argSyms; // indices into constants
    int* optArgSyms;
    int* optArgVals;
    unsigned short* lineIps; // pairs of {ip, line}
    naRef srcFile;
    naRef restArgSym; // The "..." vector name, defaults to "arg"
};

struct naFunc {
    GC_HEADER;
    naRef code;
    naRef namespace;
    naRef next; // parent closure
};

struct naCCode {
    GC_HEADER;
    naCFunction fptr;
};

struct naGhost {
    GC_HEADER;
    naGhostType* gtype;
    void* ptr;
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

int naHash_tryset(naRef hash, naRef key, naRef val); // sets if exists
int naHash_sym(struct naHash* h, struct naStr* sym, naRef* out);
void naHash_newsym(struct naHash* h, naRef* sym, naRef* val);

void naGC_init(struct naPool* p, int type);
struct naObj** naGC_get(struct naPool* p, int n, int* nout);
void naGC_swapfree(void** target, void* val);
void naGC_freedead();

void naStr_gcclean(struct naStr* s);
void naVec_gcclean(struct naVec* s);
void naHash_gcclean(struct naHash* s);

#endif // _DATA_H
