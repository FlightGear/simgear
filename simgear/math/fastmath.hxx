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

double fast_exp(double y);
double fast_log(double val);
double fast_log2 (double val);
double fast_log10(double val);
void fast_BSL(double &x, register unsigned long shiftAmount);
void fast_BSR(double &x, register unsigned long shiftAmount);

#endif // !_SG_FMATH_HXX

