#include "parse.h"

// Static table of recognized lexemes in the language
struct Lexeme {
    char* str;
    int   tok;
} LEXEMES[] = {
    {"and", TOK_AND},
    {"or",  TOK_OR},
    {"!",   TOK_NOT},
    {"(", TOK_LPAR},
    {")", TOK_RPAR},
    {"[", TOK_LBRA},
    {"]", TOK_RBRA},
    {"{", TOK_LCURL},
    {"}", TOK_RCURL},
    {"*", TOK_MUL},
    {"+", TOK_PLUS},
    {"-", TOK_MINUS},
    {"/", TOK_DIV},
    {"~", TOK_CAT},
    {":", TOK_COLON},
    {".", TOK_DOT},
    {",", TOK_COMMA},
    {";", TOK_SEMI},
    {"=", TOK_ASSIGN},
    {"<",  TOK_LT},
    {"<=", TOK_LTE},
    {"==", TOK_EQ},
    {"!=", TOK_NEQ},
    {">",  TOK_GT},
    {">=", TOK_GTE},
    {"nil", TOK_NIL},
    {"if",    TOK_IF},
    {"elsif", TOK_ELSIF},
    {"else",  TOK_ELSE},
    {"for",     TOK_FOR},
    {"foreach", TOK_FOREACH},
    {"while",   TOK_WHILE},
    {"return",   TOK_RETURN},
    {"break",    TOK_BREAK},
    {"continue", TOK_CONTINUE},
    {"func", TOK_FUNC}
};

// Build a table of where each line ending is
static int* findLines(struct Parser* p)
{
    char* buf = p->buf;
    int sz = p->len/10 + 16;
    int* lines = naParseAlloc(p, (sizeof(int) * sz));
    int i, j, n=0;

    for(i=0; i<p->len; i++) {
        // Not a line ending at all
        if(buf[i] != '\n' && buf[i] != '\r')
            continue;

        // Skip over the \r of a \r\n pair.
        if(buf[i] == '\r' && (i+1)<p->len && buf[i+1] == '\n') {
            i++;
            continue;
        }
        // Reallocate if necessary
        if(n == sz) {
            int* nl;
            sz *= 2;
            nl = naParseAlloc(p, sizeof(int) * sz);
            for(j=0; j<n; j++) nl[j] = lines[j];
            lines = nl;
        }
        lines[n++] = i;
    }
    p->lines = lines;
    p->nLines = n;
    return lines;
}

// What line number is the index on?
static int getLine(struct Parser* p, int index)
{
    int i;
    for(i=0; i<p->nLines; i++)
        if(p->lines[i] > index)
            return (p->firstLine-1) + i+1;
    return (p->firstLine-1) + p->nLines+1;
}

static void error(struct Parser* p, char* msg, int index)
{
    naParseError(p, msg, getLine(p, index));
}

// End index (the newline character) of the given line
static int lineEnd(struct Parser* p, int line)
{
    if(line > p->nLines) return p->len;
    return p->lines[line-1];
}

static void newToken(struct Parser* p, int pos, int type,
                     char* str, int slen, double num)
{
    struct Token* tok;

    tok = naParseAlloc(p, sizeof(struct Token));
    tok->type = type;
    tok->line = getLine(p, pos);
    tok->str = str;
    tok->strlen = slen;
    tok->num = num;
    tok->parent = &p->tree;
    tok->next = 0;
    tok->prev = p->tree.lastChild;
    tok->children = 0;
    tok->lastChild = 0;

    // Context sensitivity hack: a "-" following a binary operator of
    // higher precedence (MUL and DIV, basically) must be a unary
    // negation.  Needed to get precedence right in the parser for
    // expressiong like "a * -2"
    if(type == TOK_MINUS && tok->prev)
        if(tok->prev->type == TOK_MUL || tok->prev->type == TOK_DIV)
            tok->type = type = TOK_NEG;

    if(!p->tree.children) p->tree.children = tok;
    if(p->tree.lastChild) p->tree.lastChild->next = tok;
    p->tree.lastChild = tok;
}

// Parse a hex nibble
static int hexc(char c, struct Parser* p, int index)
{
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    error(p, "bad hex constant", index);
    return 0;
}

// Escape and returns a single backslashed expression in a single
// quoted string.  Trivial, just escape \' and leave everything else
// alone.
static void sqEscape(char* buf, int len, int index, struct Parser* p,
                     char* cOut, int* eatenOut)
{
    if(len < 2) error(p, "unterminated string", index);
    if(buf[1] == '\'') {
        *cOut = '\'';
        *eatenOut = 2;
    } else {
        *cOut = '\\';
        *eatenOut = 1;
    }
}

// Ditto, but more complicated for double quotes.
static void dqEscape(char* buf, int len, int index, struct Parser* p,
                     char* cOut, int* eatenOut)
{
    if(len < 2) error(p, "unterminated string", index);
    *eatenOut = 2;
    switch(buf[1]) {
    case '"': *cOut = '"'; break;
    case 'r': *cOut = '\r'; break;
    case 'n': *cOut = '\n'; break;
    case 't': *cOut = '\t'; break;
    case '\\': *cOut = '\\'; break;
    case 'x':
        if(len < 4) error(p, "unterminated string", index);
        *cOut = (char)((hexc(buf[2], p, index)<<4) | hexc(buf[3], p, index));
        *eatenOut = 4;
        break;
    default:
        // Unhandled, put the backslash back
        *cOut = '\\';
        *eatenOut = 1;
    }
}

// Read in a string literal
static int lexStringLiteral(struct Parser* p, int index, int singleQuote)
{
    int i, j, len, iteration;
    char* out = 0;
    char* buf = p->buf;
    char endMark = singleQuote ? '\'' : '"';

    for(iteration = 0; iteration<2; iteration++) {
        i = index+1;
        j = len = 0;
        while(i < p->len) {
            char c = buf[i];
            int eaten = 1;
            if(c == endMark)
                break;
            if(c == '\\') {
                if(singleQuote) sqEscape(buf+i, p->len-i, i, p, &c, &eaten);
                else            dqEscape(buf+i, p->len-i, i, p, &c, &eaten);
            }
            if(iteration == 1) out[j++] = c;
            i += eaten;
            len++;
        }
        // Finished stage one -- allocate the buffer for stage two
        if(iteration == 0) out = naParseAlloc(p, len);
    }
    newToken(p, index, TOK_LITERAL, out, len, 0);
    return i+1;
}

static int lexNumLiteral(struct Parser* p, int index)
{
    int len = p->len, i = index;
    unsigned char* buf = p->buf;
    double d;

    while(i<len && buf[i] >= '0' && buf[i] <= '9') i++;
    if(i<len && buf[i] == '.') {
        i++;
        while(i<len && buf[i] >= '0' && buf[i] <= '9') i++;
    }
    if(i<len && (buf[i] == 'e' || buf[i] == 'E')) {
        i++;
        if(i<len
           && (buf[i] == '-' || buf[i] == '+')
           && (i+1<len && buf[i+1] >= '0' && buf[i+1] <= '9')) i++;
        while(i<len && buf[i] >= '0' && buf[i] <= '9') i++;
    }
    naStr_parsenum(p->buf + index, i - index, &d);
    newToken(p, index, TOK_LITERAL, 0, 0, d);
    return i;
}

static int trySymbol(struct Parser* p, int start)
{
    int i = start;
    while((i < p->len) &&
          ((p->buf[i] == '_') ||
           (p->buf[i] >= 'A' && p->buf[i] <= 'Z') ||
           (p->buf[i] >= 'a' && p->buf[i] <= 'z') ||
           (p->buf[i] >= '0' && p->buf[i] <= '9')))
    { i++; }
    return i-start;
}

// Returns the length of lexeme l if the buffer prefix matches, or
// else zero.
static int matchLexeme(char* buf, int len, char* l)
{
    int i;
    for(i=0; i<len; i++) {
        if(l[i] == 0)      return i;
        if(l[i] != buf[i]) return 0;
    }
    // Ran out of buffer.  This is still OK if we're also at the end
    // of the lexeme.
    if(l[i] == 0) return i;
    return 0;
}

// This is dumb and algorithmically slow.  It would be much more
// elegant to sort and binary search the lexeme list, but that's a lot
// more code and this really isn't very slow in practice; it checks
// every byte of every lexeme for each input byte.  There are less
// than 100 bytes of lexemes in the grammar.  Returns the number of
// bytes in the lexeme read (or zero if none was recognized)
static int tryLexemes(struct Parser* p, int index, int* lexemeOut)
{
    int i, n, best, bestIndex=-1;
    char* start = p->buf + index;
    int len = p->len - index;

    n = sizeof(LEXEMES) / sizeof(struct Lexeme);
    best = 0;
    for(i=0; i<n; i++) {
        int l = matchLexeme(start, len, LEXEMES[i].str);
        if(l > best) {
            best = l;
            bestIndex = i;
        }
    }
    if(best > 0) *lexemeOut = bestIndex;
    return best;
}

void naLex(struct Parser* p)
{
    int i = 0;
    findLines(p);
    while(i<p->len) {
        char c = p->buf[i];

        // Whitespace, comments and string literals have obvious
        // markers and can be handled by a switch:
        int handled = 1;
        switch(c) {
        case ' ': case '\t': case '\n': case '\r': case '\f': case '\v':
            i++;
            break;
        case '#':
            i = lineEnd(p, getLine(p, i));
            break;
        case '\'': case '"':
            i = lexStringLiteral(p, i, (c=='"' ? 0 : 1));
            break;
        default:
            if(c >= '0' && c <= '9') i = lexNumLiteral(p, i);
            else                     handled = 0;
        }

        // Lexemes and symbols are a little more complicated.  Pick
        // the longest one that matches.  Since some lexemes look like
        // symbols (e.g. "or") they need a higher precedence, but we
        // don't want a lexeme match to clobber the beginning of a
        // symbol (e.g. "orchid").  If neither match, we have a bad
        // character in the mix.
        if(!handled) {
            int symlen=0, lexlen=0, lexeme;
            lexlen = tryLexemes(p, i, &lexeme);
            if((c>='A' && c<='Z') || (c>='a' && c<='z') || (c=='_'))
                symlen = trySymbol(p, i);
            if(lexlen && lexlen >= symlen) {
                newToken(p, i, LEXEMES[lexeme].tok, 0, 0, 0);
                i += lexlen;
            } else if(symlen) {
                newToken(p, i, TOK_SYMBOL, p->buf+i, symlen, 0);
                i += symlen;
            } else {
                error(p, "illegal character", i);
            }
        }
    }
}
