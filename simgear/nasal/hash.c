#include <string.h>
#include "nasal.h"
#include "data.h"

/* A HashRec lives in a single allocated block.  The layout is the
 * header struct, then a table of 2^lgsz hash entries (key/value
 * pairs), then an index table of 2*2^lgsz integers storing index
 * values into the entry table.  There are two tokens needed for
 * "unused" and "used but empty". */

#define ENT_EMPTY -1
#define ENT_DELETED -2

typedef struct { naRef key, val; } HashEnt;

typedef struct HashRec {
    int size; /* number of active entries */
    int lgsz; /* base-2 logarithm of the allocated (!) size */
    int next; /* next entry to use */
} HashRec;

#define REC(h) (PTR(h).hash->rec)
#define POW2(n) (1<<(n))
#define NCELLS(hr) (2*POW2((hr)->lgsz))
#define ROUNDUPOFF(n,m) ((((n)+(m-1))/m)*m)-(n)
#define ALIGN(p,sz) (((char*)p)+ROUNDUPOFF(((size_t)p)%sz,sz))
#define ENTS(h) ((HashEnt*)ALIGN(&((HashRec*)h)[1],sizeof(naRef)))
#define TAB(h) ((int*)&(ENTS(h)[1<<(h)->lgsz]))
#define HBITS(hr,code) ((hr)->lgsz ? ((code)>>(32-(hr)->lgsz)) : 0)

#define LROT(h,n) (((h)<<n)|((h)>>((8*sizeof(h))-n)))
static unsigned int mix32(unsigned int h)
{
    h ^= 0x2e63823a;  h += LROT(h, 15); h -= LROT(h, 9);
    h += LROT(h, 4);  h -= LROT(h, 1);  h ^= LROT(h, 2);
    return h;
}
static unsigned int hash32(const unsigned char* in, int len)
{
    unsigned int h = len, val = 0;
    int i, count = 0;
    for(i=0; i<len; i++) {
        val = (val<<8) ^ in[i];
        if(++count == 4) {
            h = mix32(h ^ val);
            val = count = 0;
        }
    }
    return mix32(h ^ val);
}

static unsigned int refhash(naRef key)
{
    if(IS_STR(key)) {
        struct naStr* s = PTR(key).str;
        if(s->hashcode) return s->hashcode;
        return s->hashcode = hash32((void*)naStr_data(key), naStr_len(key));
    } else { /* must be a number */
        union { double d; unsigned int u[2]; } n;
        n.d = key.num == -0.0 ? 0.0 : key.num; /* remember negative zero! */ 
        return mix32(mix32(n.u[0]) ^ n.u[1]);
    }
}

static int equal(naRef a, naRef b)
{
    if(IS_NUM(a)) return a.num == b.num;
    if(PTR(a).obj == PTR(b).obj) return 1;
    if(naStr_len(a) != naStr_len(b)) return 0;
    return memcmp(naStr_data(a), naStr_data(b), naStr_len(a)) == 0;
}

/* Returns the index of a cell that either contains a matching key, or
 * is the empty slot to receive a new insertion. */
static int findcell(struct HashRec *hr, naRef key, unsigned int hash)
{
    int i, mask = POW2(hr->lgsz+1)-1, step = (2*hash+1) & mask;
    for(i=HBITS(hr,hash); TAB(hr)[i] != ENT_EMPTY; i=(i+step)&mask)
        if(TAB(hr)[i] != ENT_DELETED && equal(key, ENTS(hr)[TAB(hr)[i]].key))
            break;
    return i;
}

static void hashset(HashRec* hr, naRef key, naRef val)
{
    int ent, cell = findcell(hr, key, refhash(key));
    if((ent = TAB(hr)[cell]) == ENT_EMPTY) {
        ent = hr->next++;
        if(ent >= NCELLS(hr)) return; /* race protection, don't overrun */
        TAB(hr)[cell] = ent;
        hr->size++;
        ENTS(hr)[ent].key = key;
    }
    ENTS(hr)[ent].val = val;
}

static int recsize(int lgsz)
{
    HashRec hr;
    hr.lgsz = lgsz;
    return (int)((char*)&TAB(&hr)[POW2(lgsz+1)] - (char*)&hr) + sizeof(naRef);
}

static HashRec* resize(struct naHash* hash)
{
    HashRec *hr = hash->rec, *hr2;
    int i, lgsz = 0;
    if(hr) {
        int oldsz = hr->size;
        while(oldsz) { oldsz >>= 1; lgsz++; }
    }

    // REVIEW: Memory Leak - 196,628 bytes in 1 blocks are still reachable
    // Since method returns a HashRec*, assuming caller is responsible for freeing the memory
    // trace: codegen::naInternSymbol() > naHash_set() > size()
    // seems to be a known issue - refer to comment @ codegen::naInternSymbol()
    hr2 = naAlloc(recsize(lgsz));
    hr2->size = hr2->next = 0;
    hr2->lgsz = lgsz;
    for(i=0; i<(2*(1<<lgsz)); i++)
        TAB(hr2)[i] = ENT_EMPTY;
    for(i=0; hr && i < POW2(hr->lgsz+1); i++)
        if(TAB(hr)[i] >= 0)
            hashset(hr2, ENTS(hr)[TAB(hr)[i]].key, ENTS(hr)[TAB(hr)[i]].val);
    naGC_swapfree((void*)&hash->rec, hr2);
    return hr2;
}

int naHash_size(naRef h) { return REC(h) ? REC(h)->size : 0; }

int naHash_get(naRef hash, naRef key, naRef* out)
{
    HashRec* hr = REC(hash);
    if(hr) {
        int ent, cell = findcell(hr, key, refhash(key));
        if((ent = TAB(hr)[cell]) < 0) return 0;
        *out = ENTS(hr)[ent].val;
        return 1;
    }
    return 0;
}

void naHash_set(naRef hash, naRef key, naRef val)
{
    HashRec* hr = REC(hash);
    if(!hr || hr->next >= POW2(hr->lgsz))
        hr = resize(PTR(hash).hash);
    hashset(hr, key, val);
}

void naHash_delete(naRef hash, naRef key)
{
    HashRec* hr = REC(hash);
    if(hr) {
        int cell = findcell(hr, key, refhash(key));
        if(TAB(hr)[cell] >= 0) {
            TAB(hr)[cell] = ENT_DELETED;
            if(--hr->size < POW2(hr->lgsz-1))
                resize(PTR(hash).hash);
        }
    }
}

void naHash_keys(naRef dst, naRef hash)
{
    int i;
    HashRec* hr = REC(hash);
    for(i=0; hr && i < NCELLS(hr); i++)
        if(TAB(hr)[i] >= 0)
            naVec_append(dst, ENTS(hr)[TAB(hr)[i]].key);
}

void naiGCMarkHash(naRef hash)
{
    int i;
    HashRec* hr = REC(hash);
    for(i=0; hr && i < NCELLS(hr); i++)
        if(TAB(hr)[i] >= 0) {
            naiGCMark(ENTS(hr)[TAB(hr)[i]].key);
            naiGCMark(ENTS(hr)[TAB(hr)[i]].val);
        }
}

static void tmpStr(naRef* out, struct naStr* str, const char* key)
{
    str->type = T_STR;
    str->hashcode = 0;
    str->emblen = -1;
    str->data.ref.ptr = (unsigned char*)key;
    str->data.ref.len = strlen(key);
    SETPTR(*out, str);
}

int naMember_cget(naContext c, naRef obj, const char* field, naRef* out)
{
    naRef key; struct naStr str;
    tmpStr(&key, &str, field);
    return naMember_get(c, obj, key, out);
}

naRef naHash_cget(naRef hash, char* key)
{
    struct naStr str;
    naRef result, key2;
    tmpStr(&key2, &str, key);
    return naHash_get(hash, key2, &result) ? result : naNil();
}

void naHash_cset(naRef hash, char* key, naRef val)
{
    naRef key2; struct naStr str;
    tmpStr(&key2, &str, key);
    naiHash_tryset(hash, key2, val);
}

int naiHash_tryset(naRef hash, naRef key, naRef val)
{
    HashRec* hr = REC(hash);
    if(hr) {
        int ent, cell = findcell(hr, key, refhash(key));
        if((ent = TAB(hr)[cell]) >= 0) { ENTS(hr)[ent].val = val; return 1; }
    }
    return 0;
}

void naiGCHashClean(struct naHash* h)
{
    naFree(h->rec);
    h->rec = 0;
}

/* Optimized naHash_get for looking up local variables (OP_LOCAL is by
 * far the most common opcode and deserves some special case
 * optimization).  Assumes that the key is an interned symbol
 * (i.e. the hash code is precomputed, and we only need to test for
 * pointer identity). */
int naiHash_sym(struct naHash* hash, struct naStr* sym, naRef* out)
{
    HashRec* hr = hash->rec;
    if(hr) {
        int* tab = TAB(hr);
        HashEnt* ents = ENTS(hr);
        unsigned int hc = sym->hashcode;
        int cell, mask = POW2(hr->lgsz+1) - 1, step = (2*hc+1) & mask;
        for(cell=HBITS(hr,hc); tab[cell] != ENT_EMPTY; cell=(cell+step)&mask)
            if(tab[cell]!=ENT_DELETED && sym==PTR(ents[tab[cell]].key).str) {
                *out = ents[tab[cell]].val;
                return 1;
            }
    }
    return 0;
}


/* As above, a special naHash_set for setting local variables.
 * Assumes that the key is interned, and also that it isn't already
 * present in the hash. */
void naiHash_newsym(struct naHash* hash, naRef* sym, naRef* val)
{
    HashRec* hr = hash->rec;
    int mask, step, cell, ent;
    struct naStr *s = PTR(*sym).str;
    if(!hr || hr->next >= POW2(hr->lgsz))
        hr = resize(hash);
    mask = POW2(hr->lgsz+1) - 1;
    step = (2*s->hashcode+1) & mask;
    cell = HBITS(hr, s->hashcode);
    while(TAB(hr)[cell] != ENT_EMPTY)
        cell = (cell + step) & mask;
    ent = hr->next++;
    if(ent >= NCELLS(hr)) return; /* race protection, don't overrun */
    TAB(hr)[cell] = ent;
    hr->size++;
    ENTS(hr)[TAB(hr)[cell]].key = *sym;
    ENTS(hr)[TAB(hr)[cell]].val = *val;
}

