#include <string.h>
#include "data.h"

// Note that this currently supports a maximum field width of 32
// bits (i.e. an unsigned int).  Using a 64 bit integer would stretch
// that beyond what is representable in the double result, but
// requires portability work.
#define MSK(n) (1 << (7 - ((n) & 7)))
#define BIT(s,l,n) s[(n)>>3] & MSK(n)
#define CLRB(s,l,n) s[(n)>>3] &= ~MSK(n)
#define SETB(s,l,n) s[(n)>>3] |= MSK(n)

static unsigned int fld(naContext c, unsigned char* s,
                        int slen, int bit, int flen)
{
    int i;
    unsigned int fld = 0;
    if(bit + flen > 8*slen) naRuntimeError(c, "bitfield out of bounds");
    for(i=0; i<flen; i++) if(BIT(s, slen, bit+flen-i-1)) fld |= (1<<i);
    return fld;
}

static void setfld(naContext c, unsigned char* s, int slen,
                   int bit, int flen, unsigned int fld)
{
    int i;
    if(bit + flen > 8*slen) naRuntimeError(c, "bitfield out of bounds");
    for(i=0; i<flen; i++)
        if(fld & (1<<i)) SETB(s, slen, i+bit);
        else CLRB(s, slen, i+bit);
}

static naRef dofld(naContext c, int argc, naRef* args, int sign)
{
    naRef s = argc > 0 ? args[0] : naNil();
    int bit = argc > 1 ? (int)naNumValue(args[1]).num : -1;
    int len = argc > 2 ? (int)naNumValue(args[2]).num : -1;
    unsigned int f;
    if(!naIsString(s) || !MUTABLE(args[0]) || bit < 0 || len < 0)
        naRuntimeError(c, "missing/bad argument to fld/sfld");
    f = fld(c, (void*)naStr_data(s), naStr_len(s), bit, len);
    if(!sign) return naNum(f);
    if(f & (1 << (len-1))) f |= ~((1<<len)-1); // sign extend
    return naNum((signed int)f);
}

static naRef f_sfld(naContext c, naRef me, int argc, naRef* args)
{
    return dofld(c, argc, args, 1);
}

static naRef f_fld(naContext c, naRef me, int argc, naRef* args)
{
    return dofld(c, argc, args, 0);
}

static naRef f_setfld(naContext c, naRef me, int argc, naRef* args)
{
    naRef s = argc > 0 ? args[0] : naNil();
    int bit = argc > 1 ? (int)naNumValue(args[1]).num : -1;
    int len = argc > 2 ? (int)naNumValue(args[2]).num : -1;
    naRef val = argc > 3 ? naNumValue(args[3]) : naNil();
    if(!argc || !MUTABLE(args[0])|| bit < 0 || len < 0 || IS_NIL(val))
        naRuntimeError(c, "missing/bad argument to setfld");
    setfld(c, (void*)naStr_data(s), naStr_len(s), bit, len, (unsigned int)val.num);
    return naNil();
}

static naRef f_buf(naContext c, naRef me, int argc, naRef* args)
{
    naRef len = argc ? naNumValue(args[0]) : naNil();
    if(IS_NIL(len)) naRuntimeError(c, "missing/bad argument to buf");
    return naStr_buf(naNewString(c), (int)len.num);
}

static naCFuncItem funcs[] = {
    { "sfld", f_sfld },
    { "fld", f_fld },
    { "setfld", f_setfld },
    { "buf", f_buf },
    { 0 }
};

naRef naInit_bits(naContext c)
{
    return naGenLib(c, funcs);
}
