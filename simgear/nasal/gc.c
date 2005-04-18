#include "nasal.h"
#include "data.h"
#include "code.h"

#define MIN_BLOCK_SIZE 256

// "type" for an object freed by the collector
#define T_GCFREED 123 // DEBUG

static void reap(struct naPool* p);
static void mark(naRef r);

struct Block {
    int   size;
    char* block;
    struct Block* next;
};

// Must be called with the giant exclusive lock!
static void freeDead()
{
    int i;
    for(i=0; i<globals->ndead; i++)
        naFree(globals->deadBlocks[i]);
    globals->ndead = 0;
}


// Must be called with the big lock!
static void garbageCollect()
{
    int i;
    struct Context* c;
    globals->allocCount = 0;
    c = globals->allContexts;
    while(c) {
        for(i=0; i<NUM_NASAL_TYPES; i++)
            c->nfree[i] = 0;
        for(i=0; i < c->fTop; i++) {
            mark(c->fStack[i].func);
            mark(c->fStack[i].locals);
        }
        for(i=0; i < c->opTop; i++)
            mark(c->opStack[i]);
        mark(c->dieArg);
        mark(c->temps);
        c = c->nextAll;
    }

    mark(globals->save);
    mark(globals->symbols);
    mark(globals->meRef);
    mark(globals->argRef);
    mark(globals->parentsRef);

    // Finally collect all the freed objects
    for(i=0; i<NUM_NASAL_TYPES; i++)
        reap(&(globals->pools[i]));

    // Make enough space for the dead blocks we need to free during
    // execution.  This works out to 1 spot for every 2 live objects,
    // which should be limit the number of bottleneck operations
    // without imposing an undue burden of extra "freeable" memory.
    if(globals->deadsz < globals->allocCount) {
        globals->deadsz = globals->allocCount;
        if(globals->deadsz < 256) globals->deadsz = 256;
        naFree(globals->deadBlocks);
        globals->deadBlocks = naAlloc(sizeof(void*) * globals->deadsz);
    }
    globals->needGC = 0;
}

void naModLock()
{
    naCheckBottleneck();
    LOCK();
    globals->nThreads++;
    UNLOCK();
}

void naModUnlock()
{
    LOCK();
    globals->nThreads--;
    UNLOCK();
}

// Must be called with the main lock.  Engages the "bottleneck", where
// all threads will block so that one (the last one to call this
// function) can run alone.  This is done for GC, and also to free the
// list of "dead" blocks when it gets full (which is part of GC, if
// you think about it).
static void bottleneck()
{
    struct Globals* g = globals;
    g->bottleneck = 1;
    while(g->bottleneck && g->waitCount < g->nThreads - 1) {
        g->waitCount++;
        UNLOCK(); naSemDown(g->sem); LOCK();
        g->waitCount--;
    }
    if(g->waitCount >= g->nThreads - 1) {
        freeDead();
        if(g->needGC) garbageCollect();
        if(g->waitCount) naSemUpAll(g->sem, g->waitCount);
        g->bottleneck = 0;
    }
}

void naCheckBottleneck()
{
    if(globals->bottleneck) { LOCK(); bottleneck(); UNLOCK(); }
}

static void naCode_gcclean(struct naCode* o)
{
    naFree(o->byteCode);   o->byteCode = 0;
    naFree(o->constants);  o->constants = 0;
    naFree(o->argSyms);    o->argSyms = 0;
    naFree(o->optArgSyms); o->argSyms = 0;
}

static void naGhost_gcclean(struct naGhost* g)
{
    if(g->ptr) g->gtype->destroy(g->ptr);
    g->ptr = 0;
}

static void freeelem(struct naPool* p, struct naObj* o)
{
    // Free any intrinsic (i.e. non-garbage collected) storage the
    // object might have
    switch(p->type) {
    case T_STR:
        naStr_gcclean((struct naStr*)o);
        break;
    case T_VEC:
        naVec_gcclean((struct naVec*)o);
        break;
    case T_HASH:
        naHash_gcclean((struct naHash*)o);
        break;
    case T_CODE:
        naCode_gcclean((struct naCode*)o);
        break;
    case T_GHOST:
        naGhost_gcclean((struct naGhost*)o);
        break;
    }

    // And add it to the free list
    o->type = T_GCFREED; // DEBUG
    p->free[p->nfree++] = o;
}

static void newBlock(struct naPool* p, int need)
{
    int i;
    struct Block* newb;

    if(need < MIN_BLOCK_SIZE) need = MIN_BLOCK_SIZE;

    newb = naAlloc(sizeof(struct Block));
    newb->block = naAlloc(need * p->elemsz);
    newb->size = need;
    newb->next = p->blocks;
    p->blocks = newb;
    naBZero(newb->block, need * p->elemsz);
    
    if(need > p->freesz - p->freetop) need = p->freesz - p->freetop;
    p->nfree = 0;
    p->free = p->free0 + p->freetop;
    for(i=0; i < need; i++) {
        struct naObj* o = (struct naObj*)(newb->block + i*p->elemsz);
        o->mark = 0;
        o->type = T_GCFREED; // DEBUG
        p->free[p->nfree++] = o;
    }
    p->freetop += need;
}

void naGC_init(struct naPool* p, int type)
{
    p->type = type;
    p->elemsz = naTypeSize(type);
    p->blocks = 0;

    p->free0 = p->free = 0;
    p->nfree = p->freesz = p->freetop = 0;
    reap(p);
}

static int poolsize(struct naPool* p)
{
    int total = 0;
    struct Block* b = p->blocks;
    while(b) { total += b->size; b = b->next; }
    return total;
}

struct naObj** naGC_get(struct naPool* p, int n, int* nout)
{
    struct naObj** result;
    naCheckBottleneck();
    LOCK();
    while(globals->allocCount < 0 || (p->nfree == 0 && p->freetop >= p->freesz)) {
        globals->needGC = 1;
        bottleneck();
    }
    if(p->nfree == 0)
        newBlock(p, poolsize(p)/8);
    n = p->nfree < n ? p->nfree : n;
    *nout = n;
    p->nfree -= n;
    globals->allocCount -= n;
    result = (struct naObj**)(p->free + p->nfree);
    UNLOCK();
    return result;
}

// Sets the reference bit on the object, and recursively on all
// objects reachable from it.  Uses the processor stack for recursion...
static void mark(naRef r)
{
    int i;

    if(IS_NUM(r) || IS_NIL(r))
        return;

    if(r.ref.ptr.obj->mark == 1)
        return;

    // Verify that the object hasn't been freed incorrectly:
    if(r.ref.ptr.obj->type == T_GCFREED) *(int*)0=0; // DEBUG

    r.ref.ptr.obj->mark = 1;
    switch(r.ref.ptr.obj->type) {
    case T_VEC:
        if(r.ref.ptr.vec->rec)
            for(i=0; i<r.ref.ptr.vec->rec->size; i++)
                mark(r.ref.ptr.vec->rec->array[i]);
        break;
    case T_HASH:
        if(r.ref.ptr.hash->rec != 0) {
            struct HashRec* hr = r.ref.ptr.hash->rec;
            for(i=0; i < (1<<hr->lgalloced); i++) {
                struct HashNode* hn = hr->table[i];
                while(hn) {
                    mark(hn->key);
                    mark(hn->val);
                    hn = hn->next;
                }
            }
        }
        break;
    case T_CODE:
        mark(r.ref.ptr.code->srcFile);
        for(i=0; i<r.ref.ptr.code->nConstants; i++)
            mark(r.ref.ptr.code->constants[i]);
        break;
    case T_FUNC:
        mark(r.ref.ptr.func->code);
        mark(r.ref.ptr.func->namespace);
        mark(r.ref.ptr.func->next);
        break;
    }
}

// Collects all the unreachable objects into a free list, and
// allocates more space if needed.
static void reap(struct naPool* p)
{
    struct Block* b;
    int elem, freesz, total = poolsize(p);
    p->nfree = 0;
    freesz = total < MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : total;
    freesz = (3 * freesz / 2) + (globals->nThreads * OBJ_CACHE_SZ);
    if(p->freesz < freesz) {
        naFree(p->free0);
        p->freesz = freesz;
        p->free = p->free0 = naAlloc(sizeof(void*) * p->freesz);
    }

    for(b = p->blocks; b; b = b->next)
        for(elem=0; elem < b->size; elem++) {
            struct naObj* o = (struct naObj*)(b->block + elem * p->elemsz);
            if(o->mark == 0)
                freeelem(p, o);
            o->mark = 0;
        }

    // allocs of this type until the next collection
    globals->allocCount += total/2;
    
    // Allocate more if necessary (try to keep 25-50% of the objects
    // available)
    if(p->nfree < total/4) {
        int used = total - p->nfree;
        int avail = total - used;
        int need = used/2 - avail;
        if(need > 0)
            newBlock(p, need);
    }
    p->freetop = p->nfree;
}

// Atomically replaces target with a new pointer, and adds the old one
// to the list of blocks to free the next time something holds the
// giant lock.
void naGC_swapfree(void** target, void* val)
{
    LOCK();
    while(globals->ndead >= globals->deadsz)
        bottleneck();
    globals->deadBlocks[globals->ndead++] = *target;
    *target = val;
    UNLOCK();
}
