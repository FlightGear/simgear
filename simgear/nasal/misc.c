#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "nasal.h"
#include "code.h"

void naFree(void* m) { free(m); }
void* naAlloc(int n) { return malloc(n); }
void* naRealloc(void* b, int n) { return realloc(b, n); }
void naBZero(void* m, int n) { memset(m, 0, n); }

void naTempSave(naContext c, naRef r)
{
    int i;
    if(!IS_OBJ(r)) return;
    if(c->ntemps >= c->tempsz) {
        struct naObj** newtemps;
        c->tempsz *= 2;
        // REVIEW: Memory Leak - 1,024 bytes in 1 blocks are still reachable
        newtemps = naAlloc(c->tempsz * sizeof(struct naObj*));
        for(i=0; i<c->ntemps; i++)
            newtemps[i] = c->temps[i];
        naFree(c->temps);
        c->temps = newtemps;
    }
    c->temps[c->ntemps++] = PTR(r).obj;
}

naRef naObj(int type, struct naObj* o)
{
    naRef r;
    SETPTR(r, o);
    o->type = type;
    return r;
}

int naTrue(naRef r)
{
    if(IS_NIL(r)) return 0;
    if(IS_NUM(r)) return r.num != 0;
    if(IS_STR(r)) return 1;
    return 0;
}

naRef naNumValue(naRef n)
{
    double d;
    if(IS_NUM(n)) return n;
    if(IS_NIL(n)) return naNil();
    if(IS_STR(n) && naStr_tonum(n, &d))
        return naNum(d);
    return naNil();
}

naRef naStringValue(naContext c, naRef r)
{
    if(IS_NIL(r) || IS_STR(r)) return r;
    if(IS_NUM(r)) {
        naRef s = naNewString(c);
        naStr_fromnum(s, r.num);
        return s;
    }
    return naNil();
}

naRef naNew(struct Context* c, int type)
{
    naRef result;
    if(c->nfree[type] == 0)
        c->free[type] = naGC_get(&globals->pools[type],
                                 OBJ_CACHE_SZ, &c->nfree[type]);
    result = naObj(type, c->free[type][--c->nfree[type]]);
    naTempSave(c, result);
    return result;
}

naRef naNewString(struct Context* c)
{
    naRef s = naNew(c, T_STR);
    PTR(s).str->emblen = 0;
    PTR(s).str->data.ref.len = 0;
    PTR(s).str->data.ref.ptr = 0;
    PTR(s).str->hashcode = 0;
    return s;
}

naRef naNewVector(struct Context* c)
{
    naRef r = naNew(c, T_VEC);
    PTR(r).vec->rec = 0;
    return r;
}

naRef naNewHash(struct Context* c)
{
    naRef r = naNew(c, T_HASH);
    PTR(r).hash->rec = 0;
    return r;
}

naRef naNewCode(struct Context* c)
{
    naRef r = naNew(c, T_CODE);
    // naNew can return a previously used naCode. naCodeGen will init
    // all these members but a GC can occur inside naCodeGen, so we see
    // partially initalized state here. To avoid this, clear out the values
    // which mark() cares about.
    PTR(r).code->srcFile = naNil();
    PTR(r).code->nConstants = 0;
    return r;
}

naRef naNewCCode(struct Context* c, naCFunction fptr)
{
    naRef r = naNew(c, T_CCODE);
    PTR(r).ccode->fptr = fptr;
    PTR(r).ccode->fptru = 0;
    return r;
}

naRef naNewCCodeU(struct Context* c, naCFunctionU fptr, void* user_data)
{
    return naNewCCodeUD(c, fptr, user_data, 0);
}

naRef naNewCCodeUD( struct Context* c,
                    naCFunctionU fptr,
                    void* user_data,
                    void (*destroy)(void*) )
{
    naRef r = naNew(c, T_CCODE);
    PTR(r).ccode->fptru = fptr;
    PTR(r).ccode->user_data = user_data;
    PTR(r).ccode->destroy = destroy;
    return r;
}

naRef naNewFunc(struct Context* c, naRef code)
{
    naRef func = naNew(c, T_FUNC);
    PTR(func).func->code = code;
    PTR(func).func->namespace = naNil();
    PTR(func).func->next = naNil();
    return func;
}

naRef naNewGhost(naContext c, naGhostType* type, void* ptr)
{
    naRef ghost;
    // ensure 'simple' ghost users don't see garbage for these fields
    type->get_member = 0;
    type->set_member = 0;

    ghost = naNew(c, T_GHOST);
    PTR(ghost).ghost->gtype = type;
    PTR(ghost).ghost->ptr = ptr;
    PTR(ghost).ghost->data = naNil();
    return ghost;
}

naRef naNewGhost2(naContext c, naGhostType* t, void* ptr)
{
    naRef ghost = naNew(c, T_GHOST);
    PTR(ghost).ghost->gtype = t;
    PTR(ghost).ghost->ptr = ptr;
    PTR(ghost).ghost->data = naNil();
    return ghost;
}

naGhostType* naGhost_type(naRef ghost)
{
    if(!IS_GHOST(ghost)) return 0;
    return PTR(ghost).ghost->gtype;
}

void* naGhost_ptr(naRef ghost)
{
    if(!IS_GHOST(ghost)) return 0;
    return PTR(ghost).ghost->ptr;
}

void naGhost_setData(naRef ghost, naRef data)
{
    if(IS_GHOST(ghost))
        PTR(ghost).ghost->data = data;
}

naRef naGhost_data(naRef ghost)
{
    if(!IS_GHOST(ghost)) return naNil();
    return PTR(ghost).ghost->data;
}

naRef naNil()
{
    naRef r;
    SETPTR(r, 0);
    return r;
}

naRef naNum(double num)
{
    naRef r;
    SETNUM(r, num);
    return r;
}

int naEqual(naRef a, naRef b)
{
    double na=0, nb=0;
    if(IS_REF(a) && IS_REF(b) && PTR(a).obj == PTR(b).obj)
        return 1; // Object identity (and nil == nil)
    if(IS_NIL(a) || IS_NIL(b))
        return 0;
    if(IS_NUM(a) && IS_NUM(b) && a.num == b.num)
        return 1; // Numeric equality
    if(IS_STR(a) && IS_STR(b) && naStr_equal(a, b))
        return 1; // String equality

    // Numeric equality after conversion
    if(IS_NUM(a)) { na = a.num; }
    else if(!(IS_STR(a) && naStr_tonum(a, &na))) { return 0; }

    if(IS_NUM(b)) { nb = b.num; }
    else if(!(IS_STR(b) && naStr_tonum(b, &nb))) { return 0; }

    return na == nb ? 1 : 0;
}

int naStrEqual(naRef a, naRef b)
{
    int i;
    char *ap, *bp;
    if(!IS_STR(a) || !IS_STR(b) || naStr_len(a) != naStr_len(b))
        return 0;
    ap = naStr_data(a);
    bp = naStr_data(b);
    for(i=0; i<naStr_len(a); i++)
        if(ap[i] != bp[i])
            return 0;
    return 1;
}

int naTypeSize(int type)
{
    switch(type) {
    case T_STR: return sizeof(struct naStr);
    case T_VEC: return sizeof(struct naVec);
    case T_HASH: return sizeof(struct naHash);
    case T_CODE: return sizeof(struct naCode);
    case T_FUNC: return sizeof(struct naFunc);
    case T_CCODE: return sizeof(struct naCCode);
    case T_GHOST: return sizeof(struct naGhost);
    };
    return 0x7fffffff; // Make sure the answer is nonsense :)
}

int naIsNil(naRef r)    { return IS_NIL(r); }
int naIsNum(naRef r)    { return IS_NUM(r); }
int naIsString(naRef r) { return IS_STR(r); }
int naIsScalar(naRef r) { return IS_SCALAR(r); }
int naIsVector(naRef r) { return IS_VEC(r); }
int naIsHash(naRef r)   { return IS_HASH(r); }
int naIsFunc(naRef r)   { return IS_FUNC(r); }
int naIsCode(naRef r)   { return IS_CODE(r); }
int naIsCCode(naRef r)  { return IS_CCODE(r); }
int naIsGhost(naRef r)  { return IS_GHOST(r); }
int naIsIdentical(naRef l, naRef r) { return IDENTICAL(l, r); }

void naSetUserData(naContext c, void* p) { c->userData = p; }
void* naGetUserData(naContext c)
{
    if(c->userData) return c->userData;
    return c->callParent ? naGetUserData(c->callParent) : 0;
}

void naAddSym(naContext c, naRef ns, char *sym, naRef val)
{
    naRef name = naStr_fromdata(naNewString(c), sym, strlen(sym));
    naHash_set(ns, naInternSymbol(name), val);
}

naRef naGenLib(naContext c, naCFuncItem *fns)
{
    naRef ns = naNewHash(c);
    for(/**/; fns->name; fns++)
        naAddSym(c, ns, fns->name, naNewFunc(c, naNewCCode(c, fns->func)));
    return ns;
}
