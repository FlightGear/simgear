#include <setjmp.h>
#include <string.h>

#include "parse.h"

// Static precedence table, from low (loose binding, do first) to high
// (tight binding, do last).
#define MAX_PREC_TOKS 6
struct precedence {
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
    // Some errors (e.g. code generation of a null pointer) lack a
    // line number, so we throw -1 and set the line earlier.
    if(line > 0) p->errLine = line;
    p->err = msg;
    longjmp(p->jumpHandle, 1);
}

// A "generic" (too obfuscated to describe) parser error
static void oops(struct Parser* p, struct Token* t)
{
    naParseError(p, "parse error", t->line);
}

void naParseInit(struct Parser* p)
{
    p->buf = 0;
    p->len = 0;
    p->lines = 0;
    p->nLines = 0;
    p->chunks = 0;
    p->chunkSizes = 0;
    p->nChunks = 0;
    p->leftInChunk = 0;
    p->cg = 0;

    p->tree.type = TOK_TOP;
    p->tree.line = 1;
    p->tree.str = 0;
    p->tree.strlen = 0;
    p->tree.num = 0;
    p->tree.next = 0;
    p->tree.prev = 0;
    p->tree.children = 0;
    p->tree.lastChild = 0;
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

    // Round up to 8 byte chunks for alignment
    if(bytes & 0x7) bytes = ((bytes>>3) + 1) << 3;
    
    // Need a new chunk?
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
    return (void*)result;
}

// Remove the child from the list where it exists, and insert it at
// the end of the parents child list.
static void addNewChild(struct Token* p, struct Token* c)
{
    if(c->prev) c->prev->next = c->next;
    if(c->next) c->next->prev = c->prev;
    if(c == c->parent->children)
        c->parent->children = c->next;
    if(c == c->parent->lastChild)
        c->parent->lastChild = c->prev;
    c->parent = p;
    c->next = 0;
    c->prev = p->lastChild;
    if(p->lastChild) p->lastChild->next = c;
    if(!p->children) p->children = c;
    p->lastChild = c;
}

// Follows the token list from start (which must be a left brace of
// some type), placing all tokens found into start's child list until
// it reaches the matching close brace.
static void collectBrace(struct Parser* p, struct Token* start)
{
    struct Token* t;
    int closer = -1;
    if(start->type == TOK_LPAR)  closer = TOK_RPAR;
    if(start->type == TOK_LBRA)  closer = TOK_RBRA;
    if(start->type == TOK_LCURL) closer = TOK_RCURL;

    t = start->next;
    while(t) {
        struct Token* next;
        switch(t->type) {
        case TOK_LPAR: case TOK_LBRA: case TOK_LCURL:
            collectBrace(p, t);
            break;
        case TOK_RPAR: case TOK_RBRA: case TOK_RCURL:
            if(t->type != closer)
                naParseError(p, "mismatched closing brace", t->line);

            // Drop this node on the floor, stitch up the list and return
            if(start->parent->lastChild == t)
                start->parent->lastChild = t->prev;
            start->next = t->next;
            if(t->next) t->next->prev = start;
            return;
        }
        // Snip t out of the existing list, and append it to start's
        // children.
        next = t->next;
        addNewChild(start, t);
        t = next;
    }
    naParseError(p, "unterminated brace", start->line);
}

// Recursively find the contents of all matching brace pairs in the
// token list and turn them into children of the left token.  The
// right token disappears.
static void braceMatch(struct Parser* p, struct Token* start)
{
    struct Token* t = start;
    while(t) {
        switch(t->type) {
        case TOK_LPAR: case TOK_LBRA: case TOK_LCURL:
            collectBrace(p, t);
            break;
        case TOK_RPAR: case TOK_RBRA: case TOK_RCURL:
            if(start->type != TOK_LBRA)
                naParseError(p, "stray closing brace", t->line);
            break;
        }
        t = t->next;
    }
}

// Allocate and return an "empty" token as a parsing placeholder.
static struct Token* emptyToken(struct Parser* p)
{
    struct Token* t = naParseAlloc(p, sizeof(struct Token));
    t->type = TOK_EMPTY;
    t->line = -1;
    t->strlen = 0;
    t->num = 0;
    t->str = 0;
    t->next = t->prev = t->children = t->lastChild = 0;
    t->parent = 0;
    return t;
}

// Synthesize a curly brace token to wrap token t foward to the end of
// "statement".  FIXME: unify this with the addNewChild(), which does
// very similar stuff.
static void embrace(struct Parser* p, struct Token* t)
{
    struct Token *b, *end = t;
    if(!t) return;
    while(end->next) {
        if(end->next->type == TOK_SEMI) {
            // Slurp up the semi, iff it is followed by an else/elsif,
            // otherwise leave it in place.
            if(end->next->next) {
                if(end->next->next->type == TOK_ELSE)  end = end->next;
                if(end->next->next->type == TOK_ELSIF) end = end->next;
            }
            break;
        }
        if(end->next->type == TOK_COMMA) break;
        if(end->next->type == TOK_ELSE) break;
        if(end->next->type == TOK_ELSIF) break;
        end = end->next;
    }
    b = emptyToken(p);
    b->type = TOK_LCURL;
    b->line = t->line;
    b->parent = t->parent;
    b->prev = t->prev;
    b->next = end->next;
    b->children = t;
    b->lastChild = end;
    if(t->prev) t->prev->next = b;
    else b->parent->children = b;
    if(end->next) end->next->prev = b;
    else b->parent->lastChild = b;
    t->prev = 0;
    end->next = 0;
    for(; t; t = t->next)
        t->parent = b;
}

#define NEXT(t) (t ? t->next : 0)
#define TYPE(t) (t ? t->type : -1)

static void fixBracelessBlocks(struct Parser* p, struct Token* t)
{
    // Find the end, and march *backward*
    while(t && t->next) t = t->next;
    for(/**/; t; t=t->prev) {
        switch(t->type) {
        case TOK_FOR: case TOK_FOREACH: case TOK_FORINDEX: case TOK_WHILE:
        case TOK_IF: case TOK_ELSIF:
            if(TYPE(NEXT(t)) == TOK_LPAR && TYPE(NEXT(NEXT(t))) != TOK_LCURL)
                    embrace(p, t->next->next);
            break;
        case TOK_ELSE:
            if(TYPE(NEXT(t)) != TOK_LCURL)
                embrace(p, t->next);
            break;
        case TOK_FUNC:
            if(TYPE(NEXT(t)) == TOK_LPAR) {
                if(TYPE(NEXT(NEXT(t))) != TOK_LCURL)
                    embrace(p, NEXT(NEXT(t)));
            } else if(TYPE(NEXT(t)) != TOK_LCURL)
                embrace(p, t->next);
            break;
        default:
            break;
        }
    }
}

// Fixes up parenting for obvious parsing situations, like code blocks
// being the child of a func keyword, etc...
static void fixBlockStructure(struct Parser* p, struct Token* start)
{
    struct Token *t, *c;
    fixBracelessBlocks(p, start);
    t = start;
    while(t) {
        switch(t->type) {
        case TOK_FUNC:
            // Slurp an optional paren block containing an arglist, then
            // fall through to parse the curlies...
            if(t->next && t->next->type == TOK_LPAR) {
                c = t->next;
                addNewChild(t, c);
                fixBlockStructure(p, c);
            }
        case TOK_ELSE: // and TOK_FUNC!
            // These guys precede a single curly block
            if(!t->next || t->next->type != TOK_LCURL) oops(p, t);
            c = t->next;
            addNewChild(t, c);
            fixBlockStructure(p, c);
            break;
        case TOK_FOR: case TOK_FOREACH: case TOK_FORINDEX: case TOK_WHILE:
        case TOK_IF: case TOK_ELSIF:
            // Expect a paren and then a curly
            if(!t->next || t->next->type != TOK_LPAR) oops(p, t);
            c = t->next;
            addNewChild(t, c);
            fixBlockStructure(p, c);

            if(!t->next || t->next->type != TOK_LCURL) oops(p, t);
            c = t->next;
            addNewChild(t, c);
            fixBlockStructure(p, c);
            break;
        case TOK_LPAR: case TOK_LBRA: case TOK_LCURL:
            fixBlockStructure(p, t->children);
            break;
        }
        t = t->next;
    }

    // Another pass to hook up the elsif/else chains.
    t = start;
    while(t) {
        if(t->type == TOK_IF) {
            while(t->next && t->next->type == TOK_ELSIF)
                addNewChild(t, t->next);
            if(t->next && t->next->type == TOK_ELSE)
                addNewChild(t, t->next);
        }
        t = t->next;
    }

    // And a final one to add semicolons.  Always add one after
    // for/foreach/while expressions.  Add one after a function lambda
    // if it immediately follows an assignment, and add one after an
    // if/elsif/else if it is the first token in an expression list
    // (i.e has no previous token, or is preceded by a ';' or '{').
    // This mimicks common usage and avoids a conspicuous difference
    // between this grammar and more common languages.  It can be
    // "escaped" with extra parenthesis if necessary, e.g.:
    //      a = (func { join(" ", arg) })(1, 2, 3, 4);
    t = start;
    while(t) {
        int addSemi = 0;
        switch(t->type) {
        case TOK_IF:
            if(!t->prev
               || t->prev->type == TOK_SEMI
               || t->prev->type == TOK_LCURL)
                addSemi = 1;
            break;
        case TOK_FOR: case TOK_FOREACH: case TOK_FORINDEX: case TOK_WHILE:
            addSemi = 1;
            break;
        case TOK_FUNC:
            if(t->prev && t->prev->type == TOK_ASSIGN)
                addSemi = 1;
            break;
        }
        if(!t->next || t->next->type == TOK_SEMI || t->next->type == TOK_COMMA)
            addSemi = 0; // don't bother, no need
        if(addSemi) {
            struct Token* semi = emptyToken(p);
            semi->type = TOK_SEMI;
            semi->line = t->line;
            semi->next = t->next;
            semi->prev = t;
            semi->parent = t->parent;
            if(semi->next) semi->next->prev = semi;
            else semi->parent->lastChild = semi;
            t->next = semi;
            t = semi; // don't bother checking the new one
        }
        t = t->next;
    }
    
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

static int isBrace(int type)
{
    return type == TOK_LPAR || type == TOK_LBRA || type == TOK_LCURL;
}

static int isBlock(int t)
{
    return t == TOK_IF  || t == TOK_ELSIF   || t == TOK_ELSE
        || t == TOK_FOR || t == TOK_FOREACH || t == TOK_WHILE
        || t == TOK_FUNC || t == TOK_FORINDEX;
}

static void precChildren(struct Parser* p, struct Token* t);
static void precBlock(struct Parser* p, struct Token* t);

static struct Token* parsePrecedence(struct Parser* p,
                                     struct Token* start, struct Token* end,
                                     int level)
{
    int rule;
    struct Token *t, *top, *left, *right;
    struct Token *a, *b, *c, *d; // temporaries

    // This is an error.  No "siblings" are allowed at the bottom level.
    if(level >= PRECEDENCE_LEVELS && start != end)
        oops(p, start);

    // Synthesize an empty token if necessary
    if(end == 0 && start == 0)
        return emptyToken(p);

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
        if(isBrace(start->type)) {
            precChildren(p, start);
        } else if(isBlock(start->type)) {
            precBlock(p, start);
        }
        return start;
    }

    // A context-sensitivity: we want to parse ';' and ',' as binary
    // operators, but want them to be legal at the beginning and end
    // of a list (unlike, say, '+' where we want a parse error).
    // Generate empties as necessary.
    if(start->type == TOK_SEMI || start->type == TOK_COMMA) {
        t = emptyToken(p);
        start->prev = t;
        t->next = start;
        start = t;
    }
    if(end->type == TOK_SEMI || end->type == TOK_COMMA) {
        t = emptyToken(p);
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
        left->parent = top;
    }
    top->children = left;

    if(right) {
        right->next = 0;
        right->prev = left;
        right->parent = top;
    }
    top->lastChild = right;

    top->next = top->prev = 0;
    return top;
}

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
        if(isBrace(t->type))
            precChildren(p, t);
        else if(isBlock(t->type))
            precBlock(p, t);
        t = t->next;
    }
}

naRef naParseCode(struct Context* c, naRef srcFile, int firstLine,
                  char* buf, int len, int* errLine)
{
    naRef codeObj;
    struct Token* t;
    struct Parser p;

    // Protect from garbage collection
    naTempSave(c, srcFile);

    // Catch parser errors here.
    *errLine = 0;
    if(setjmp(p.jumpHandle)) {
        strncpy(c->error, p.err, sizeof(c->error));
        *errLine = p.errLine;
        return naNil();
    }

    naParseInit(&p);
    p.context = c;
    p.srcFile = srcFile;
    p.firstLine = firstLine;
    p.buf = buf;
    p.len = len;

    // Lexify, match brace structure, fixup if/for/etc...
    naLex(&p);
    braceMatch(&p, p.tree.children);
    fixBlockStructure(&p, p.tree.children);

    // Recursively run the precedence parser, and fixup the treetop
    t = parsePrecedence(&p, p.tree.children, p.tree.lastChild, 0);
    t->prev = t->next = 0;
    p.tree.children = t;
    p.tree.lastChild = t;

    // Generate code!
    codeObj = naCodeGen(&p, &(p.tree), 0);

    // Clean up our mess
    naParseDestroy(&p);
    naTempSave(c, codeObj);

    return codeObj;
}
