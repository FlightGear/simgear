#include "nasal.h"
#include "data.h"

#define MIN_BLOCK_SIZE 256

// "type" for an object freed by the collector
#define T_GCFREED 123 // DEBUG

struct Block {
    int   size;
    char* block;
};

// Decremented every allocation.  When it reaches zero, we do a
// garbage collection.  The value is reset to 1/2 of the total object
// count each collection, which is sane: it ensures that no more than
// 50% growth can happen between collections, and ensures that garbage
// collection work is constant with allocation work (i.e. that O(N)
// work is done only every O(1/2N) allocations).
static int GlobalAllocCount = 256;

static void appendfree(struct naPool*p, struct naObj* o)
{
    // Need more space?
    if(p->freesz <= p->nfree) {
        int i, n = 1+((3*p->nfree)>>1);
        void** newf = naAlloc(n * sizeof(void*));
        for(i=0; i<p->nfree; i++)
            newf[i] = p->free[i];
        naFree(p->free);
        p->free = newf;
        p->freesz = n;
    }

    p->free[p->nfree++] = o;
}

static void naCode_gcclean(struct naCode* o)
{
    naFree(o->byteCode);  o->byteCode = 0;
    naFree(o->constants); o->constants = 0;
}

static void naGhost_gcclean(struct naGhost* g)
{
    if(g->ptr) g->gtype->destroy(g->ptr);
    g->ptr = 0;
}

static void freeelem(struct naPool* p, struct naObj* o)
{
    // Mark the object as "freed" for debugging purposes
    o->type = T_GCFREED; // DEBUG

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
    appendfree(p, o);
}

static void newBlock(struct naPool* p, int need)
{
    int i;
    char* buf;
    struct Block* newblocks;

    if(need < MIN_BLOCK_SIZE)
        need = MIN_BLOCK_SIZE;
    
    newblocks = naAlloc((p->nblocks+1) * sizeof(struct Block));
    for(i=0; i<p->nblocks; i++) newblocks[i] = p->blocks[i];
    naFree(p->blocks);
    p->blocks = newblocks;
    buf = naAlloc(need * p->elemsz);
    naBZero(buf, need * p->elemsz);
    p->blocks[p->nblocks].size = need;
    p->blocks[p->nblocks].block = buf;
    p->nblocks++;
    
    for(i=0; i<need; i++) {
        struct naObj* o = (struct naObj*)(buf + i*p->elemsz);
        o->mark = 0;
        o->type = p->type;
        appendfree(p, o);
    }
}

void naGC_init(struct naPool* p, int type)
{
    p->type = type;
    p->elemsz = naTypeSize(type);
    p->nblocks = 0;
    p->blocks = 0;
    p->nfree = 0;
    p->freesz = 0;
    p->free = 0;
    naGC_reap(p);
}

int naGC_size(struct naPool* p)
{
    int i, total=0;
    for(i=0; i<p->nblocks; i++)
        total += ((struct Block*)(p->blocks + i))->size;
    return total;
}

struct naObj* naGC_get(struct naPool* p)
{
    // Collect every GlobalAllocCount allocations.
    // This gets set to ~50% of the total object count each
    // collection (it's incremented in naGC_reap()).
    if(--GlobalAllocCount < 0) {
        GlobalAllocCount = 0;
        naGarbageCollect();
    }

    // If we're out, then allocate an extra 12.5%
    if(p->nfree == 0)
        newBlock(p, naGC_size(p)/8);
    return p->free[--p->nfree];
}

// Sets the reference bit on the object, and recursively on all
// objects reachable from it.  Clumsy: uses C stack recursion, which
// is slower than it need be and may cause problems on some platforms
// due to the very large stack depths that result.
void naGC_mark(naRef r)
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
        for(i=0; i<r.ref.ptr.vec->size; i++)
            naGC_mark(r.ref.ptr.vec->array[i]);
        break;
    case T_HASH:
        if(r.ref.ptr.hash->table == 0)
            break;
        for(i=0; i < (1<<r.ref.ptr.hash->lgalloced); i++) {
            struct HashNode* hn = r.ref.ptr.hash->table[i];
            while(hn) {
                naGC_mark(hn->key);
                naGC_mark(hn->val);
                hn = hn->next;
            }
        }
        break;
    case T_CODE:
        naGC_mark(r.ref.ptr.code->srcFile);
        for(i=0; i<r.ref.ptr.code->nConstants; i++)
            naGC_mark(r.ref.ptr.code->constants[i]);
        break;
    case T_CLOSURE:
        naGC_mark(r.ref.ptr.closure->namespace);
        naGC_mark(r.ref.ptr.closure->next);
        break;
    case T_FUNC:
        naGC_mark(r.ref.ptr.func->code);
        naGC_mark(r.ref.ptr.func->closure);
        break;
    }
}

// Collects all the unreachable objects into a free list, and
// allocates more space if needed.
void naGC_reap(struct naPool* p)
{
    int i, elem, total = 0;
    p->nfree = 0;
    for(i=0; i<p->nblocks; i++) {
        struct Block* b = p->blocks + i;
        total += b->size;
        for(elem=0; elem < b->size; elem++) {
            struct naObj* o = (struct naObj*)(b->block + elem * p->elemsz);
            if(o->mark == 0)
                freeelem(p, o);
            o->mark = 0;
        }
    }

    // Add 50% of our total to the global count
    GlobalAllocCount += total/2;
    
    // Allocate more if necessary (try to keep 25-50% of the objects
    // available)
    if(p->nfree < total/4) {
        int used = total - p->nfree;
        int avail = total - used;
        int need = used/2 - avail;
        if(need > 0)
            newBlock(p, need);
    }
}

