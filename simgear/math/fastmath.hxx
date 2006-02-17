/*
 * \file fastmath.hxx
 * fast mathematics routines.
 *
 * Refferences:
 *
 * A Fast, Compact Approximation of the Exponential Function
 * Nicol N. Schraudolph
 * IDSIA, Lugano, Switzerland
 * http://www.inf.ethz.ch/~schraudo/pubs/exp.pdf
 *
 * Fast log() Function, by Laurent de Soras:
 * http://www.flipcode.com/cgi-bin/msg.cgi?showThread=Tip-Fastlogfunction&forum=totd&id=-1
 *
 */

/*
 * $Id$
 */

#ifndef _SG_FMATH_HXX
#define _SG_FMATH_HXX 1

#ifndef __cplusplus
# error This library requires C++
#endif

#include <math.h>


double fast_exp(double val);
double fast_exp2(const double val);

float fast_pow(const float val1, const float val2);
float fast_log2(const float cal);
float fast_root(const float f, const int n);

float _fast_pow2(const float cal);
float _fast_log2(const float val);

float fast_sin(const float val);
float fast_cos(const float val);
float fast_tan(const float val);
float fast_asin(const float val);
float fast_acos(const float val);
float fast_atan(const float val);

void fast_BSL(float &x, register unsigned long shiftAmount);
void fast_BSR(float &x, register unsigned long shiftAmount);


inline float fast_log2 (float val)
{
    int * const  exp_ptr = reinterpret_cast <int *> (&val);
    int          x = *exp_ptr;
    const int    log_2 = ((x >> 23) & 255) - 128;
    x &= ~(255 << 23);
    x += 127 << 23;
    *exp_ptr = x;

    val = ((-1.0f/3) * val + 2) * val - 2.0f/3;   // (1)

    return (val + log_2);
}


/**
 * This function is about 3 times faster than the system log() function
 * and has an error of about 0.01%
 */
inline float fast_log (const float &val)
{
   return (fast_log2 (val) * 0.69314718f);
}

inline float fast_log10 (const float &val)
{
   return (fast_log2(val) / 3.321928095f);
}


/**
 * This function is about twice as fast as the system pow(x,y) function
 */
inline float fast_pow(const float val1, const float val2)
{
   return _fast_pow2(val2 * _fast_log2(val1));
}


/*
 * Haven't seen this elsewhere, probably because it is too obvious?
 * Anyway, these functions are intended for 32-bit floating point numbers
 * only and should work a bit faster than the regular ones.
 */
inline float fast_abs(float f)
{
    int i=((*(int*)&f)&0x7fffffff);
    return (*(float*)&i);
}

inline float fast_neg(float f)
{
    int i=((*(int*)&f)^0x80000000);
    return (*(float*)&i);
}

inline int fast_sgn(float f)
{
    return 1+(((*(int*)&f)>>31)<<1);
}

#endif // !_SG_FMATH_HXX

