#ifndef _CODE_H
#define _CODE_H

#include <setjmp.h>
#include "nasal.h"
#include "data.h"

#define MAX_STACK_DEPTH 512
#define MAX_RECURSION 128
#define MAX_MARK_DEPTH 128

// Number of objects (per pool per thread) asked for using naGC_get().
// The idea is that contexts can "cache" allocations to prevent thread
// contention on the global pools.  But in practice this interacts
// very badly with small subcontext calls, which grab huge numbers of
// cached objects and don't use them, causing far more collections
// than necessary.  Just leave it at 1 pending a rework of the
// collector synchronization.
#define OBJ_CACHE_SZ 1

enum {    
    OP_NOT, OP_MUL, OP_PLUS, OP_MINUS, OP_DIV, OP_NEG, OP_CAT, OP_LT, OP_LTE,
    OP_GT, OP_GTE, OP_EQ, OP_NEQ, OP_EACH, OP_JMP, OP_JMPLOOP, OP_JIFNOTPOP,
    OP_JIFEND, OP_FCALL, OP_MCALL, OP_RETURN, OP_PUSHCONST, OP_PUSHONE,
    OP_PUSHZERO, OP_PUSHNIL, OP_POP, OP_DUP, OP_XCHG, OP_INSERT, OP_EXTRACT,
    OP_MEMBER, OP_SETMEMBER, OP_LOCAL, OP_SETLOCAL, OP_NEWVEC, OP_VAPPEND,
    OP_NEWHASH, OP_HAPPEND, OP_MARK, OP_UNMARK, OP_BREAK, OP_SETSYM, OP_DUP2,
    OP_INDEX, OP_BREAK2, OP_PUSHEND, OP_JIFTRUE, OP_JIFNOT, OP_FCALLH,
    OP_MCALLH, OP_XCHG2, OP_UNPACK, OP_SLICE, OP_SLICE2, OP_BIT_AND, OP_BIT_OR,
    OP_BIT_XOR, OP_BIT_NEG
};

struct Frame {
    naRef func; // naFunc object
    naRef locals; // local per-call namespace
    int ip; // instruction pointer into code
    int bp; // opStack pointer to start of frame
};

struct Globals {
    // Garbage collecting allocators:
    struct naPool pools[NUM_NASAL_TYPES];
    int allocCount;

    // Dead blocks waiting to be freed when it is safe
    void** deadBlocks;
    int deadsz;
    int ndead;
    
    // Threading stuff
    int nThreads;
    int waitCount;
    int needGC;
    int bottleneck;
    void* sem;
    void* lock;

    // Constants
    naRef meRef;
    naRef argRef;
    naRef parentsRef;

    // A hash of symbol names
    naRef symbols;

    // Vector/hash containing objects which should not be freed by the gc
    // TODO do we need a separate vector and hash?
    naRef save;
    naRef save_hash;
    int next_gc_key;

    struct Context* freeContexts;
    struct Context* allContexts;
};

struct Context {
    // Stack(s)
    struct Frame fStack[MAX_RECURSION];
    int fTop;
    naRef opStack[MAX_STACK_DEPTH];
    int opFrame; // like Frame::bp, but for C functions
    int opTop;
    int markStack[MAX_MARK_DEPTH];
    int markTop;

    // Free object lists, cached from the global GC
    struct naObj** free[NUM_NASAL_TYPES];
    int nfree[NUM_NASAL_TYPES];

    // GC-findable reference point for objects that may live on the
    // processor ("real") stack during execution.  naNew() places them
    // here, and clears the array each instruction
    struct naObj** temps;
    int ntemps;
    int tempsz;

    // Error handling
    jmp_buf jumpHandle;
    char error[128];
    naRef dieArg;

    // Sub-call lists
    struct Context* callParent;
    struct Context* callChild;

    // Linked list pointers in globals
    struct Context* nextFree;
    struct Context* nextAll;

    void* userData;
};

#define globals nasal_globals
extern struct Globals* globals;

// Threading low-level functions
void* naNewLock();
void naFreeLock(void* lock);
void naLock(void* lock);
void naUnlock(void* lock);
void* naNewSem();
void naFreeSem(void* sem);
void naSemDown(void* sem);
void naSemUp(void* sem, int count);

void naCheckBottleneck();

#define LOCK() naLock(globals->lock)
#define UNLOCK() naUnlock(globals->lock)

#endif // _CODE_H
