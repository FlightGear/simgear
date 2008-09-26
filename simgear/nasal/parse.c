#include <setjmp.h>
#include <string.h>

#include "parse.h"

// Static precedence table, from low (loose binding, do first) to high
// (tight binding, do last).
#define MAX_PREC_TOKS 6
static const struct precedence {
    int toks[MAX_PREC_TOKS];
    int rule;
} PRECEDENCE[] = {
    { { TOK_SEMI, TOK_COMMA },                 PREC_REVERSE },
    { { TOK_ELLIPSIS },                        PREC_SUFFIX  },
    { { TOK_RETURN, TOK_BREAK, TOK_CONTINUE }, PREC_PREFIX  },
    { { TOK_ASSIGN, TOK_PLUSEQ, TOK_MINUSEQ,
        TOK_MULEQ, TOK_DIVEQ, TOK_CATEQ     }, PREC_REVERSE },
    { { TOK_COLON, TOK_QUESTION },             PREC_REVERSE },
    { { TOK_VAR },                             PREC_PREFIX  },
    { { TOK_OR },                              PREC_BINARY  },
    { { TOK_AND },                             PREC_BINARY  },
    { { TOK_EQ, TOK_NEQ },                     PREC_BINARY  },
    { { TOK_LT, TOK_LTE, TOK_GT, TOK_GTE },    PREC_BINARY  },
    { { TOK_PLUS, TOK_MINUS, TOK_CAT },        PREC_BINARY  },
    { { TOK_MUL, TOK_DIV },                    PREC_BINARY  },
    { { TOK_MINUS, TOK_NEG, TOK_NOT },         PREC_PREFIX  },
    { { TOK_LPAR, TOK_LBRA },                  PREC_SUFFIX  },
    { { TOK_DOT },                             PREC_BINARY  },
};
#define PRECEDENCE_LEVELS (sizeof(PRECEDENCE)/sizeof(struct precedence))

void naParseError(struct Parser* p, char* msg, int line)
{
    if(line > 0) p->errLine = line;
    p->err = msg;
    longjmp(p->jumpHandle, 1);
}

static void oops(struct Parser* p) { naParseError(p, "parse error", -1); }

void naParseInit(struct Parser* p)
{
    memset(p, 0, sizeof(*p));
    p->tree.type = TOK_TOP;
    p->tree.line = 1;
}

void naParseDestroy(struct Parser* p)
{
    int i;
    for(i=0; i<p->nChunks; i++) naFree(p->chunks[i]);
    naFree(p->chunks);
    naFree(p->chunkSizes);
    p->buf = 0;
}

void* naParseAlloc(struct Parser* p, int bytes)
{
    char* result;
    bytes = (bytes+7) & (~7); // Round up to 8 byte chunks for alignment
    
    if(p->leftInChunk < bytes) {
        void* newChunk;
        void** newChunks;
        int* newChunkSizes;
        int sz, i;

        sz = p->len;
        if(sz < bytes) sz = bytes;
        newChunk = naAlloc(sz);

        p->nChunks++;

        newChunks = naAlloc(p->nChunks * sizeof(void*));
        for(i=1; i<p->nChunks; i++) newChunks[i] = p->chunks[i-1];
        newChunks[0] = newChunk;
        naFree(p->chunks);
        p->chunks = newChunks;

        newChunkSizes = naAlloc(p->nChunks * sizeof(int));
        for(i=1; i<p->nChunks; i++) newChunkSizes[i] = p->chunkSizes[i-1];
        newChunkSizes[0] = sz;
        naFree(p->chunkSizes);
        p->chunkSizes = newChunkSizes;

        p->leftInChunk = sz;
    }

    result = (char *)p->chunks[0] + p->chunkSizes[0] - p->leftInChunk;
    p->leftInChunk -= bytes;
    return result;
}

static void addChild(struct Token *par, struct Token *ch)
{
    if(par->lastChild) {
        ch->prev = par->lastChild;
        par->lastChild->next = ch;
    } else
        par->children = ch;
    par->lastChild = ch;
}

static int endBrace(int tok)
{
    if(tok == TOK_LBRA) return TOK_RBRA;
    if(tok == TOK_LPAR) return TOK_RPAR;
    if(tok == TOK_LCURL) return TOK_RCURL;
    return -1;
}

static int isOpenBrace(int t)
{
    return t==TOK_LPAR || t==TOK_LBRA || t==TOK_LCURL;
}

static int isLoopoid(int t)
{
    return t==TOK_FOR || t==TOK_FOREACH || t==TOK_WHILE || t==TOK_FORINDEX;
}

static int isBlockoid(int t)
{
    return isLoopoid(t)||t==TOK_IF||t==TOK_ELSIF||t==TOK_ELSE||t==TOK_FUNC;
}

/* Yes, a bare else or elsif ends a block; it means we've reached the
 * end of the previous if/elsif clause. */
static int isBlockEnd(int t)
{
    return t==TOK_RPAR||t==TOK_RBRA||t==TOK_RCURL||t==TOK_ELSIF||t==TOK_ELSE;
}

/* To match C's grammar, "blockoid" expressions sometimes need
 * synthesized terminating semicolons to make them act like
 * "statements" in C.  Always add one after "loopoid"
 * (for/foreach/while) expressions.  Add one after a func if it
 * immediately follows an assignment, and add one after an
 * if/elsif/else if it is the first token in an expression list */
static int needsSemi(struct Token* t, struct Token* next)
{
    if(!next || next->type == TOK_SEMI || isBlockEnd(next->type)) return 0;
    if(t->type == TOK_IF)   return !t->prev || t->prev->type == TOK_SEMI;
    if(t->type == TOK_FUNC) return t->prev && t->prev->type == TOK_ASSIGN;
    if(isLoopoid(t->type))  return 1;
    return 0;
}

static struct Token* newToken(struct Parser* p, int type)
{
    struct Token* t = naParseAlloc(p, sizeof(struct Token));
    memset(t, 0, sizeof(*t));
    t->type = type;
    t->line = -1;
    return t;
}

static struct Token* parseToken(struct Parser* p, struct Token** list);

static void parseBlock(struct Parser* p, struct Token *top,
                       int end, struct Token** list)
{
    struct Token *t;
    while(*list) {
        if(isBlockEnd((*list)->type) && (*list)->type != end) break;
        if(end == TOK_SEMI && (*list)->type == TOK_COMMA) break;
        t = parseToken(p, list);
        if(t->type == end) return; /* drop end token on the floor */
        addChild(top, t);
        if(needsSemi(t, *list))
            addChild(top, newToken(p, TOK_SEMI));
    }
    /* Context dependency: end of block is a parse error UNLESS we're
     * looking for a statement terminator (a braceless block) or a -1
     * (the top level) */
    if(end != TOK_SEMI && end != -1) oops(p);
}

static struct Token* parseToken(struct Parser* p, struct Token** list)
{
    struct Token *t = *list;
    *list = t->next;
    if(t->next) t->next->prev = 0;
    t->next = t->prev = 0;
    p->errLine = t->line;

    if(!t) return 0;
    if(isOpenBrace(t->type)) {
        parseBlock(p, t, endBrace(t->type), list);
    } else if(isBlockoid(t->type)) {
        /* Read an optional paren expression */
        if(!*list) oops(p);
        if((*list)->type == TOK_LPAR)
            addChild(t, parseToken(p, list));

        /* And the code block, which might be implicit/braceless */
        if(!*list) oops(p);
        if((*list)->type == TOK_LCURL) {
            addChild(t, parseToken(p, list));
        } else {
            /* Context dependency: if we're reading a braceless block,
             * and the first (!) token is itself a "blockoid"
             * expression, it is parsed alone, otherwise, read to the
             * terminating semicolon. */
            struct Token *blk = newToken(p, TOK_LCURL);
            if(isBlockoid((*list)->type)) addChild(blk, parseToken(p, list));
            else                          parseBlock(p, blk, TOK_SEMI, list);
            addChild(t, blk);
        }

        /* Read the elsif/else chain */
        if(t->type == TOK_IF) {
            while(*list && ((*list)->type == TOK_ELSIF))
                addChild(t, parseToken(p, list));
            if(*list && (*list)->type == TOK_ELSE)
                addChild(t, parseToken(p, list));
        }

        /* Finally, check for proper usage */
        if(t->type != TOK_FUNC) {
            if(t->type == TOK_ELSE && t->children->type != TOK_LCURL) oops(p);
            if(t->type != TOK_ELSE && t->children->type != TOK_LPAR) oops(p);
        }
    }
    return t;
}

// True if the token's type exists in the precedence level.
static int tokInLevel(struct Token* tok, int level)
{
    int i;
    for(i=0; i<MAX_PREC_TOKS; i++)
        if(PRECEDENCE[level].toks[i] == tok->type)
            return 1;
    return 0;
}

static struct Token* parsePrecedence(struct Parser* p, struct Token* start,
                                     struct Token* end, int level);

static void precChildren(struct Parser* p, struct Token* t)
{
    struct Token* top = parsePrecedence(p, t->children, t->lastChild, 0);
    t->children = top;
    t->lastChild = top;
}

// Run a "block structure" node (if/elsif/else/for/while/foreach)
// through the precedence parser.  The funny child structure makes
// this a little more complicated than it should be.
static void precBlock(struct Parser* p, struct Token* block)
{
    struct Token* t = block->children;
    while(t) {
        if(isOpenBrace(t->type))
            precChildren(p, t);
        else if(isBlockoid(t->type))
            precBlock(p, t);
        t = t->next;
    }
}

/* Binary tokens that get empties synthesized if one side is missing */
static int oneSidedBinary(int t)
{ return t == TOK_SEMI || t ==  TOK_COMMA || t == TOK_COLON; }

static struct Token* parsePrecedence(struct Parser* p,
                                     struct Token* start, struct Token* end,
                                     int level)
{
    int rule;
    struct Token *t, *top, *left, *right;
    struct Token *a, *b, *c, *d; // temporaries

    // This is an error.  No "siblings" are allowed at the bottom level.
    if(level >= PRECEDENCE_LEVELS && start != end)
        naParseError(p, "parse error", start->line);

    // Synthesize an empty token if necessary
    if(end == 0 && start == 0)
        return newToken(p, TOK_EMPTY);

    // Sanify the list.  This is OK, since we're recursing into the
    // list structure; stuff to the left and right has already been
    // handled somewhere above.
    if(end == 0) end = start;
    if(start == 0) start = end;
    if(start->prev) start->prev->next = 0;
    if(end->next) end->next->prev = 0;
    start->prev = end->next = 0;

    // Single tokens parse as themselves.  Recurse into braces, and
    // parse children of block structure.
    if(start == end) {
        if     (isOpenBrace(start->type)) precChildren(p, start);
        else if(isBlockoid(start->type))  precBlock(p, start);
        return start;
    }

    if(oneSidedBinary(start->type)) {
        t = newToken(p, TOK_EMPTY);
        start->prev = t;
        t->next = start;
        start = t;
    }
    if(oneSidedBinary(end->type)) {
        t = newToken(p, TOK_EMPTY);
        end->next = t;
        t->prev = end;
        end = t;
    }

    // Another one: the "." and (postfix) "[]/()" operators should
    // really be the same precendence level, but the existing
    // implementation doesn't allow for it.  Bump us up a level if we
    // are parsing for DOT but find a LPAR/LBRA at the end of the
    // list.
    if(PRECEDENCE[level].toks[0] == TOK_DOT)
        if(end->type == TOK_LPAR || end->type == TOK_LBRA)
            level--;

    top = left = right = 0;
    rule = PRECEDENCE[level].rule;
    switch(rule) {
    case PREC_PREFIX:
        if(tokInLevel(start, level) && start->next) {
            a = start->children;
            b = start->lastChild;
            c = start->next;
            d = end;
            top = start;
            if(a) left = parsePrecedence(p, a, b, 0);
            right = parsePrecedence(p, c, d, level);
        }
        break;
    case PREC_SUFFIX:
        if(tokInLevel(end, level) && end->prev) {
            a = start;
            b = end->prev;
            c = end->children;
            d = end->lastChild;
            top = end;
            left = parsePrecedence(p, a, b, level);
            if(c) right = parsePrecedence(p, c, d, 0);
        }
        break;
    case PREC_BINARY:
        t = end->prev;
        while(t->prev) {
            if(tokInLevel(t, level)) {
                a = t->prev ? start : 0;
                b = t->prev;
                c = t->next;
                d = t->next ? end : 0;
                top = t;
                left = parsePrecedence(p, a, b, level);
                right = parsePrecedence(p, c, d, level+1);
                break;
            }
            t = t->prev;
        }
        break;
    case PREC_REVERSE:
        t = start->next;
        while(t->next) {
            if(tokInLevel(t, level)) {
                a = t->prev ? start : 0;
                b = t->prev;
                c = t->next;
                d = t->next ? end : 0;
                top = t;
                left = parsePrecedence(p, a, b, level+1);
                right = parsePrecedence(p, c, d, level);
                break;
            }
            t = t->next;
        }
        break;
    }

    // Found nothing, try the next level
    if(!top)
        return parsePrecedence(p, start, end, level+1);

    top->rule = rule;

    if(left) {
        left->next = right;
        left->prev = 0;
    }
    top->children = left;

    if(right) {
        right->next = 0;
        right->prev = left;
    }
    top->lastChild = right;

    top->next = top->prev = 0;
    return top;
}

naRef naParseCode(struct Context* c, naRef srcFile, int firstLine,
                  char* buf, int len, int* errLine)
{
    naRef codeObj;
    struct Token* t;
    struct Parser p;

    // Protect from garbage collection
    naTempSave(c, srcFile);

    naParseInit(&p);

    // Catch parser errors here.
    p.errLine = *errLine = 1;
    if(setjmp(p.jumpHandle)) {
        strncpy(c->error, p.err, sizeof(c->error));
        *errLine = p.errLine;
        naParseDestroy(&p);
        return naNil();
    }

    p.context = c;
    p.srcFile = srcFile;
    p.firstLine = firstLine;
    p.buf = buf;
    p.len = len;

    // Lexify, match brace structure, fixup if/for/etc...
    naLex(&p);

    // Run the block parser, make sure everything was eaten
    t = p.tree.children;
    p.tree.children = p.tree.lastChild = 0;
    parseBlock(&p, &p.tree, -1, &t);
    if(t) oops(&p);

    // Recursively run the precedence parser, and fixup the treetop
    t = parsePrecedence(&p, p.tree.children, p.tree.lastChild, 0);
    t->prev = t->next = 0;
    p.tree.children = t;
    p.tree.lastChild = t;

    // Generate code
    codeObj = naCodeGen(&p, &(p.tree), 0);

    // Clean up our mess
    naParseDestroy(&p);
    naTempSave(c, codeObj);

    return codeObj;
}
