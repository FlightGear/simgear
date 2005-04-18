#include "nasal.h"
#include "data.h"

static void realloc(struct naVec* v)
{
    struct VecRec* old = v->rec;
    int i, oldsz = old ? old->size : 0, newsz = 1 + ((oldsz*3)>>1);
    struct VecRec* vr = naAlloc(sizeof(struct VecRec) + sizeof(naRef) * newsz);
    if(oldsz > newsz) oldsz = newsz; // race protection
    vr->alloced = newsz;
    vr->size = oldsz;
    for(i=0; i<oldsz; i++)
        vr->array[i] = old->array[i];
    naGC_swapfree((void**)&(v->rec), vr);
}

void naVec_gcclean(struct naVec* v)
{
    naFree(v->rec);
    v->rec = 0;
}

naRef naVec_get(naRef v, int i)
{
    if(IS_VEC(v)) {
        struct VecRec* r = v.ref.ptr.vec->rec;
        if(r && i < r->size) return r->array[i];
    }
    return naNil();
}

void naVec_set(naRef vec, int i, naRef o)
{
    if(IS_VEC(vec)) {
        struct VecRec* r = vec.ref.ptr.vec->rec;
        if(r && i >= r->size) return;
        r->array[i] = o;
    }
}

int naVec_size(naRef v)
{
    if(IS_VEC(v)) {
        struct VecRec* r = v.ref.ptr.vec->rec;
        return r ? r->size : 0;
    }
    return 0;
}

int naVec_append(naRef vec, naRef o)
{
    if(IS_VEC(vec)) {
        struct VecRec* r = vec.ref.ptr.vec->rec;
        if(!r || r->size >= r->alloced) {
            realloc(vec.ref.ptr.vec);
            r = vec.ref.ptr.vec->rec;
        }
        r->array[r->size] = o;
        return r->size++;
    }
    return 0;
}

void naVec_setsize(naRef vec, int sz)
{
    int i;
    struct VecRec* v = vec.ref.ptr.vec->rec;
    struct VecRec* nv = naAlloc(sizeof(struct VecRec) + sizeof(naRef) * sz);
    nv->size = sz;
    nv->alloced = sz;
    for(i=0; i<sz; i++)
        nv->array[i] = (v && i < v->size) ? v->array[i] : naNil();
    naFree(v);
    vec.ref.ptr.vec->rec = nv;
}

naRef naVec_removelast(naRef vec)
{
    naRef o;
    if(IS_VEC(vec)) {
        struct VecRec* v = vec.ref.ptr.vec->rec;
        if(!v || v->size == 0) return naNil();
        o = v->array[v->size - 1];
        v->size--;
        if(v->size < (v->alloced >> 1))
            realloc(vec.ref.ptr.vec);
        return o;
    }
    return naNil();
}
