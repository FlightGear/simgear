#include <string.h>
#include "nasal.h"
#include "parse.h"

// bytes required to store a given character
static int cbytes(unsigned int c)
{
    static const int NB[] = { 0x7f, 0x07ff, 0xffff, 0x001fffff, 0x03ffffff };
    int i;
    for(i=0; i<(sizeof(NB)/sizeof(NB[0])) && c>NB[i]; i++) {}
    return i+1;
}

// Returns a byte with the N high order bits set
#define TOPBITS(n) ((unsigned char)(((signed char)0x80)>>((n)-1)))

// write a utf8 character, return bytes written or zero on error
static int writec(unsigned int c, unsigned char* s, int len)
{
    int i, n = cbytes(c);
    if(len < n) return 0;
    for(i=n-1; i>0; i--) {
        s[i] = 0x80 | (c & 0x3f);
        c >>= 6;
    }
    s[0] = (n > 1 ? TOPBITS(n) : 0) | c;
    return n;
}

// read a utf8 character, or -1 on error.
static int readc(unsigned char* s, int len, int* used)
{
    int n, i, c;
    if(!len) return -1;
    if(s[0] < 0x80) { *used = 1; return s[0]; }
    for(n=2; n<7; n++)
        if((s[0] & TOPBITS(n+1)) == TOPBITS(n))
            break;
    if(len < n || n > 6) return -1;
    c = s[0] & (~TOPBITS(n+1));
    for(i=1; i<n; i++) {
        if((s[i] >> 6) != 2) return -1;
        c = (c << 6) | (s[i] & 0x3f);
    }
    if(n != cbytes(c)) return -1;
    *used = n;
    return c;
}

/* Public symbol used by the parser */
int naLexUtf8C(char* s, int len, int* used)
{ return readc((void*)s, len, used); }

static unsigned char* nthchar(unsigned char* s, int n, int* len)
{
    int i, bytes;
    for(i=0; *len && i<n; i++) {
        if(readc(s, *len, &bytes) < 0) return 0;
        s += bytes; *len -= bytes;
    }
    return s;
}

static naRef f_chstr(naContext ctx, naRef me, int argc, naRef* args)
{
    int n;
    naRef ch;
    unsigned char buf[6];
    if(argc < 1 || naIsNil(ch=naNumValue(args[0])))
        naRuntimeError(ctx, "bad/missing argument to utf8.chstr");
    n = writec((int)ch.num, buf, sizeof(buf));
    return naStr_fromdata(naNewString(ctx), (void*)buf, n);
}

static naRef f_size(naContext c, naRef me, int argc, naRef* args)
{
    unsigned char* s;
    int sz=0, n=0, len;
    if(argc < 1 || !naIsString(args[0]))
        naRuntimeError(c, "bad/missing argument to utf8.strc");
    s = (void*)naStr_data(args[0]);
    len = naStr_len(args[0]);
    while(len > 0) {
        if(readc(s, len, &n) < 0)
            naRuntimeError(c, "utf8 encoding error in utf8.size");
        sz++; len -= n; s += n;
    }
    return naNum(sz);
}

static naRef f_strc(naContext ctx, naRef me, int argc, naRef* args)
{
    naRef idx;
    unsigned char* s;
    int len, c=0, bytes;
    if(argc < 2 || !naIsString(args[0]) || naIsNil(idx=naNumValue(args[1])))
        naRuntimeError(ctx, "bad/missing argument to utf8.strc");
    len = naStr_len(args[0]);
    s = nthchar((void*)naStr_data(args[0]), (int)idx.num, &len);
    if(!s || (c = readc(s, len, &bytes)) < 0)
        naRuntimeError(ctx, "utf8 encoding error in utf8.strc");
    return naNum(c);
}

static naRef f_substr(naContext c, naRef me, int argc, naRef* args)
{
    naRef start, end;
    int len;
    unsigned char *s, *s2;
    end = argc > 2 ? naNumValue(args[2]) : naNil();
    if((argc < 2 || !naIsString(args[0]) || naIsNil(start=naNumValue(args[1])))
       || (argc > 2 && naIsNil(end)))
        naRuntimeError(c, "bad/missing argument to utf8.substr");
    len = naStr_len(args[0]);
    if(!(s = nthchar((void*)naStr_data(args[0]), (int)start.num, &len)))
        naRuntimeError(c, "start index overrun in utf8.substr");
    if(!naIsNil(end)) {
        if(!(s2 = nthchar(s, (int)end.num, &len)))
            naRuntimeError(c, "end index overrun in utf8.substr");
        len = (int)(s2-s);
    }
    return naStr_fromdata(naNewString(c), (void*)s, len);
}

static naRef f_validate(naContext c, naRef me, int argc, naRef* args)
{
    naRef result, unkc=naNil();
    int len, len2, lenout=0, n;
    unsigned char *s, *s2, *buf;
    if(argc < 1 || !naIsString(args[0]) ||
       (argc > 1 && naIsNil(unkc=naNumValue(args[1]))))
        naRuntimeError(c, "bad/missing argument to utf8.strc");
    if(naIsNil(unkc)) unkc = naNum('?');
    len = naStr_len(args[0]);
    s = (void*)naStr_data(args[0]);
    len2 = 6*len; // max for ridiculous unkc values
    s2 = buf = naAlloc(len2);
    while(len > 0) {
        int c = readc(s, len, &n);
        if(c < 0) { c = (int)unkc.num; n = 1; }
        s += n; len -= n;
        n = writec(c, s2, len2);
        s2 += n; len2 -= n; lenout += n;
    }
    result = naStr_fromdata(naNewString(c), (char*)buf, lenout);
    naFree(buf);
    return result;
}

static naCFuncItem funcs[] = {
    { "chstr", f_chstr },
    { "strc", f_strc },
    { "substr", f_substr },
    { "size", f_size },
    { "validate", f_validate },
    { 0 }
};

naRef naInit_utf8(naContext c)
{
    return naGenLib(c, funcs);
}
