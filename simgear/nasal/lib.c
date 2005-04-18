#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef _MSC_VER // sigh...
#define vsnprintf _vsnprintf
#endif

#include "nasal.h"
#include "code.h"

#define NEWSTR(c, s, l) naStr_fromdata(naNewString(c), s, l)

static naRef size(naContext c, naRef me, int argc, naRef* args)
{
    if(argc == 0) return naNil();
    if(naIsString(args[0])) return naNum(naStr_len(args[0]));
    if(naIsVector(args[0])) return naNum(naVec_size(args[0]));
    if(naIsHash(args[0])) return naNum(naHash_size(args[0]));
    naRuntimeError(c, "object has no size()");
    return naNil();
}

static naRef keys(naContext c, naRef me, int argc, naRef* args)
{
    naRef v, h = args[0];
    if(!naIsHash(h)) return naNil();
    v = naNewVector(c);
    naHash_keys(v, h);
    return v;
}

static naRef append(naContext c, naRef me, int argc, naRef* args)
{
    int i;
    if(argc < 2) return naNil();
    if(!naIsVector(args[0])) return naNil();
    for(i=1; i<argc; i++) naVec_append(args[0], args[i]);
    return args[0];
}

static naRef pop(naContext c, naRef me, int argc, naRef* args)
{
    if(argc < 1 || !naIsVector(args[0])) return naNil();
    return naVec_removelast(args[0]);
}

static naRef setsize(naContext c, naRef me, int argc, naRef* args)
{
    if(argc < 2) return naNil();
    int sz = (int)naNumValue(args[1]).num;
    if(!naIsVector(args[0])) return naNil();
    naVec_setsize(args[0], sz);
    return args[0];
}

static naRef subvec(naContext c, naRef me, int argc, naRef* args)
{
    int i;
    naRef nlen, result, v = args[0];
    int len = 0, start = (int)naNumValue(args[1]).num;
    if(argc < 2) return naNil();
    nlen = argc > 2 ? naNumValue(args[2]) : naNil();
    if(!naIsNil(nlen))
        len = (int)nlen.num;
    if(!naIsVector(v) || start < 0 || start >= naVec_size(v) || len < 0)
        return naNil();
    if(len == 0 || len > naVec_size(v) - start) len = naVec_size(v) - start;
    result = naNewVector(c);
    naVec_setsize(result, len);
    for(i=0; i<len; i++)
        naVec_set(result, i, naVec_get(v, start + i));
    return result;
}

static naRef delete(naContext c, naRef me, int argc, naRef* args)
{
    if(argc > 1 && naIsHash(args[0])) naHash_delete(args[0], args[1]);
    return naNil();
}

static naRef intf(naContext c, naRef me, int argc, naRef* args)
{
    if(argc > 0) {
        naRef n = naNumValue(args[0]);
        if(naIsNil(n)) return n;
        if(n.num < 0) n.num = -floor(-n.num);
        else n.num = floor(n.num);
        return n;
    } else return naNil();
}

static naRef num(naContext c, naRef me, int argc, naRef* args)
{
    return argc > 0 ? naNumValue(args[0]) : naNil();
}

static naRef streq(naContext c, naRef me, int argc, naRef* args)
{
    return argc > 1 ? naNum(naStrEqual(args[0], args[1])) : naNil();
}

static naRef substr(naContext c, naRef me, int argc, naRef* args)
{
    naRef src = argc > 1 ? args[0] : naNil();
    naRef startR = argc > 1 ? args[1] : naNil();
    naRef lenR = argc > 2 ? args[2] : naNil();
    int start, len;
    if(!naIsString(src)) return naNil();
    startR = naNumValue(startR);
    if(naIsNil(startR)) return naNil();
    start = (int)startR.num;
    if(naIsNil(lenR)) {
        len = naStr_len(src) - start;
    } else {
        lenR = naNumValue(lenR);
        if(naIsNil(lenR)) return naNil();
        len = (int)lenR.num;
    }
    return naStr_substr(naNewString(c), src, start, len);
}

static naRef f_strc(naContext c, naRef me, int argc, naRef* args)
{
    int idx;
    struct naStr* str = args[0].ref.ptr.str;
    naRef idr = argc > 1 ? naNumValue(args[1]) : naNum(0);
    if(argc < 2 || IS_NIL(idr) || !IS_STR(args[0]))
        naRuntimeError(c, "bad arguments to strc");
    idx = (int)naNumValue(idr).num;
    if(idx > str->len) naRuntimeError(c, "strc index out of bounds");
    return naNum(str->data[idx]);
}

static naRef f_chr(naContext c, naRef me, int argc, naRef* args)
{
    char chr[1];
    naRef cr = argc ? naNumValue(args[0]) : naNil();
    if(IS_NIL(cr)) naRuntimeError(c, "chr argument not string");
    chr[0] = (char)cr.num;
    return NEWSTR(c, chr, 1);
}

static naRef contains(naContext c, naRef me, int argc, naRef* args)
{
    naRef hash = argc > 0 ? args[0] : naNil();
    naRef key = argc > 1 ? args[1] : naNil();
    if(naIsNil(hash) || naIsNil(key)) return naNil();
    if(!naIsHash(hash)) return naNil();
    return naHash_get(hash, key, &key) ? naNum(1) : naNum(0);
}

static naRef typeOf(naContext c, naRef me, int argc, naRef* args)
{
    naRef r = argc > 0 ? args[0] : naNil();
    char* t = "unknown";
    if(naIsNil(r)) t = "nil";
    else if(naIsNum(r)) t = "scalar";
    else if(naIsString(r)) t = "scalar";
    else if(naIsVector(r)) t = "vector";
    else if(naIsHash(r)) t = "hash";
    else if(naIsFunc(r)) t = "func";
    else if(naIsGhost(r)) t = "ghost";
    r = NEWSTR(c, t, strlen(t));
    return r;
}

static naRef f_compile(naContext c, naRef me, int argc, naRef* args)
{
    int errLine;
    naRef script, code, fname;
    script = argc > 0 ? args[0] : naNil();
    if(!naIsString(script)) return naNil();
    fname = NEWSTR(c, "<compile>", 9);
    code = naParseCode(c, fname, 1,
                       naStr_data(script), naStr_len(script), &errLine);
    if(!naIsCode(code)) return naNil(); // FIXME: export error to caller...
    return naBindToContext(c, code);
}

// Funcation metacall API.  Allows user code to generate an arg vector
// at runtime and/or call function references on arbitrary objects.
static naRef f_call(naContext c, naRef me, int argc, naRef* args)
{
    naContext subc;
    naRef callargs, callme, result;
    callargs = argc > 1 ? args[1] : naNil();
    callme = argc > 2 ? args[2] : naNil(); // Might be nil, that's OK
    if(!naIsFunc(args[0])) naRuntimeError(c, "call() on non-function");
    if(naIsNil(callargs)) callargs = naNewVector(c);
    else if(!naIsVector(callargs)) naRuntimeError(c, "call() args not vector");
    if(!naIsHash(callme)) callme = naNil();
    subc = naNewContext();
    subc->callParent = c;
    c->callChild = subc;
    result = naCall(subc, args[0], callargs, callme, naNil());
    c->callChild = 0;
    if(argc > 2 && !IS_NIL(subc->dieArg))
        if(naIsVector(args[argc-1]))
            naVec_append(args[argc-1], subc->dieArg);
    naFreeContext(subc);
    return result;
}

static naRef f_die(naContext c, naRef me, int argc, naRef* args)
{
    c->dieArg = argc > 0 ? args[0] : naNil();
    naRuntimeError(c, "__die__");
    return naNil(); // never executes
}

// Wrapper around vsnprintf, iteratively increasing the buffer size
// until it fits.  Returned buffer should be freed by the caller.
char* dosprintf(char* f, ...)
{
    char* buf;
    va_list va;
    int len = 16;
    while(1) {
        buf = naAlloc(len);
        va_start(va, f);
        if(vsnprintf(buf, len, f, va) < len) {
            va_end(va);
            return buf;
        }
        va_end(va);
        naFree(buf);
        len *= 2;
    }
}

// Inspects a printf format string f, and finds the next "%..." format
// specifier.  Stores the start of the specifier in out, the length in
// len, and the type in type.  Returns a pointer to the remainder of
// the format string, or 0 if no format string was found.  Recognizes
// all of ANSI C's syntax except for the "length modifier" feature.
// Note: this does not validate the format character returned in
// "type". That is the caller's job.
static char* nextFormat(naContext ctx, char* f, char** out, int* len, char* type)
{
    // Skip to the start of the format string
    while(*f && *f != '%') f++;
    if(!*f) return 0;
    *out = f++;

    while(*f && (*f=='-' || *f=='+' || *f==' ' || *f=='0' || *f=='#')) f++;

    // Test for duplicate flags.  This is pure pedantry and could
    // be removed on all known platforms, but just to be safe...
    {   char *p1, *p2;
        for(p1 = *out + 1; p1 < f; p1++)
            for(p2 = p1+1; p2 < f; p2++)
                if(*p1 == *p2)
                    naRuntimeError(ctx, "duplicate flag in format string"); }

    while(*f && *f >= '0' && *f <= '9') f++;
    if(*f && *f == '.') f++;
    while(*f && *f >= '0' && *f <= '9') f++;
    if(!*f) naRuntimeError(ctx, "invalid format string");

    *type = *f++;
    *len = f - *out;
    return f;
}

#define ERR(m) naRuntimeError(ctx, m)
#define APPEND(r) result = naStr_concat(naNewString(ctx), result, r)
static naRef f_sprintf(naContext ctx, naRef me, int argc, naRef* args)
{
    char t, nultmp, *fstr, *next, *fout=0, *s;
    int flen, argn=1;
    naRef format, arg, result = naNewString(ctx);

    if(argc < 1) ERR("not enough arguments to sprintf");
    format = naStringValue(ctx, argc > 0 ? args[0] : naNil());
    if(naIsNil(format)) ERR("bad format string in sprintf");
    s = naStr_data(format);
                               
    while((next = nextFormat(ctx, s, &fstr, &flen, &t))) {
        APPEND(NEWSTR(ctx, s, fstr-s)); // stuff before the format string
        if(flen == 2 && fstr[1] == '%') {
            APPEND(NEWSTR(ctx, "%", 1));
            s = next;
            continue;
        }
        if(argn >= argc) ERR("not enough arguments to sprintf");
        arg = args[argn++];
        nultmp = fstr[flen]; // sneaky nul termination...
        fstr[flen] = 0;
        if(t == 's') {
            arg = naStringValue(ctx, arg);
            if(naIsNil(arg)) fout = dosprintf(fstr, "nil");
            else             fout = dosprintf(fstr, naStr_data(arg));
        } else {
            arg = naNumValue(arg);
            if(naIsNil(arg))
                fout = dosprintf(fstr, "nil");
            else if(t=='d' || t=='i' || t=='c')
                fout = dosprintf(fstr, (int)naNumValue(arg).num);
            else if(t=='o' || t=='u' || t=='x' || t=='X')
                fout = dosprintf(fstr, (unsigned int)naNumValue(arg).num);
            else if(t=='e' || t=='E' || t=='f' || t=='F' || t=='g' || t=='G')
                fout = dosprintf(fstr, naNumValue(arg).num);
            else
                ERR("invalid sprintf format type");
        }
        fstr[flen] = nultmp;
        APPEND(NEWSTR(ctx, fout, strlen(fout)));
        naFree(fout);
        s = next;
    }
    APPEND(NEWSTR(ctx, s, strlen(s)));
    return result;
}

// FIXME: handle ctx->callParent frames too!
static naRef f_caller(naContext ctx, naRef me, int argc, naRef* args)
{
    int fidx;
    struct Frame* frame;
    naRef result, fr = argc ? naNumValue(args[0]) : naNil();
    if(IS_NIL(fr)) naRuntimeError(ctx, "non numeric argument to caller()");
    fidx = (int)fr.num;
    if(fidx > ctx->fTop - 1) return naNil();
    frame = &ctx->fStack[ctx->fTop - 1 - fidx];
    result = naNewVector(ctx);
    naVec_append(result, frame->locals);
    naVec_append(result, frame->func);
    naVec_append(result, frame->func.ref.ptr.func->code.ref.ptr.code->srcFile);
    naVec_append(result, naNum(naGetLine(ctx, fidx)));
    return result;
}

static naRef f_closure(naContext ctx, naRef me, int argc, naRef* args)
{
    int i;
    naRef func, idx;
    struct naFunc* f;
    func = argc > 0 ? args[0] : naNil();
    idx = argc > 1 ? naNumValue(args[1]) : naNil();
    if(!IS_FUNC(func) || IS_NIL(idx))
        naRuntimeError(ctx, "bad arguments to closure()");
    i = (int)idx.num;
    f = func.ref.ptr.func;
    while(i > 0 && f) { i--; f = f->next.ref.ptr.func; }
    if(!f) return naNil();
    return f->namespace;
}

static int match(char* a, char* b, int l)
{
    int i;
    for(i=0; i<l; i++) if(a[i] != b[i]) return 0;
    return 1;
}

static int find(char* a, int al, char* s, int sl)
{
    int i;
    if(al == 0) return 0;
    for(i=0; i<sl-al+1; i++) if(match(a, s+i, al)) return i;
    return -1;
}

static naRef f_find(naContext ctx, naRef me, int argc, naRef* args)
{
    if(argc < 2 || !IS_STR(args[0]) || !IS_STR(args[1]))
        naRuntimeError(ctx, "bad/missing argument to split");
    return naNum(find(args[0].ref.ptr.str->data, args[0].ref.ptr.str->len,
                      args[1].ref.ptr.str->data, args[1].ref.ptr.str->len));
}

static naRef f_split(naContext ctx, naRef me, int argc, naRef* args)
{
    int sl, dl, i;
    char *s, *d, *s0;
    naRef result;
    if(argc < 2 || !IS_STR(args[0]) || !IS_STR(args[1]))
        naRuntimeError(ctx, "bad/missing argument to split");
    d = naStr_data(args[0]); dl = naStr_len(args[0]);
    s = naStr_data(args[1]); sl = naStr_len(args[1]);
    result = naNewVector(ctx);
    if(dl == 0) { // special case zero-length delimiter
        for(i=0; i<sl; i++) naVec_append(result, NEWSTR(ctx, s+i, 1));
        return result;
    }
    s0 = s;
    for(i=0; i <= sl-dl; i++) {
        if(match(s+i, d, dl)) {
            naVec_append(result, NEWSTR(ctx, s0, s+i-s0));
            s0 = s + i + dl;
            i += dl - 1;
        }
    }
    if(s0 - s <= sl) naVec_append(result, NEWSTR(ctx, s0, s+sl-s0));
    return result;
}

// This is a comparatively weak RNG, based on the C library's rand()
// function, which is usually not threadsafe and often of limited
// precision.  The 5x loop guarantees that we get a full double worth
// of precision even for 15 bit (Win32...) rand() implementations.
static naRef f_rand(naContext ctx, naRef me, int argc, naRef* args)
{
    int i;
    double r = 0;
    if(argc) {
        if(!IS_NUM(args[0])) naRuntimeError(ctx, "rand() seed not number");
        srand((unsigned int)args[0].num);
        return naNil();
    }
    for(i=0; i<5; i++) r = (r + rand()) * (1.0/(RAND_MAX+1.0));
    return naNum(r);
}

struct func { char* name; naCFunction func; };
static struct func funcs[] = {
    { "size", size },
    { "keys", keys }, 
    { "append", append }, 
    { "pop", pop }, 
    { "setsize", setsize }, 
    { "subvec", subvec }, 
    { "delete", delete }, 
    { "int", intf },
    { "num", num },
    { "streq", streq },
    { "substr", substr },
    { "strc", f_strc },
    { "chr", f_chr },
    { "contains", contains },
    { "typeof", typeOf },
    { "compile", f_compile },
    { "call", f_call },
    { "die", f_die },
    { "sprintf", f_sprintf },
    { "caller", f_caller },
    { "closure", f_closure },
    { "find", f_find },
    { "split", f_split },
    { "rand", f_rand },
};

naRef naStdLib(naContext c)
{
    naRef namespace = naNewHash(c);
    int i, n = sizeof(funcs)/sizeof(struct func);
    for(i=0; i<n; i++) {
        naRef code = naNewCCode(c, funcs[i].func);
        naRef name = NEWSTR(c, funcs[i].name, strlen(funcs[i].name));
        name = naInternSymbol(name);
        naHash_set(namespace, name, naNewFunc(c, code));
    }
    return namespace;
}
