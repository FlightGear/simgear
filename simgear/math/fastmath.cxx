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

