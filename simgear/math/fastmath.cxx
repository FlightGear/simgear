/*
 * \file fastmath.cxx
 * fast mathematics routines.
 *
 * Refferences:
 *
 * A Fast, Compact Approximation of the Exponential Function
 * Nicol N. Schraudolph
 * IDSIA, Lugano, Switzerland
 * http://www.inf.ethz.ch/~schraudo/pubs/exp.pdf
 *
 * Base-2 exp, Laurent de Soras
 * http://www.musicdsp.org/archive.php?classid=5#106
 *
 * Fast log() Function, by Laurent de Soras:
 * http://www.flipcode.com/cgi-bin/msg.cgi?showThread=Tip-Fastlogfunction&forum=totd&id=-1
 *
 * Sin, Cos, Tan approximation
 * http://www.musicdsp.org/showArchiveComment.php?ArchiveID=115
 *
 * fast floating point power computation:
 * http://playstation2-linux.com/download/adam/power.c
 */

/*
 * $Id$
 */


#include <simgear/constants.h>

#include "fastmath.hxx"

/**
 * This function is on avarage 9 times faster than the system exp() function
 * and has an error of about 1.5%
 */
static union {
    double d;
    struct {
#if BYTE_ORDER == BIG_ENDIAN
        int i, j;
#else
        int j, i;
#endif
    } n;
} _eco;

double fast_exp(double val) {
    const double a = 1048576/M_LN2;
    const double b_c = 1072632447; /* 1072693248 - 60801 */

    _eco.n.i = (int)(a*val + b_c);

    return _eco.d;
}

/*
 * Linear approx. between 2 integer values of val. Uses 32-bit integers.
 * Not very efficient but faster than exp()
 */
double fast_exp2( const double val )
{
    int e;
    double ret;

    if (val >= 0) {
        e = int (val);
        ret = val - (e - 1);

#if BYTE_ORDER == BIG_ENDIAN
        ((*((int *) &ret)) &= ~(2047 << 20)) += (e + 1023) << 20;
#else
        ((*(1 + (int *) &ret)) &= ~(2047 << 20)) += (e + 1023) << 20;
#endif
    } else {
        e = int (val + 1023);
        ret = val - (e - 1024);

#if BYTE_ORDER == BIG_ENDIAN
        ((*((int *) &ret)) &= ~(2047 << 20)) += e << 20;
#else
        ((*(1 + (int *) &ret)) &= ~(2047 << 20)) += e << 20;
#endif
    }

    return ret;
}


/*
 *
 */
float _fast_log2(const float val)
{
    float result, tmp;
    float mp = 0.346607f;

    result = *(int*)&val;
    result *= 1.0/(1<<23);
    result = result - 127;
 
    tmp = result - floor(result);
    tmp = (tmp - tmp*tmp) * mp;
    return tmp + result;
}

float _fast_pow2(const float val)
{
    float result;

    float mp = 0.33971f;
    float tmp = val - floor(val);
    tmp = (tmp - tmp*tmp) * mp;

    result = val + 127 - tmp; 
    result *= (1<<23);
    *(int*)&result = (int)result;
    return result;
}



/**
 * While we're on the subject, someone might have use for these as well?
 * Float Shift Left and Float Shift Right. Do what you want with this.
 */
void fast_BSL(float &x, register unsigned long shiftAmount) {

	*(unsigned long*)&x+=shiftAmount<<23;

}

void fast_BSR(float &x, register unsigned long shiftAmount) {

	*(unsigned long*)&x-=shiftAmount<<23;

}


/*
 * fastpow(f,n) gives a rather *rough* estimate of a float number f to the
 * power of an integer number n (y=f^n). It is fast but result can be quite a
 * bit off, since we directly mess with the floating point exponent.
 * 
 * Use it only for getting rough estimates of the values and where precision 
 * is not that important.
 */
float fast_pow(const float f, const int n)
{
    long *lp,l;
    lp=(long*)(&f);
    l=*lp;l-=0x3F800000l;l<<=(n-1);l+=0x3F800000l;
    *lp=l;
    return f;
}

float fast_root(const float f, const int n)
{
    long *lp,l;
    lp=(long*)(&f);
    l=*lp;l-=0x3F800000l;l>>=(n-1);l+=0x3F800000l;
    *lp=l;
    return f;
}


/*
 * Code for approximation of cos, sin, tan and inv sin, etc.
 * Surprisingly accurate and very usable.
 *
 * Domain:
 * Sin/Cos    [0, pi/2]
 * Tan        [0,pi/4]
 * InvSin/Cos [0, 1]
 * InvTan     [-1, 1]
 */

float fast_sin(const float val)
{
    float fASqr = val*val;
    float fResult = -2.39e-08f;
    fResult *= fASqr;
    fResult += 2.7526e-06f;
    fResult *= fASqr;
    fResult -= 1.98409e-04f;
    fResult *= fASqr;
    fResult += 8.3333315e-03f;
    fResult *= fASqr;
    fResult -= 1.666666664e-01f;
    fResult *= fASqr;
    fResult += 1.0f;
    fResult *= val;

    return fResult;
}

float fast_cos(const float val)
{
    float fASqr = val*val;
    float fResult = -2.605e-07f;
    fResult *= fASqr;
    fResult += 2.47609e-05f;
    fResult *= fASqr;
    fResult -= 1.3888397e-03f;
    fResult *= fASqr;
    fResult += 4.16666418e-02f;
    fResult *= fASqr;
    fResult -= 4.999999963e-01f;
    fResult *= fASqr;
    fResult += 1.0f;

    return fResult;  
}

float fast_tan(const float val)
{
    float fASqr = val*val;
    float fResult = 9.5168091e-03f;
    fResult *= fASqr;
    fResult += 2.900525e-03f;
    fResult *= fASqr;
    fResult += 2.45650893e-02f;
    fResult *= fASqr;
    fResult += 5.33740603e-02f;
    fResult *= fASqr;
    fResult += 1.333923995e-01f;
    fResult *= fASqr;
    fResult += 3.333314036e-01f;
    fResult *= fASqr;
    fResult += 1.0f;
    fResult *= val;

    return fResult;

}

float fast_asin(float val)
{
    float fRoot = sqrt(1.0f-val);
    float fResult = -0.0187293f;
    fResult *= val;
    fResult += 0.0742610f;
    fResult *= val;
    fResult -= 0.2121144f;
    fResult *= val;
    fResult += 1.5707288f;
    fResult = SGD_PI_2 - fRoot*fResult;

    return fResult;
}

float fast_acos(float val)
{
    float fRoot = sqrt(1.0f-val);
    float fResult = -0.0187293f;
    fResult *= val;
    fResult += 0.0742610f;
    fResult *= val;
    fResult -= 0.2121144f;
    fResult *= val;
    fResult += 1.5707288f;
    fResult *= fRoot;

    return fResult;
}

float fast_atan(float val)
{
    float fVSqr = val*val;
    float fResult = 0.0028662257f;
    fResult *= fVSqr;
    fResult -= 0.0161657367f;
    fResult *= fVSqr;
    fResult += 0.0429096138f;
    fResult *= fVSqr;
    fResult -= 0.0752896400f;
    fResult *= fVSqr;
    fResult += 0.1065626393f;
    fResult *= fVSqr;
    fResult -= 0.1420889944f;
    fResult *= fVSqr;
    fResult += 0.1999355085f;
    fResult *= fVSqr;
    fResult -= 0.3333314528f;
    fResult *= fVSqr;
    fResult += 1.0f;
    fResult *= val;

    return fResult;
}
