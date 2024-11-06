// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2003  Andy Ross  andy@plausible.org

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>

#include "data.h"
#include "iolib.h"

#define IOFILE(r) ((FILE*)IOGHOST(r)->handle)
#define NEWSTR(c, s, l) naStr_fromdata(naNewString(c), s, l)
#define NEWCSTR(c, s) NEWSTR(c, s, strlen(s))

static naRef f_pipe(naContext ctx, naRef me, int argc, naRef* args)
{
    int pip[2];
    naRef result = naNewVector(ctx);
    if(pipe(pip) != 0) naRuntimeError(ctx, strerror(errno));
    naVec_append(result, naIOGhost(ctx, fdopen(pip[0], "r")));
    naVec_append(result, naIOGhost(ctx, fdopen(pip[1], "w")));
    return result;
}

static naRef f_fork(naContext ctx, naRef me, int argc, naRef* args)
{
    double pid = fork();
    if(pid < 0) naRuntimeError(ctx, strerror(errno));
    return naNum(pid);
}

static naRef f_dup2(naContext ctx, naRef me, int argc, naRef* args)
{
    if(argc != 2 || !IS_STDIO(args[0]) || !IS_STDIO(args[1]))
        naRuntimeError(ctx, "bad argument to dup2");
    if(dup2(fileno(IOFILE(args[0])), fileno(IOFILE(args[1]))) < 0)
        naRuntimeError(ctx, strerror(errno));
    return args[1];
}

static naRef f_waitpid(naContext ctx, naRef me, int argc, naRef* args)
{
    int child, status;
    naRef result, pid = argc > 0 ? naNumValue(args[0]) : naNil();
    naRef opts = argc > 1 ? naNumValue(args[1]) : naNum(0);
    if(!IS_NUM(pid) || !IS_NUM(opts))
        naRuntimeError(ctx, "bad argument to waitpid");
    child = waitpid((pid_t)pid.num, &status, opts.num == 0 ? 0 : WNOHANG);
    if(child < 0) naRuntimeError(ctx, strerror(errno));
    result = naNewVector(ctx);
    naVec_append(result, naNum(child));
    naVec_append(result, naNum(status));
    return result;
}

// Note loop to make sure that the gettimeofday call doesn't happen
// across an integer seconds boundary.
static naRef f_time(naContext ctx, naRef me, int argc, naRef* args)
{
    time_t t;
    struct timeval tod;
    do { t = time(0); gettimeofday(&tod, 0); } while(t != time(0));
    return naNum(t + tod.tv_usec * (1.0/1000000.0));
}

static naRef f_chdir(naContext ctx, naRef me, int argc, naRef* args)
{
    if(argc < 1 || !IS_STR(args[0]))
        naRuntimeError(ctx, "bad argument to chdir");
    if(chdir(naStr_data(args[0])) < 0)
        naRuntimeError(ctx, strerror(errno));
    return naNil();
}

static void dirGhostDestroy(void* g)
{
    DIR** d = (DIR**)g;
    if(*d) closedir(*d);
    naFree(g);
}

static naGhostType DirGhostType = { dirGhostDestroy, "dir" };

static naRef f_opendir(naContext ctx, naRef me, int argc, naRef* args)
{
    naRef result;
    DIR* d;
    if(argc < 1 || !IS_STR(args[0]))
        naRuntimeError(ctx, "bad argument to opendir");
    if(!(d = opendir(naStr_data(args[0]))))
        naRuntimeError(ctx, strerror(errno));
    result = naNewGhost(ctx, &DirGhostType, naAlloc(sizeof(DIR*)));
    *(DIR**)PTR(result).ghost->ptr = d;
    return result;
}

static naRef f_readdir(naContext ctx, naRef me, int argc, naRef* args)
{
    struct dirent* dent;
    if(argc < 1 || !IS_GHOST(args[0]) || naGhost_type(args[0]) != &DirGhostType)
        naRuntimeError(ctx, "bad argument to readdir");
    dent = readdir(*(DIR**)naGhost_ptr(args[0]));
    return dent ? NEWCSTR(ctx, dent->d_name) : naNil();
}

static naRef f_closedir(naContext ctx, naRef me, int argc, naRef* args)
{
    if(argc < 1 || !IS_GHOST(args[0]) || naGhost_type(args[0]) != &DirGhostType)
        naRuntimeError(ctx, "bad argument to closedir");
    closedir(*(DIR**)naGhost_ptr(args[0]));
    *(DIR**)naGhost_ptr(args[0]) = 0;
    return naNil();
}

static char** strv(naContext c, naRef v)
{
    int i, n = naVec_size(v);
    char** sv = naAlloc(sizeof(char*) * (n+1));
    for(i=0; i<n; i++) {
        naRef s = naVec_get(v, i);
        if(!IS_STR(s)) naRuntimeError(c, "non-string passed to exec() vector");
        sv[i] = naStr_data(s);
    }
    sv[n] = 0;
    return sv;
}

static naRef f_exec(naContext ctx, naRef me, int argc, naRef* args)
{
    char **argv, **envp;
    naRef file = argc > 0 ? naStringValue(ctx, args[0]) : naNil();
    naRef av = argc > 1 ? args[1] : naNil();
    naRef ev = argc > 2 ? args[2] : naNil();
    if(!IS_STR(file) || !IS_VEC(av) || !IS_VEC(ev))
        naRuntimeError(ctx, "bad argument to exec");
    argv = strv(ctx, av); envp = strv(ctx, ev);
    execve(naStr_data(file), argv, envp);
    naFree(argv); naFree(envp);
    naRuntimeError(ctx, strerror(errno));
    return file; // never reached
}

static naRef f_getcwd(naContext ctx, naRef me, int argc, naRef* args)
{
    char buf[1024];
    if(!getcwd(buf, sizeof(buf))) naRuntimeError(ctx, strerror(errno));
    return NEWCSTR(ctx, buf);
}

extern char** environ;
static naRef f_environ(naContext ctx, naRef me, int argc, naRef* args)
{
    naRef result = naNewVector(ctx);
    char** p;
    for(p = environ; *p; p++)
        naVec_append(result, NEWCSTR(ctx, *p));
    return result;
}

static naRef f_sleep(naContext ctx, naRef me, int argc, naRef* args)
{
    double secs = argc > 0 ? naNumValue(args[0]).num : 0;
    naModUnlock();
    usleep((useconds_t)(secs * 1e6));
    naModLock();
    return naNil();
}

static naCFuncItem funcs[] = {
    { "pipe", f_pipe },
    { "fork", f_fork },
    { "dup2", f_dup2 },
    { "exec", f_exec },
    { "waitpid", f_waitpid },
    { "opendir", f_opendir },
    { "readdir", f_readdir },
    { "closedir", f_closedir },
    { "time", f_time },
    { "chdir", f_chdir },
    { "getcwd", f_getcwd },
    { "environ", f_environ },
    { "sleep", f_sleep },
    { 0 }
};

naRef naInit_unix(naContext c)
{
    return naGenLib(c, funcs);
}
