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

void fast_BSL(float &x, register unsigned long shiftAmount);
void fast_BSR(float &x, register unsigned long shiftAmount);

inline float fast_log2 (float val)
{
   int * const    exp_ptr = reinterpret_cast <int *> (&val);
   int            x = *exp_ptr;
   const int      log_2 = ((x >> 23) & 255) - 128;
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


#endif // !_SG_FMATH_HXX

