#include "parse.h"
#include "code.h"

// These are more sensical predicate names in most contexts in this file
#define LEFT(tok)   ((tok)->children)
#define RIGHT(tok)  ((tok)->lastChild)
#define BINARY(tok) (LEFT(tok) && RIGHT(tok) && LEFT(tok) != RIGHT(tok))

// Forward references for recursion
static void genExpr(struct Parser* p, struct Token* t);
static void genExprList(struct Parser* p, struct Token* t);

static void emit(struct Parser* p, int byte)
{
    if(p->cg->nBytes >= p->cg->codeAlloced) {
        int i, sz = p->cg->codeAlloced * 2;
        unsigned char* buf = naParseAlloc(p, sz);
        for(i=0; i<p->cg->codeAlloced; i++) buf[i] = p->cg->byteCode[i];
        p->cg->byteCode = buf;
        p->cg->codeAlloced = sz;
    }
    p->cg->byteCode[p->cg->nBytes++] = (unsigned char)byte;
}

static void emitImmediate(struct Parser* p, int byte, int arg)
{
    emit(p, byte);
    emit(p, arg >> 8);
    emit(p, arg & 0xff);
}

static void genBinOp(int op, struct Parser* p, struct Token* t)
{
    if(!LEFT(t) || !RIGHT(t))
        naParseError(p, "empty subexpression", t->line);
    genExpr(p, LEFT(t));
    genExpr(p, RIGHT(t));
    emit(p, op);
}

static int newConstant(struct Parser* p, naRef c)
{
    int i;
    naVec_append(p->cg->consts, c);
    i = naVec_size(p->cg->consts) - 1;
    if(i > 0xffff) naParseError(p, "too many constants in code block", 0);
    return i;
}

static naRef getConstant(struct Parser* p, int idx)
{
    return naVec_get(p->cg->consts, idx);
}

// Interns a scalar (!) constant and returns its index
static int internConstant(struct Parser* p, naRef c)
{
    int i, j, n = naVec_size(p->cg->consts);
    for(i=0; i<n; i++) {
        naRef b = naVec_get(p->cg->consts, i);
        if(IS_NUM(b) && IS_NUM(c) && b.num == c.num)
            return i;
        if(IS_STR(b) && IS_STR(c)) {
            int len = naStr_len(c);
            char* cs = naStr_data(c);
            char* bs = naStr_data(b);
            if(naStr_len(b) != len)
                continue;
            for(j=0; j<len; j++)
                if(cs[j] != bs[j])
                    continue;
        }
        if(IS_REF(b) && IS_REF(c))
            if(b.ref.ptr.obj->type == c.ref.ptr.obj->type)
                if(naEqual(b, c))
                    return i;
    }
    return newConstant(p, c);
}

static void genScalarConstant(struct Parser* p, struct Token* t)
{
    naRef c = (t->str
               ? naStr_fromdata(naNewString(p->context), t->str, t->strlen)
               : naNum(t->num));
    int idx = internConstant(p, c);
    emitImmediate(p, OP_PUSHCONST, idx);
}

static int genLValue(struct Parser* p, struct Token* t)
{
    if(t->type == TOK_LPAR) {
        return genLValue(p, LEFT(t)); // Handle stuff like "(a) = 1"
    } else if(t->type == TOK_SYMBOL) {
        genScalarConstant(p, t);
        return OP_SETLOCAL;
    } else if(t->type == TOK_DOT && RIGHT(t) && RIGHT(t)->type == TOK_SYMBOL) {
        genExpr(p, LEFT(t));
        genScalarConstant(p, RIGHT(t));
        return OP_SETMEMBER;
    } else if(t->type == TOK_LBRA) {
        genExpr(p, LEFT(t));
        genExpr(p, RIGHT(t));
        return OP_INSERT;
    } else {
        naParseError(p, "bad lvalue", t->line);
        return -1;
    }
}

static void genLambda(struct Parser* p, struct Token* t)
{
    int idx;
    struct CodeGenerator* cgSave;
    naRef codeObj;
    if(LEFT(t)->type != TOK_LCURL)
        naParseError(p, "bad function definition", t->line);

    // Save off the generator state while we do the new one
    cgSave = p->cg;
    codeObj = naCodeGen(p, LEFT(LEFT(t)));
    p->cg = cgSave;

    idx = newConstant(p, codeObj);
    emitImmediate(p, OP_PUSHCONST, idx);
}

static void genList(struct Parser* p, struct Token* t)
{
    if(t->type == TOK_COMMA) {
        genExpr(p, LEFT(t));
        emit(p, OP_VAPPEND);
        genList(p, RIGHT(t));
    } else if(t->type == TOK_EMPTY) {
        return;
    } else {
        genExpr(p, t);
        emit(p, OP_VAPPEND);
    }
}

static void genHashElem(struct Parser* p, struct Token* t)
{
    if(t->type == TOK_EMPTY)
        return;
    if(t->type != TOK_COLON)
        naParseError(p, "bad hash/object initializer", t->line);
    if(LEFT(t)->type == TOK_SYMBOL) genScalarConstant(p, LEFT(t));
    else if(LEFT(t)->type == TOK_LITERAL) genExpr(p, LEFT(t));
    else naParseError(p, "bad hash/object initializer", t->line);
    genExpr(p, RIGHT(t));
    emit(p, OP_HAPPEND);
}

static void genHash(struct Parser* p, struct Token* t)
{
    if(t->type == TOK_COMMA) {
        genHashElem(p, LEFT(t));
        genHash(p, RIGHT(t));
    } else if(t->type != TOK_EMPTY) {
        genHashElem(p, t);
    }
}

static void genFuncall(struct Parser* p, struct Token* t)
{
    int op = OP_FCALL;
    if(LEFT(t)->type == TOK_DOT) {
        genExpr(p, LEFT(LEFT(t)));
        emit(p, OP_DUP);
        genScalarConstant(p, RIGHT(LEFT(t)));
        emit(p, OP_MEMBER);
        op = OP_MCALL;
    } else {
        genExpr(p, LEFT(t));
    }
    emit(p, OP_NEWVEC);
    if(RIGHT(t)) genList(p, RIGHT(t));
    emit(p, op);
}

static void pushLoop(struct Parser* p, struct Token* label)
{
    int i = p->cg->loopTop;
    p->cg->loops[i].breakIP = 0xffffff;
    p->cg->loops[i].contIP = 0xffffff;
    p->cg->loops[i].label = label;
    p->cg->loopTop++;
    emit(p, OP_MARK);
}

static void popLoop(struct Parser* p)
{
    p->cg->loopTop--;
    if(p->cg->loopTop < 0) naParseError(p, "BUG: loop stack underflow", -1);
    emit(p, OP_UNMARK);
}

// Emit a jump operation, and return the location of the address in
// the bytecode for future fixup in fixJumpTarget
static int emitJump(struct Parser* p, int op)
{
    int ip;
    emit(p, op);
    ip = p->cg->nBytes;
    emit(p, 0xff); // dummy address
    emit(p, 0xff);
    return ip;
}

// Points a previous jump instruction at the current "end-of-bytecode"
static void fixJumpTarget(struct Parser* p, int spot)
{
    p->cg->byteCode[spot]   = p->cg->nBytes >> 8;
    p->cg->byteCode[spot+1] = p->cg->nBytes & 0xff;
}

static void genShortCircuit(struct Parser* p, struct Token* t)
{
    int jumpNext, jumpEnd, isAnd = (t->type == TOK_AND);
    genExpr(p, LEFT(t));
    if(isAnd) emit(p, OP_NOT);
    jumpNext = emitJump(p, OP_JIFNOT);
    emit(p, isAnd ? OP_PUSHNIL : OP_PUSHONE);
    jumpEnd = emitJump(p, OP_JMP);
    fixJumpTarget(p, jumpNext);
    genExpr(p, RIGHT(t));
    fixJumpTarget(p, jumpEnd);
}


static void genIf(struct Parser* p, struct Token* tif, struct Token* telse)
{
    int jumpNext, jumpEnd;
    genExpr(p, tif->children); // the test
    jumpNext = emitJump(p, OP_JIFNOT);
    genExprList(p, tif->children->next->children); // the body
    jumpEnd = emitJump(p, OP_JMP);
    fixJumpTarget(p, jumpNext);
    if(telse) {
        if(telse->type == TOK_ELSIF) genIf(p, telse, telse->next);
        else genExprList(p, telse->children->children);
    } else {
        emit(p, OP_PUSHNIL);
    }
    fixJumpTarget(p, jumpEnd);
}

static void genIfElse(struct Parser* p, struct Token* t)
{
    genIf(p, t, t->children->next->next);
}

static int countSemis(struct Token* t)
{
    if(!t || t->type != TOK_SEMI) return 0;
    return 1 + countSemis(RIGHT(t));
}

static void genLoop(struct Parser* p, struct Token* body,
                    struct Token* update, struct Token* label,
                    int loopTop, int jumpEnd)
{
    int cont, jumpOverContinue;
    
    p->cg->loops[p->cg->loopTop-1].breakIP = jumpEnd-1;

    jumpOverContinue = emitJump(p, OP_JMP);
    p->cg->loops[p->cg->loopTop-1].contIP = p->cg->nBytes;
    cont = emitJump(p, OP_JMP);
    fixJumpTarget(p, jumpOverContinue);

    genExprList(p, body);
    emit(p, OP_POP);
    fixJumpTarget(p, cont);
    if(update) { genExpr(p, update); emit(p, OP_POP); }
    emitImmediate(p, OP_JMP, loopTop);
    fixJumpTarget(p, jumpEnd);
    popLoop(p);
    emit(p, OP_PUSHNIL); // Leave something on the stack
}

static void genForWhile(struct Parser* p, struct Token* init,
                        struct Token* test, struct Token* update,
                        struct Token* body, struct Token* label)
{
    int loopTop, jumpEnd;
    if(init) { genExpr(p, init); emit(p, OP_POP); }
    pushLoop(p, label);
    loopTop = p->cg->nBytes;
    genExpr(p, test);
    jumpEnd = emitJump(p, OP_JIFNOT);
    genLoop(p, body, update, label, loopTop, jumpEnd);
}

static void genWhile(struct Parser* p, struct Token* t)
{
    struct Token *test=LEFT(t)->children, *body, *label=0;
    int semis = countSemis(test);
    if(semis == 1) {
        label = LEFT(test);
        if(!label || label->type != TOK_SYMBOL)
            naParseError(p, "bad loop label", t->line);
        test = RIGHT(test);
    }
    else if(semis != 0)
        naParseError(p, "too many semicolons in while test", t->line);
    body = LEFT(RIGHT(t));
    genForWhile(p, 0, test, 0, body, label);
}

static void genFor(struct Parser* p, struct Token* t)
{
    struct Token *init, *test, *body, *update, *label=0;
    struct Token *h = LEFT(t)->children;
    int semis = countSemis(h);
    if(semis == 3) {
        if(!LEFT(h) || LEFT(h)->type != TOK_SYMBOL)
            naParseError(p, "bad loop label", h->line);
        label = LEFT(h);
        h=RIGHT(h);
    } else if(semis != 2) {
        naParseError(p, "wrong number of terms in for header", t->line);
    }

    // Parse tree hell :)
    init = LEFT(h);
    test = LEFT(RIGHT(h));
    update = RIGHT(RIGHT(h));
    body = RIGHT(t)->children;
    genForWhile(p, init, test, update, body, label);
}

static void genForEach(struct Parser* p, struct Token* t)
{
    int loopTop, jumpEnd, assignOp;
    struct Token *elem, *body, *vec, *label=0;
    struct Token *h = LEFT(LEFT(t));
    int semis = countSemis(h);
    if(semis == 2) {
        if(!LEFT(h) || LEFT(h)->type != TOK_SYMBOL)
            naParseError(p, "bad loop label", h->line);
        label = LEFT(h);
        h = RIGHT(h);
    } else if (semis != 1) {
        naParseError(p, "wrong number of terms in foreach header", t->line);
    }
    elem = LEFT(h);
    vec = RIGHT(h);
    body = RIGHT(t)->children;

    pushLoop(p, label);
    genExpr(p, vec);
    emit(p, OP_PUSHZERO);
    loopTop = p->cg->nBytes;
    emit(p, OP_EACH);
    jumpEnd = emitJump(p, OP_JIFNIL);
    assignOp = genLValue(p, elem);
    emit(p, OP_XCHG);
    emit(p, assignOp);
    emit(p, OP_POP);
    genLoop(p, body, 0, label, loopTop, jumpEnd);
}

static int tokMatch(struct Token* a, struct Token* b)
{
    int i, l = a->strlen;
    if(!a || !b) return 0;
    if(l != b->strlen) return 0;
    for(i=0; i<l; i++) if(a->str[i] != b->str[i]) return 0;
    return 1;
}

static void genBreakContinue(struct Parser* p, struct Token* t)
{
    int levels = 1, loop = -1, bp, cp, i;
    if(RIGHT(t)) {
        if(RIGHT(t)->type != TOK_SYMBOL)
            naParseError(p, "bad break/continue label", t->line);
        for(i=0; i<p->cg->loopTop; i++)
            if(tokMatch(RIGHT(t), p->cg->loops[i].label))
                loop = i;
        if(loop == -1)
            naParseError(p, "no match for break/continue label", t->line);
        levels = p->cg->loopTop - loop;
    }
    bp = p->cg->loops[p->cg->loopTop - levels].breakIP;
    cp = p->cg->loops[p->cg->loopTop - levels].contIP;
    for(i=0; i<levels; i++)
        emit(p, OP_BREAK);
    if(t->type == TOK_BREAK)
        emit(p, OP_PUSHNIL); // breakIP is always a JIFNOT/JIFNIL!
    emitImmediate(p, OP_JMP, t->type == TOK_BREAK ? bp : cp);
}

static void genExpr(struct Parser* p, struct Token* t)
{
    int i;
    if(t == 0)
        naParseError(p, "BUG: null subexpression", -1);
    if(t->line != p->cg->lastLine)
        emitImmediate(p, OP_LINE, t->line);
    p->cg->lastLine = t->line;
    switch(t->type) {
    case TOK_IF:
        genIfElse(p, t);
        break;
    case TOK_WHILE:
        genWhile(p, t);
        break;
    case TOK_FOR:
        genFor(p, t);
        break;
    case TOK_FOREACH:
        genForEach(p, t);
        break;
    case TOK_BREAK: case TOK_CONTINUE:
        genBreakContinue(p, t);
        break;
    case TOK_TOP:
        genExprList(p, LEFT(t));
        break;
    case TOK_FUNC:
        genLambda(p, t);
        break;
    case TOK_LPAR:
        if(BINARY(t) || !RIGHT(t)) genFuncall(p, t); // function invocation
        else          genExpr(p, LEFT(t)); // simple parenthesis
        break;
    case TOK_LBRA:
        if(BINARY(t)) {
            genBinOp(OP_EXTRACT, p, t); // a[i]
        } else {
            emit(p, OP_NEWVEC);
            genList(p, LEFT(t));
        }
        break;
    case TOK_LCURL:
        emit(p, OP_NEWHASH);
        genHash(p, LEFT(t));
        break;
    case TOK_ASSIGN:
        i = genLValue(p, LEFT(t));
        genExpr(p, RIGHT(t));
        emit(p, i); // use the op appropriate to the lvalue
        break;
    case TOK_RETURN:
        if(RIGHT(t)) genExpr(p, RIGHT(t));
        else emit(p, OP_PUSHNIL);
        emit(p, OP_RETURN);
        break;
    case TOK_NOT:
        genExpr(p, RIGHT(t));
        emit(p, OP_NOT);
        break;
    case TOK_SYMBOL:
        genScalarConstant(p, t);
        emit(p, OP_LOCAL);
        break;
    case TOK_LITERAL:
        genScalarConstant(p, t);
        break;
    case TOK_MINUS:
        if(BINARY(t)) {
            genBinOp(OP_MINUS,  p, t);  // binary subtraction
        } else if(RIGHT(t)->type == TOK_LITERAL && !RIGHT(t)->str) {
            RIGHT(t)->num *= -1;        // Pre-negate constants
            genScalarConstant(p, RIGHT(t));
        } else {
            genExpr(p, RIGHT(t));       // unary negation
            emit(p, OP_NEG);
        }
        break;
    case TOK_NEG:
        genExpr(p, RIGHT(t)); // unary negation (see also TOK_MINUS!)
        emit(p, OP_NEG);
        break;
    case TOK_DOT:
        genExpr(p, LEFT(t));
        if(RIGHT(t)->type != TOK_SYMBOL)
            naParseError(p, "object field not symbol", RIGHT(t)->line);
        genScalarConstant(p, RIGHT(t));
        emit(p, OP_MEMBER);
        break;
    case TOK_EMPTY: case TOK_NIL:
        emit(p, OP_PUSHNIL); break; // *NOT* a noop!
    case TOK_AND: case TOK_OR:
        genShortCircuit(p, t);
        break;
    case TOK_MUL:   genBinOp(OP_MUL,    p, t); break;
    case TOK_PLUS:  genBinOp(OP_PLUS,   p, t); break;
    case TOK_DIV:   genBinOp(OP_DIV,    p, t); break;
    case TOK_CAT:   genBinOp(OP_CAT,    p, t); break;
    case TOK_LT:    genBinOp(OP_LT,     p, t); break;
    case TOK_LTE:   genBinOp(OP_LTE,    p, t); break;
    case TOK_EQ:    genBinOp(OP_EQ,     p, t); break;
    case TOK_NEQ:   genBinOp(OP_NEQ,    p, t); break;
    case TOK_GT:    genBinOp(OP_GT,     p, t); break;
    case TOK_GTE:   genBinOp(OP_GTE,    p, t); break;
    default:
        naParseError(p, "parse error", t->line);
    };
}

static void genExprList(struct Parser* p, struct Token* t)
{
    if(t->type == TOK_SEMI) {
        genExpr(p, LEFT(t));
        if(RIGHT(t) && RIGHT(t)->type != TOK_EMPTY) {
            emit(p, OP_POP);
            genExprList(p, RIGHT(t));
        }
    } else {
        genExpr(p, t);
    }
}

naRef naCodeGen(struct Parser* p, struct Token* t)
{
    int i;
    naRef codeObj;
    struct naCode* code;
    struct CodeGenerator cg;

    cg.lastLine = 0;
    cg.codeAlloced = 1024; // Start fairly big, this is a cheap allocation
    cg.byteCode = naParseAlloc(p, cg.codeAlloced);
    cg.nBytes = 0;
    cg.consts = naNewVector(p->context);
    cg.loopTop = 0;
    p->cg = &cg;

    genExprList(p, t);

    // Now make a code object
    codeObj = naNewCode(p->context);
    code = codeObj.ref.ptr.code;
    code->nBytes = cg.nBytes;
    code->byteCode = naAlloc(cg.nBytes);
    for(i=0; i < cg.nBytes; i++)
        code->byteCode[i] = cg.byteCode[i];
    code->nConstants = naVec_size(cg.consts);
    code->constants = naAlloc(code->nConstants * sizeof(naRef));
    code->srcFile = p->srcFile;
    for(i=0; i<code->nConstants; i++)
        code->constants[i] = getConstant(p, i);

    return codeObj;
}
