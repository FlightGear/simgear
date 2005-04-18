#ifndef _CODE_H
#define _CODE_H

#include <setjmp.h>
#include "nasal.h"
#include "data.h"

#define MAX_STACK_DEPTH 512
#define MAX_RECURSION 128
#define MAX_MARK_DEPTH 128

// Number of objects (per pool per thread) asked for using naGC_get().
// Testing with fib.nas shows that this gives the best performance,
// without too much per-thread overhead.
#define OBJ_CACHE_SZ 128

enum {    
    OP_AND, OP_OR, OP_NOT, OP_MUL, OP_PLUS, OP_MINUS, OP_DIV, OP_NEG,
    OP_CAT, OP_LT, OP_LTE, OP_GT, OP_GTE, OP_EQ, OP_NEQ, OP_EACH,
    OP_JMP, OP_JMPLOOP, OP_JIFNOT, OP_JIFNIL, OP_FCALL, OP_MCALL, OP_RETURN,
    OP_PUSHCONST, OP_PUSHONE, OP_PUSHZERO, OP_PUSHNIL, OP_POP,
    OP_DUP, OP_XCHG, OP_INSERT, OP_EXTRACT, OP_MEMBER, OP_SETMEMBER,
    OP_LOCAL, OP_SETLOCAL, OP_NEWVEC, OP_VAPPEND, OP_NEWHASH, OP_HAPPEND,
    OP_MARK, OP_UNMARK, OP_BREAK, OP_FTAIL, OP_MTAIL, OP_SETSYM
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

    naRef save;

    struct Context* freeContexts;
    struct Context* allContexts;
};

struct Context {
    // Stack(s)
    struct Frame fStack[MAX_RECURSION];
    int fTop;
    naRef opStack[MAX_STACK_DEPTH];
    int opTop;
    int markStack[MAX_MARK_DEPTH];
    int markTop;

    // Free object lists, cached from the global GC
    struct naObj** free[NUM_NASAL_TYPES];
    int nfree[NUM_NASAL_TYPES];

    // GC-findable reference point for objects that may live on the
    // processor ("real") stack during execution.  naNew() places them
    // here, and clears the array each instruction
    naRef temps;

    // Error handling
    jmp_buf jumpHandle;
    char* error;
    naRef dieArg;

    // Sub-call lists
    struct Context* callParent;
    struct Context* callChild;

    // Linked list pointers in globals
    struct Context* nextFree;
    struct Context* nextAll;
};

#define globals nasal_globals
extern struct Globals* globals;

// Threading low-level functions
void* naNewLock();
void naLock(void* lock);
void naUnlock(void* lock);
void* naNewSem();
void naSemDown(void* sem);
void naSemUpAll(void* sem, int count);

void naCheckBottleneck();

#define LOCK() naLock(globals->lock)
#define UNLOCK() naUnlock(globals->lock)

#endif // _CODE_H
