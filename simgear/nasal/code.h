#ifndef _CODE_H
#define _CODE_H

#include <setjmp.h>
#include "nasal.h"
#include "data.h"

#define MAX_STACK_DEPTH 1024
#define MAX_RECURSION 128
#define MAX_MARK_DEPTH 32

enum {    
    OP_AND, OP_OR, OP_NOT, OP_MUL, OP_PLUS, OP_MINUS, OP_DIV, OP_NEG,
    OP_CAT, OP_LT, OP_LTE, OP_GT, OP_GTE, OP_EQ, OP_NEQ, OP_EACH,
    OP_JMP, OP_JIFNOT, OP_JIFNIL, OP_FCALL, OP_MCALL, OP_RETURN,
    OP_PUSHCONST, OP_PUSHONE, OP_PUSHZERO, OP_PUSHNIL, OP_POP,
    OP_DUP, OP_XCHG, OP_INSERT, OP_EXTRACT, OP_MEMBER, OP_SETMEMBER,
    OP_LOCAL, OP_SETLOCAL, OP_NEWVEC, OP_VAPPEND, OP_NEWHASH, OP_HAPPEND,
    OP_LINE, OP_MARK, OP_UNMARK, OP_BREAK, OP_NEWARGS
};

struct Frame {
    naRef func; // naFunc object
    naRef locals; // local per-call namespace
    naRef args; // vector of arguments
    int ip; // instruction pointer into code
    int bp; // opStack pointer to start of frame
    int line; // current line number
};

struct Context {
    // Garbage collecting allocators:
    struct naPool pools[NUM_NASAL_TYPES];

    // Stack(s)
    struct Frame fStack[MAX_RECURSION];
    int fTop;
    naRef opStack[MAX_STACK_DEPTH];
    int opTop;
    int markStack[MAX_MARK_DEPTH];
    int markTop;
    int done;

    // Vector of arguments vectors.  A LIFO cache, basically, to avoid
    // thrashing the GC just for function call arguments.
    naRef argPool;

    // Constants
    naRef meRef;
    naRef argRef;
    naRef parentsRef;

    // Error handling
    jmp_buf jumpHandle;
    char* error;

    // GC-findable reference point for objects that may live on the
    // processor ("real") stack during execution.  naNew() places them
    // here, and clears the array each time we return from a C
    // function.
    naRef temps;

    naRef save;
};

void printRefDEBUG(naRef r);

#endif // _CODE_H
