#include "nasal.h"
#include "code.h"

////////////////////////////////////////////////////////////////////////
// Debugging stuff. ////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//#define DEBUG_NASAL
#if !defined(DEBUG_NASAL)
# define DBG(expr) /* noop */
#else
# define DBG(expr) expr
# include <stdio.h>
# include <stdlib.h>
#endif
char* opStringDEBUG(int op);
void printOpDEBUG(int ip, int op);
void printStackDEBUG(struct Context* ctx);
////////////////////////////////////////////////////////////////////////

struct Globals* globals = 0;

static naRef bindFunction(struct Context* ctx, struct Frame* f, naRef code);

#define ERR(c, msg) naRuntimeError((c),(msg))
void naRuntimeError(struct Context* c, char* msg)
{ 
    c->error = msg;
    longjmp(c->jumpHandle, 1);
}

static int boolify(struct Context* ctx, naRef r)
{
    if(IS_NUM(r)) return r.num != 0;
    if(IS_NIL(r)) return 0;
    if(IS_STR(r)) {
        double d;
        if(naStr_len(r) == 0) return 0;
        if(naStr_tonum(r, &d)) return d != 0;
        else return 1;
    }
    ERR(ctx, "non-scalar used in boolean context");
    return 0;
}

static double numify(struct Context* ctx, naRef o)
{
    double n;
    if(IS_NUM(o)) return o.num;
    else if(IS_NIL(o)) ERR(ctx, "nil used in numeric context");
    else if(!IS_STR(o)) ERR(ctx, "non-scalar in numeric context");
    else if(naStr_tonum(o, &n)) return n;
    else ERR(ctx, "non-numeric string in numeric context");
    return 0;
}

static naRef stringify(struct Context* ctx, naRef r)
{
    if(IS_STR(r)) return r;
    if(IS_NUM(r)) return naStr_fromnum(naNewString(ctx), r.num);
    ERR(ctx, "non-scalar in string context");
    return naNil();
}

static int checkVec(struct Context* ctx, naRef vec, naRef idx)
{
    int i = (int)numify(ctx, idx);
    if(i < 0 || i >= vec.ref.ptr.vec->rec->size)
        ERR(ctx, "vector index out of bounds");
    return i;
}

static naRef containerGet(struct Context* ctx, naRef box, naRef key)
{
    naRef result = naNil();
    if(!IS_SCALAR(key)) ERR(ctx, "container index not scalar");
    if(IS_HASH(box)) {
        if(!naHash_get(box, key, &result))
            ERR(ctx, "undefined value in container");
    } else if(IS_VEC(box)) {
        result = naVec_get(box, checkVec(ctx, box, key));
    } else {
        ERR(ctx, "extract from non-container");
    }
    return result;
}

static void containerSet(struct Context* ctx, naRef box, naRef key, naRef val)
{
    if(!IS_SCALAR(key))   ERR(ctx, "container index not scalar");
    else if(IS_HASH(box)) naHash_set(box, key, val);
    else if(IS_VEC(box))  naVec_set(box, checkVec(ctx, box, key), val);
    else                  ERR(ctx, "insert into non-container");
}

static void initContext(struct Context* c)
{
    int i;
    c->fTop = c->opTop = c->markTop = 0;
    for(i=0; i<NUM_NASAL_TYPES; i++)
        c->nfree[i] = 0;
    naVec_setsize(c->temps, 4);
    c->callParent = 0;
    c->callChild = 0;
    c->dieArg = naNil();
    c->error = 0;
}

static void initGlobals()
{
    int i;
    struct Context* c;
    globals = (struct Globals*)naAlloc(sizeof(struct Globals));
    naBZero(globals, sizeof(struct Globals));

    globals->sem = naNewSem();
    globals->lock = naNewLock();

    globals->allocCount = 256; // reasonable starting value
    for(i=0; i<NUM_NASAL_TYPES; i++)
        naGC_init(&(globals->pools[i]), i);
    globals->deadsz = 256;
    globals->ndead = 0;
    globals->deadBlocks = naAlloc(sizeof(void*) * globals->deadsz);

    // Initialize a single context
    globals->freeContexts = 0;
    globals->allContexts = 0;
    c = naNewContext();

    globals->symbols = naNewHash(c);
    globals->save = naNewVector(c);

    // Cache pre-calculated "me", "arg" and "parents" scalars
    globals->meRef = naInternSymbol(naStr_fromdata(naNewString(c), "me", 2));
    globals->argRef = naInternSymbol(naStr_fromdata(naNewString(c), "arg", 3));
    globals->parentsRef = naInternSymbol(naStr_fromdata(naNewString(c), "parents", 7));

    naFreeContext(c);
}

struct Context* naNewContext()
{
    int dummy;
    struct Context* c;
    if(globals == 0)
        initGlobals();

    LOCK();
    c = globals->freeContexts;
    if(c) {
        globals->freeContexts = c->nextFree;
        c->nextFree = 0;
        UNLOCK();
        initContext(c);
    } else {
        UNLOCK();
        c = (struct Context*)naAlloc(sizeof(struct Context));
        // Chicken and egg, can't use naNew because it requires temps to exist
        c->temps = naObj(T_VEC, (naGC_get(&globals->pools[T_VEC], 1, &dummy))[0]);
        initContext(c);
        LOCK();
        c->nextAll = globals->allContexts;
        c->nextFree = 0;
        globals->allContexts = c;
        UNLOCK();
    }
    return c;
}

void naFreeContext(struct Context* c)
{
    naVec_setsize(c->temps, 0);
    LOCK();
    c->nextFree = globals->freeContexts;
    globals->freeContexts = c;
    UNLOCK();
}

#define PUSH(r) do { \
    if(ctx->opTop >= MAX_STACK_DEPTH) ERR(ctx, "stack overflow"); \
    ctx->opStack[ctx->opTop++] = r; \
    } while(0)

struct Frame* setupFuncall(struct Context* ctx, int nargs, int mcall, int tail)
{
    int i;
    naRef *frame;
    struct Frame* f;
    struct naCode* c;
    
    DBG(printf("setupFuncall(nargs:%d, mcall:%d)\n", nargs, mcall);)

    frame = &ctx->opStack[ctx->opTop - nargs - 1];
    if(!IS_FUNC(frame[0]))
        ERR(ctx, "function/method call invoked on uncallable object");

    // Just do native calls right here, and don't touch the stack
    // frames; return the current one (unless it's a tail call!).
    if(frame[0].ref.ptr.func->code.ref.ptr.obj->type == T_CCODE) {
        naRef obj = mcall ? frame[-1] : naNil();
        naCFunction fp = frame[0].ref.ptr.func->code.ref.ptr.ccode->fptr;
        naRef result = (*fp)(ctx, obj, nargs, frame + 1);
        ctx->opTop -= nargs + 1 + mcall;
        PUSH(result);
        return &(ctx->fStack[ctx->fTop-1]);
    }
    
    if(tail) ctx->fTop--;
    else if(ctx->fTop >= MAX_RECURSION) ERR(ctx, "call stack overflow");

    // Note: assign nil first, otherwise the naNew() can cause a GC,
    // which will now (after fTop++) see the *old* reference as a
    // markable value!
    f = &(ctx->fStack[ctx->fTop++]);
    f->locals = f->func = naNil();
    f->locals = naNewHash(ctx);
    f->func = frame[0];
    f->ip = 0;
    f->bp = ctx->opTop - (nargs + 1 + mcall);

    if(mcall)
        naHash_set(f->locals, globals->meRef, frame[-1]);

    // Set the argument symbols, and put any remaining args in a vector
    c = (*frame++).ref.ptr.func->code.ref.ptr.code;
    if(nargs < c->nArgs) ERR(ctx, "not enough arguments to function call");
    for(i=0; i<c->nArgs; i++)
        naHash_newsym(f->locals.ref.ptr.hash,
                      &c->constants[c->argSyms[i]], &frame[i]);
    frame += c->nArgs;
    nargs -= c->nArgs;
    for(i=0; i<c->nOptArgs; i++, nargs--) {
        naRef val = nargs > 0 ? frame[i] : c->constants[c->optArgVals[i]];
        if(IS_CODE(val))
            val = bindFunction(ctx, &ctx->fStack[ctx->fTop-2], val);
        naHash_newsym(f->locals.ref.ptr.hash, &c->constants[c->optArgSyms[i]], 
                      &val);
    }
    if(c->needArgVector || nargs > 0)
    {
        naRef args = naNewVector(ctx);
        naVec_setsize(args, nargs > 0 ? nargs : 0);
        for(i=0; i<nargs; i++)
            args.ref.ptr.vec->rec->array[i] = *frame++;
        naHash_newsym(f->locals.ref.ptr.hash, &c->restArgSym, &args);
    }

    ctx->opTop = f->bp; // Pop the stack last, to avoid GC lossage
    DBG(printf("Entering frame %d with %d args\n", ctx->fTop-1, nargs);)
    return f;
}

static naRef evalAndOr(struct Context* ctx, int op, naRef ra, naRef rb)
{
    int a = boolify(ctx, ra);
    int b = boolify(ctx, rb);
    int result;
    if(op == OP_AND) result = a && b ? 1 : 0;
    else             result = a || b ? 1 : 0;
    return naNum(result);
}

static naRef evalEquality(int op, naRef ra, naRef rb)
{
    int result = naEqual(ra, rb);
    return naNum((op==OP_EQ) ? result : !result);
}

// When a code object comes out of the constant pool and shows up on
// the stack, it needs to be bound with the lexical context.
static naRef bindFunction(struct Context* ctx, struct Frame* f, naRef code)
{
    naRef result = naNewFunc(ctx, code);
    result.ref.ptr.func->namespace = f->locals;
    result.ref.ptr.func->next = f->func;
    return result;
}

static int getClosure(struct naFunc* c, naRef sym, naRef* result)
{
    while(c) {
        if(naHash_get(c->namespace, sym, result)) return 1;
        c = c->next.ref.ptr.func;
    }
    return 0;
}

static naRef getLocal2(struct Context* ctx, struct Frame* f, naRef sym)
{
    naRef result;
    if(!naHash_get(f->locals, sym, &result))
        if(!getClosure(f->func.ref.ptr.func, sym, &result))
            ERR(ctx, "undefined symbol");
    return result;
}

static void getLocal(struct Context* ctx, struct Frame* f,
                     naRef* sym, naRef* out)
{
    struct naFunc* func;
    struct naStr* str = sym->ref.ptr.str;
    if(naHash_sym(f->locals.ref.ptr.hash, str, out))
        return;
    func = f->func.ref.ptr.func;
    while(func && func->namespace.ref.ptr.hash) {
        if(naHash_sym(func->namespace.ref.ptr.hash, str, out))
            return;
        func = func->next.ref.ptr.func;
    }
    // Now do it again using the more general naHash_get().  This will
    // only be necessary if something has created the value in the
    // namespace using the more generic hash syntax
    // (e.g. namespace["symbol"] and not namespace.symbol).
    *out = getLocal2(ctx, f, *sym);
}

static int setClosure(naRef func, naRef sym, naRef val)
{
    struct naFunc* c = func.ref.ptr.func;
    if(c == 0) { return 0; }
    else if(naHash_tryset(c->namespace, sym, val)) { return 1; }
    else { return setClosure(c->next, sym, val); }
}

static naRef setSymbol(struct Frame* f, naRef sym, naRef val)
{
    // Try the locals first, if not already there try the closures in
    // order.  Finally put it in the locals if nothing matched.
    if(!naHash_tryset(f->locals, sym, val))
        if(!setClosure(f->func, sym, val))
            naHash_set(f->locals, sym, val);
    return val;
}

// Recursively descend into the parents lists
static int getMember(struct Context* ctx, naRef obj, naRef fld,
                     naRef* result, int count)
{
    naRef p;
    if(--count < 0) ERR(ctx, "too many parents");
    if(!IS_HASH(obj)) ERR(ctx, "non-objects have no members");
    if(naHash_get(obj, fld, result)) {
        return 1;
    } else if(naHash_get(obj, globals->parentsRef, &p)) {
        if(IS_VEC(p)) {
            int i;
            struct VecRec* v = p.ref.ptr.vec->rec;
            for(i=0; i<v->size; i++)
                if(getMember(ctx, v->array[i], fld, result, count))
                    return 1;
        } else
            ERR(ctx, "parents field not vector");
    }
    return 0;
}

// OP_EACH works like a vector get, except that it leaves the vector
// and index on the stack, increments the index after use, and pops
// the arguments and pushes a nil if the index is beyond the end.
static void evalEach(struct Context* ctx, int useIndex)
{
    int idx = (int)(ctx->opStack[ctx->opTop-1].num);
    naRef vec = ctx->opStack[ctx->opTop-2];
    if(!IS_VEC(vec)) naRuntimeError(ctx, "foreach enumeration of non-vector");
    if(!vec.ref.ptr.vec->rec || idx >= vec.ref.ptr.vec->rec->size) {
        ctx->opTop -= 2; // pop two values
        PUSH(naNil());
        return;
    }
    ctx->opStack[ctx->opTop-1].num = idx+1; // modify in place
    PUSH(useIndex ? naNum(idx) : naVec_get(vec, idx));
}

#define ARG() cd->byteCode[f->ip++]
#define CONSTARG() cd->constants[ARG()]
#define POP() ctx->opStack[--ctx->opTop]
#define STK(n) (ctx->opStack[ctx->opTop-(n)])
#define FIXFRAME() f = &(ctx->fStack[ctx->fTop-1]); \
                   cd = f->func.ref.ptr.func->code.ref.ptr.code;
static naRef run(struct Context* ctx)
{
    struct Frame* f;
    struct naCode* cd;
    int op, arg;
    naRef a, b, c;

    FIXFRAME();

    while(1) {
        op = cd->byteCode[f->ip++];
        DBG(printf("Stack Depth: %d\n", ctx->opTop));
        DBG(printOpDEBUG(f->ip-1, op));
        switch(op) {
        case OP_POP:
            ctx->opTop--;
            break;
        case OP_DUP:
            PUSH(ctx->opStack[ctx->opTop-1]);
            break;
        case OP_DUP2:
            PUSH(ctx->opStack[ctx->opTop-2]);
            PUSH(ctx->opStack[ctx->opTop-2]);
            break;
        case OP_XCHG:
            a = STK(1); STK(1) = STK(2); STK(2) = a;
            break;

#define BINOP(expr) do { \
    double l = IS_NUM(STK(2)) ? STK(2).num : numify(ctx, STK(2)); \
    double r = IS_NUM(STK(1)) ? STK(1).num : numify(ctx, STK(1)); \
    STK(2).ref.reftag = ~NASAL_REFTAG; \
    STK(2).num = expr; \
    ctx->opTop--; } while(0)

        case OP_PLUS:  BINOP(l + r);         break;
        case OP_MINUS: BINOP(l - r);         break;
        case OP_MUL:   BINOP(l * r);         break;
        case OP_DIV:   BINOP(l / r);         break;
        case OP_LT:    BINOP(l <  r ? 1 : 0); break;
        case OP_LTE:   BINOP(l <= r ? 1 : 0); break;
        case OP_GT:    BINOP(l >  r ? 1 : 0); break;
        case OP_GTE:   BINOP(l >= r ? 1 : 0); break;

#undef BINOP

        case OP_EQ: case OP_NEQ:
            STK(2) = evalEquality(op, STK(2), STK(1));
            ctx->opTop--;
            break;
        case OP_AND: case OP_OR:
            STK(2) = evalAndOr(ctx, op, STK(2), STK(1));
            ctx->opTop--;
            break;
        case OP_CAT:
            // stringify can call the GC, so don't take stuff of the stack!
            a = stringify(ctx, ctx->opStack[ctx->opTop-1]);
            b = stringify(ctx, ctx->opStack[ctx->opTop-2]);
            c = naStr_concat(naNewString(ctx), b, a);
            ctx->opTop -= 2;
            PUSH(c);
            break;
        case OP_NEG:
            STK(1) = naNum(-numify(ctx, STK(1)));
            break;
        case OP_NOT:
            STK(1) = naNum(boolify(ctx, STK(1)) ? 0 : 1);
            break;
        case OP_PUSHCONST:
            a = CONSTARG();
            if(IS_CODE(a)) a = bindFunction(ctx, f, a);
            PUSH(a);
            break;
        case OP_PUSHONE:
            PUSH(naNum(1));
            break;
        case OP_PUSHZERO:
            PUSH(naNum(0));
            break;
        case OP_PUSHNIL:
            PUSH(naNil());
            break;
        case OP_NEWVEC:
            PUSH(naNewVector(ctx));
            break;
        case OP_VAPPEND:
            naVec_append(STK(2), STK(1));
            ctx->opTop--;
            break;
        case OP_NEWHASH:
            PUSH(naNewHash(ctx));
            break;
        case OP_HAPPEND:
            naHash_set(STK(3), STK(2), STK(1));
            ctx->opTop -= 2;
            break;
        case OP_LOCAL:
            a = CONSTARG();
            getLocal(ctx, f, &a, &b);
            PUSH(b);
            break;
        case OP_SETSYM:
            STK(2) = setSymbol(f, STK(2), STK(1));
            ctx->opTop--;
            break;
        case OP_SETLOCAL:
            naHash_set(f->locals, STK(2), STK(1));
            STK(2) = STK(1); // FIXME: reverse order of arguments instead!
            ctx->opTop--;
            break;
        case OP_MEMBER:
            if(!getMember(ctx, STK(1), CONSTARG(), &STK(1), 64))
                ERR(ctx, "no such member");
            break;
        case OP_SETMEMBER:
            if(!IS_HASH(STK(3))) ERR(ctx, "non-objects have no members");
            naHash_set(STK(3), STK(2), STK(1));
            STK(3) = STK(1); // FIXME: fix arg order instead
            ctx->opTop -= 2;
            break;
        case OP_INSERT:
            containerSet(ctx, STK(3), STK(2), STK(1));
            STK(3) = STK(1); // FIXME: codegen order again...
            ctx->opTop -= 2;
            break;
        case OP_EXTRACT:
            STK(2) = containerGet(ctx, STK(2), STK(1));
            ctx->opTop--;
            break;
        case OP_JMPLOOP:
            // Identical to JMP, except for locking
            naCheckBottleneck();
            f->ip = cd->byteCode[f->ip];
            DBG(printf("   [Jump to: %d]\n", f->ip);)
            break;
        case OP_JMP:
            f->ip = cd->byteCode[f->ip];
            DBG(printf("   [Jump to: %d]\n", f->ip);)
            break;
        case OP_JIFNIL:
            arg = ARG();
            if(IS_NIL(STK(1))) {
                ctx->opTop--; // Pops **ONLY** if it's nil!
                f->ip = arg;
                DBG(printf("   [Jump to: %d]\n", f->ip);)
            }
            break;
        case OP_JIFNOT:
            arg = ARG();
            if(!boolify(ctx, POP())) {
                f->ip = arg;
                DBG(printf("   [Jump to: %d]\n", f->ip);)
            }
            break;
        case OP_FCALL:
            f = setupFuncall(ctx, ARG(), 0, 0);
            cd = f->func.ref.ptr.func->code.ref.ptr.code;
            break;
        case OP_FTAIL:
            f = setupFuncall(ctx, ARG(), 0, 1);
            cd = f->func.ref.ptr.func->code.ref.ptr.code;
            break;
        case OP_MCALL:
            f = setupFuncall(ctx, ARG(), 1, 0);
            cd = f->func.ref.ptr.func->code.ref.ptr.code;
            break;
        case OP_MTAIL:
            f = setupFuncall(ctx, ARG(), 1, 1);
            cd = f->func.ref.ptr.func->code.ref.ptr.code;
            break;
        case OP_RETURN:
            a = STK(1);
            if(--ctx->fTop <= 0) return a;
            ctx->opTop = f->bp + 1; // restore the correct opstack frame!
            STK(1) = a;
            FIXFRAME();
            break;
        case OP_EACH:
            evalEach(ctx, 0);
            break;
        case OP_INDEX:
            evalEach(ctx, 1);
            break;
        case OP_MARK: // save stack state (e.g. "setjmp")
            if(ctx->markTop >= MAX_MARK_DEPTH)
                naRuntimeError(ctx, "mark stack overflow");
            ctx->markStack[ctx->markTop++] = ctx->opTop;
            break;
        case OP_UNMARK: // pop stack state set by mark
            ctx->markTop--;
            break;
        case OP_BREAK: // restore stack state (FOLLOW WITH JMP!)
            ctx->opTop = ctx->markStack[--ctx->markTop];
            break;
        default:
            ERR(ctx, "BUG: bad opcode");
        }
        ctx->temps.ref.ptr.vec->rec->size = 0; // reset GC temp vector
        DBG(printStackDEBUG(ctx);)
    }
    return naNil(); // unreachable
}
#undef POP
#undef CONSTARG
#undef STK
#undef FIXFRAME

void naSave(struct Context* ctx, naRef obj)
{
    naVec_append(globals->save, obj);
}

// FIXME: handle ctx->callParent
int naStackDepth(struct Context* ctx)
{
    return ctx->fTop;
}

// FIXME: handle ctx->callParent
int naGetLine(struct Context* ctx, int frame)
{
    struct Frame* f = &ctx->fStack[ctx->fTop-1-frame];
    naRef func = f->func;
    int ip = f->ip;
    if(IS_FUNC(func) && IS_CODE(func.ref.ptr.func->code)) {
        struct naCode* c = func.ref.ptr.func->code.ref.ptr.code;
        unsigned short* p = c->lineIps + c->nLines - 2;
        while(p >= c->lineIps && p[0] > ip)
            p -= 2;
        return p[1];
    }
    return -1;
}

// FIXME: handle ctx->callParent
naRef naGetSourceFile(struct Context* ctx, int frame)
{
    naRef f = ctx->fStack[ctx->fTop-1-frame].func;
    f = f.ref.ptr.func->code;
    return f.ref.ptr.code->srcFile;
}

char* naGetError(struct Context* ctx)
{
    if(IS_STR(ctx->dieArg))
        return ctx->dieArg.ref.ptr.str->data;
    return ctx->error;
}

naRef naBindFunction(naContext ctx, naRef code, naRef closure)
{
    naRef func = naNewFunc(ctx, code);
    func.ref.ptr.func->namespace = closure;
    func.ref.ptr.func->next = naNil();
    return func;
}

naRef naBindToContext(naContext ctx, naRef code)
{
    naRef func = naNewFunc(ctx, code);
    struct Frame* f = &ctx->fStack[ctx->fTop-1];
    func.ref.ptr.func->namespace = f->locals;
    func.ref.ptr.func->next = f->func;
    return func;
}

naRef naCall(naContext ctx, naRef func, naRef args, naRef obj, naRef locals)
{
    naRef result;
    if(!ctx->callParent) naModLock(ctx);

    // We might have to allocate objects, which can call the GC.  But
    // the call isn't on the Nasal stack yet, so the GC won't find our
    // C-space arguments.
    naVec_append(ctx->temps, func);
    naVec_append(ctx->temps, args);
    naVec_append(ctx->temps, obj);
    naVec_append(ctx->temps, locals);

    if(IS_NIL(locals))
        locals = naNewHash(ctx);
    if(!IS_FUNC(func))
        func = naNewFunc(ctx, func); // bind bare code objects

    if(!IS_NIL(args))
        naHash_set(locals, globals->argRef, args);
    if(!IS_NIL(obj))
        naHash_set(locals, globals->meRef, obj);

    ctx->dieArg = naNil();

    ctx->opTop = ctx->markTop = 0;
    ctx->fTop = 1;
    ctx->fStack[0].func = func;
    ctx->fStack[0].locals = locals;
    ctx->fStack[0].ip = 0;
    ctx->fStack[0].bp = ctx->opTop;

    // Return early if an error occurred.  It will be visible to the
    // caller via naGetError().
    ctx->error = 0;
    if(setjmp(ctx->jumpHandle))
        return naNil();

    if(IS_CCODE(func.ref.ptr.func->code)) {
        naCFunction fp = func.ref.ptr.func->code.ref.ptr.ccode->fptr;
        struct naVec* av = args.ref.ptr.vec;
        result = (*fp)(ctx, obj, av->rec->size, av->rec->array);
    } else
        result = run(ctx);
    if(!ctx->callParent) naModUnlock(ctx);
    return result;
}

