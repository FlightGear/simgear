#ifndef _PARSE_H
#define _PARSE_H

#include <setjmp.h>

#include "nasal.h"
#include "data.h"
#include "code.h"

enum {
    TOK_TOP=1, TOK_AND, TOK_OR, TOK_NOT, TOK_LPAR, TOK_RPAR, TOK_LBRA,
    TOK_RBRA, TOK_LCURL, TOK_RCURL, TOK_MUL, TOK_PLUS, TOK_MINUS, TOK_NEG,
    TOK_DIV, TOK_CAT, TOK_COLON, TOK_DOT, TOK_COMMA, TOK_SEMI,
    TOK_ASSIGN, TOK_LT, TOK_LTE, TOK_EQ, TOK_NEQ, TOK_GT, TOK_GTE,
    TOK_IF, TOK_ELSIF, TOK_ELSE, TOK_FOR, TOK_FOREACH, TOK_WHILE,
    TOK_RETURN, TOK_BREAK, TOK_CONTINUE, TOK_FUNC, TOK_SYMBOL,
    TOK_LITERAL, TOK_EMPTY, TOK_NIL, TOK_ELLIPSIS, TOK_QUESTION, TOK_VAR
};

struct Token {
    int type;
    int line;
    char* str;
    int strlen;
    double num;
    struct Token* parent;
    struct Token* next;
    struct Token* prev;
    struct Token* children;
    struct Token* lastChild;
};

struct Parser {
    // Handle to the Nasal interpreter
    struct Context* context;

    char* err;
    int errLine;
    jmp_buf jumpHandle;

    // The parse tree ubernode
    struct Token tree;

    // The input buffer
    char* buf;
    int   len;

    // Input file parameters (for generating pretty stack dumps)
    naRef srcFile;
    int firstLine;

    // Chunk allocator.  Throw away after parsing.
    void** chunks;
    int* chunkSizes;
    int nChunks;
    int leftInChunk;

    // Computed line number table for the lexer
    int* lines;
    int  nLines;
    
    struct CodeGenerator* cg;
};

struct CodeGenerator {
    int lastLine;

    // Accumulated byte code array
    unsigned short* byteCode;
    int codesz;
    int codeAlloced;

    // Inst. -> line table, stores pairs of {ip, line}
    unsigned short* lineIps;
    int nLineIps; // number of pairs
    int nextLineIp;

    // Stack of "loop" frames for break/continue statements
    struct {
        int breakIP;
        int contIP;
        struct Token* label;
    } loops[MAX_MARK_DEPTH];
    int loopTop;

    // Dynamic storage for constants, to be compiled into a static table
    naRef consts;
};

void naParseError(struct Parser* p, char* msg, int line);
void naParseInit(struct Parser* p);
void* naParseAlloc(struct Parser* p, int bytes);
void naParseDestroy(struct Parser* p);
void naLex(struct Parser* p);
naRef naCodeGen(struct Parser* p, struct Token* block, struct Token* arglist);

void naParse(struct Parser* p);



#endif // _PARSE_H
