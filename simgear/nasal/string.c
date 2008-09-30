#include <math.h>
#include <string.h>

#include "nasal.h"
#include "data.h"

// The maximum number of significant (decimal!) figures in an IEEE
// double.
#define DIGITS 16

static int tonum(unsigned char* s, int len, double* result);
static int fromnum(double val, unsigned char* s);

#define LEN(s) ((s)->emblen != -1 ? (s)->emblen : (s)->data.ref.len)
#define DATA(s) ((s)->emblen != -1 ? (s)->data.buf : (s)->data.ref.ptr)

int naStr_len(naRef s)
{
    return IS_STR(s) ? LEN(PTR(s).str) : 0;
}

char* naStr_data(naRef s)
{
    return IS_STR(s) ? (char*)DATA(PTR(s).str) : 0;
}

static void setlen(struct naStr* s, int sz)
{
    if(s->emblen == -1 && DATA(s)) naFree(s->data.ref.ptr);
    if(sz > MAX_STR_EMBLEN) {
        s->emblen = -1;
        s->data.ref.len = sz;
        s->data.ref.ptr = naAlloc(sz+1);
    } else {
        s->emblen = sz;
    }
    DATA(s)[sz] = 0; // nul terminate
}

naRef naStr_buf(naRef dst, int len)
{
    setlen(PTR(dst).str, len);
    naBZero(DATA(PTR(dst).str), len);
    return dst;
}

naRef naStr_fromdata(naRef dst, const char* data, int len)
{
    if(!IS_STR(dst)) return naNil();
    setlen(PTR(dst).str, len);
    memcpy(DATA(PTR(dst).str), data, len);
    return dst;
}

naRef naStr_concat(naRef dest, naRef s1, naRef s2)
{
    struct naStr* dst = PTR(dest).str;
    struct naStr* a = PTR(s1).str;
    struct naStr* b = PTR(s2).str;
    if(!(IS_STR(s1)&&IS_STR(s2)&&IS_STR(dest))) return naNil();
    setlen(dst, LEN(a) + LEN(b));
    memcpy(DATA(dst), DATA(a), LEN(a));
    memcpy(DATA(dst) + LEN(a), DATA(b), LEN(b));
    return dest;
}

naRef naStr_substr(naRef dest, naRef str, int start, int len)
{
    struct naStr* dst = PTR(dest).str;
    struct naStr* s = PTR(str).str;
    if(!(IS_STR(dest)&&IS_STR(str))) return naNil();
    if(start + len > LEN(s)) return naNil();
    setlen(dst, len);
    memcpy(DATA(dst), DATA(s) + start, len);
    return dest;
}

int naStr_equal(naRef s1, naRef s2)
{
    struct naStr* a = PTR(s1).str;
    struct naStr* b = PTR(s2).str;
    if(DATA(a) == DATA(b)) return 1;
    if(LEN(a) != LEN(b)) return 0;
    if(memcmp(DATA(a), DATA(b), LEN(a)) == 0) return 1;
    return 0;
}

naRef naStr_fromnum(naRef dest, double num)
{
    struct naStr* dst = PTR(dest).str;
    unsigned char buf[DIGITS+8];
    setlen(dst, fromnum(num, buf));
    memcpy(DATA(dst), buf, LEN(dst));
    return dest;
}

int naStr_parsenum(char* str, int len, double* result)
{
    return tonum((unsigned char*)str, len, result);
}

int naStr_tonum(naRef str, double* out)
{
    return tonum(DATA(PTR(str).str), LEN(PTR(str).str), out);
}

int naStr_numeric(naRef str)
{
    double dummy;
    return tonum(DATA(PTR(str).str), LEN(PTR(str).str), &dummy);
}

void naStr_gcclean(struct naStr* str)
{
    if(str->emblen == -1) naFree(str->data.ref.ptr);
    str->data.ref.ptr = 0;
    str->data.ref.len = 0;
    str->emblen = -1;
}

////////////////////////////////////////////////////////////////////////
// Below is a custom double<->string conversion library.  Why not
// simply use sprintf and atof?  Because they aren't acceptably
// platform independant, sadly.  I've seen some very strange results.
// This works the same way everywhere, although it is tied to an
// assumption of standard IEEE 64 bit floating point doubles.
//
// In practice, these routines work quite well.  Testing conversions
// of random doubles to strings and back, this routine is beaten by
// glibc on roundoff error 23% of the time, beats glibc in 10% of
// cases, and ties (usually with an error of exactly zero) the
// remaining 67%.
////////////////////////////////////////////////////////////////////////

// Reads an unsigned decimal out of the scalar starting at i, stores
// it in v, and returns the next index to start at.  Zero-length
// decimal numbers are allowed, and are returned as zero.
static int readdec(unsigned char* s, int len, int i, double* v)
{
    *v = 0;
    if(i >= len) return len;
    while(i < len && s[i] >= '0' && s[i] <= '9') {
        *v= (*v) * 10 + (s[i] - '0');
        i++;
    }
    return i;
}

// Reads a signed integer out of the string starting at i, stores it
// in v, and returns the next index to start at.  Zero-length
// decimal numbers are allowed, and are returned as zero.
static int readsigned(unsigned char* s, int len, int i, double* v)
{
    int i0 = i, i2;
    double sgn=1, val;
    if(i >= len) { *v = 0; return len; }
    if(s[i] == '+')      { i++; }
    else if(s[i] == '-') { i++; sgn = -1; }
    i2 = readdec(s, len, i, &val);
    if(i0 == i && i2 == i) {
        *v = 0;
        return i0; // don't successfully parse bare "+" or "-"
    }
    *v = sgn*val;
    return i2;
}


// Integer decimal power utility, with a tweak that enforces
// integer-exactness for arguments where that is possible.
static double decpow(int exp)
{
    double v = 1;
    int absexp;
    if(exp < 0 || exp >= DIGITS)
        return pow(10, exp);
    else
        absexp = exp < 0 ? -exp : exp;
    while(absexp--) v *= 10.0;
    return v;
}

static int tonum(unsigned char* s, int len, double* result)
{
    int i=0, fraclen=0;
    double sgn=1, val, frac=0, exp=0;

    if(len == 1 && (*s=='.' || *s=='-' || *s=='+')) return 0;

    // Strip off the leading negative sign first, so we can correctly
    // parse things like -.xxx which would otherwise confuse
    // readsigned.
    if(len > 1 && s[0] == '-' && s[1] != '-') {
        sgn = -1; s++; len--;
    }

    // Read the integer part
    i = readsigned(s, len, i, &val);
    if(val < 0) { sgn = -1; val = -val; }

    // Read the fractional part, if any
    if(i < len && s[i] == '.') {
        i++;
        fraclen = readdec(s, len, i, &frac) - i;
        i += fraclen;
    }

    // Nothing so far?  Then the parse failed.
    if(i == 0) return 0;

    // Read the exponent, if any
    if(i < len && (s[i] == 'e' || s[i] == 'E')) {
        int i0 = i+1;
        i = readsigned(s, len, i+1, &exp);
        if(i == i0) return 0; // Must have a number after the "e"
    }
    
    // compute the result
    *result = sgn * (val + frac * decpow(-fraclen)) * decpow(exp);

    // if we didn't use the whole string, return failure
    if(i < len) return 0;
    return 1;
}

// Very simple positive (!) integer print routine.  Puts the result in
// s and returns the number of characters written.  Does not null
// terminate the result.  Presumes at least a 32 bit integer, and
// cannot print integers larger than 9999999999.
static int decprint(int val, unsigned char* s)
{
    int p=1, i=0;
    if(val == 0) { *s = '0'; return 1; }
    while(p <= 999999999 && p*10 <= val) p *= 10;
    while(p > 0) {
        int count = 0;
        while(val >= p) { val -= p; count++; }
        s[i++] = '0' + count;
        p /= 10;
    }
    return i;
}

// Takes a positive (!) floating point numbers, and returns exactly
// DIGITS decimal numbers in the buffer pointed to by s, and an
// integer exponent as the return value.  For example, printing 1.0
// will result in "1000000000000000" in the buffer and -15 as the
// exponent.  The caller can then place the floating point as needed.
static int rawprint(double val, unsigned char* s)
{
    int exponent = (int)floor(log10(val));
    double mantissa = val / pow(10, exponent);
    int i, c;
    for(i=0; i<DIGITS-1; i++) {
        int digit = (int)floor(mantissa);
        s[i] = '0' + digit;
        mantissa -= digit;
        mantissa *= 10.0;
    }
    // Round (i.e. don't floor) the last digit
    c = (int)floor(mantissa);
    if(mantissa - c >= 0.5) c++;
    if(c < 0) c = 0;
    if(c > 9) c = 9;
    s[i] = '0' + c;
    return exponent - DIGITS + 1;
}

static int fromnum(double val, unsigned char* s)
{
    unsigned char raw[DIGITS];
    unsigned char* ptr = s;
    int exp, digs, i=0;

    // Handle negatives
    if(val < 0) { *ptr++ = '-'; val = -val; }

    // Exactly an integer is a special case
    if(val == (int)val) {
        ptr += decprint(val, ptr);
        *ptr = 0;
        return ptr - s;
    }

    // Get the raw digits
    exp = rawprint(val, raw);

    // Examine trailing zeros to get a significant digit count
    for(i=DIGITS-1; i>0; i--)
        if(raw[i] != '0') break;
    digs = i+1;

    if(exp > 0 || exp < -(DIGITS+3)) {
        // Standard scientific notation
        exp += DIGITS-1;
        *ptr++ = raw[0];
        if(digs > 1) {
            *ptr++ = '.';
            for(i=1; i<digs; i++) *ptr++ = raw[i];
        }
        *ptr++ = 'e';
        if(exp < 0) { exp = -exp; *ptr++ = '-'; }
        else { *ptr++ = '+'; }
        if(exp < 10) *ptr++ = '0';
        ptr += decprint(exp, ptr);
    } else if(exp < 1-DIGITS) {
        // Fraction with insignificant leading zeros
        *ptr++ = '0'; *ptr++ = '.';
        for(i=0; i<-(exp+DIGITS); i++) *ptr++ = '0';
        for(i=0; i<digs; i++) *ptr++ = raw[i];
    } else {
        // Integer part
        for(i=0; i<DIGITS+exp; i++) *ptr++ = raw[i];
        if(i < digs) {
            // Fraction, if any
            *ptr++ = '.';
            while(i<digs) *ptr++ = raw[i++];
        }
    }
    *ptr = 0;
    return ptr - s;
}
