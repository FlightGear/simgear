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
 * Fast log() Function, by Laurent de Soras:
 * http://www.flipcode.com/cgi-bin/msg.cgi?showThread=Tip-Fastlogfunction&forum=totd&id=-1
 *
 */

/*
 * $Id$
 */



#include <fastmath.hxx>

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

    _eco.n.i = a*val + b_c;

    return _eco.d;
}


double fast_log2 (double val)
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
double fast_log (double val)
{
   return (fast_log2 (val) * 0.69314718f);
}

double fast_log10 (double val)
{
   return (fast_log (val) / fast_log (10));
}


/**
 * While we're on the subject, someone might have use for these as well?
 * Float Shift Left and Float Shift Right. Do what you want with this.
 */
void fast_BSL(double &x, register unsigned long shiftAmount) {

	*(unsigned long*)&x+=shiftAmount<<23;

}

void fast_BSR(double &x, register unsigned long shiftAmount) {

	*(unsigned long*)&x-=shiftAmount<<23;

}

