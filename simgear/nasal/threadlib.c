#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "data.h"
#include "code.h"

static void lockDestroy(void* lock) { naFreeLock(lock); }
static naGhostType LockType = { lockDestroy, NULL, NULL, NULL };

static void semDestroy(void* sem) { naFreeSem(sem); }
static naGhostType SemType = { semDestroy, NULL, NULL, NULL };

typedef struct {
    naContext ctx;
    naRef func;
} ThreadData;

#ifdef _WIN32
static DWORD WINAPI threadtop(LPVOID param)
#else
static void* threadtop(void* param)
#endif
{
    ThreadData* td = param;
    naCall(td->ctx, td->func, 0, 0, naNil(), naNil());
    naFreeContext(td->ctx);
    naFree(td);
    return 0;
}

static naRef f_newthread(naContext c, naRef me, int argc, naRef* args)
{
    ThreadData *td;
    if(argc < 1 || !naIsFunc(args[0]))
        naRuntimeError(c, "bad/missing argument to newthread");
    td = naAlloc(sizeof(*td));
    td->ctx = naNewContext();
    td->func = args[0];
    naTempSave(td->ctx, td->func);
#ifdef _WIN32
    CreateThread(0, 0, threadtop, td, 0, 0);
#else
    {
        pthread_t t; int err;
        if((err = pthread_create(&t, 0, threadtop, td)))
            naRuntimeError(c, "newthread failed: %s", strerror(err));
        pthread_detach(t);
    }
#endif
    return naNil();
}

static naRef f_newlock(naContext c, naRef me, int argc, naRef* args)
{
    return naNewGhost(c, &LockType, naNewLock());
}

static naRef f_lock(naContext c, naRef me, int argc, naRef* args)
{
    if(argc > 0 && naGhost_type(args[0]) == &LockType) {
        naModUnlock();
        naLock(naGhost_ptr(args[0]));
        naModLock();
    }
    return naNil();
}

static naRef f_unlock(naContext c, naRef me, int argc, naRef* args)
{
    if(argc > 0 && naGhost_type(args[0]) == &LockType)
        naUnlock(naGhost_ptr(args[0]));
    return naNil();
}

static naRef f_newsem(naContext c, naRef me, int argc, naRef* args)
{
    return naNewGhost(c, &SemType, naNewSem());
}

static naRef f_semdown(naContext c, naRef me, int argc, naRef* args)
{
    if(argc > 0 && naGhost_type(args[0]) == &SemType) {
        naModUnlock();
        naSemDown(naGhost_ptr(args[0]));
        naModLock();
    }
    return naNil();
}

static naRef f_semup(naContext c, naRef me, int argc, naRef* args)
{
    if(argc > 0 && naGhost_type(args[0]) == &SemType)
        naSemUp(naGhost_ptr(args[0]), 1);
    return naNil();
}

static naCFuncItem funcs[] = {
    { "newthread", f_newthread },
    { "newlock", f_newlock },
    { "lock", f_lock },
    { "unlock", f_unlock },
    { "newsem", f_newsem },
    { "semdown", f_semdown },
    { "semup", f_semup },
    { 0 }
};

naRef naInit_thread(naContext c)
{
    return naGenLib(c, funcs);
}
