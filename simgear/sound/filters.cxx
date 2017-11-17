/*
 * Copyright 2007-2017 by Erik Hofman.
 * Copyright 2009-2017 by Adalin B.V.
 *
 * This file is part of SimGear
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the Lesser GNU General Public License as published
 *  by the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the Lesser GNU General Public License
 *  along with this program;
 *  if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <math.h>
#include <stdio.h>

#include <simgear/constants.h>
#include "filters.hxx"

namespace simgear {

FreqFilter::FreqFilter(int order, float sample_freq, float cutoff_freq, float Qfactor) {

    Q = Qfactor;
    fs = sample_freq;
    no_stages = order / 2;
    gain = order;

    butterworth_compute(cutoff_freq);

    for (unsigned int i = 0; i < 2*SG_FREQFILTER_MAX_STAGES; ++i) {
        hist[i] = 0.0f;
    }
}

FreqFilter::~FreqFilter() {
}

void FreqFilter::update( int16_t *data, unsigned int num) {

    if (num) {
        float k = gain;
        for (unsigned int stage = 0; stage < no_stages; ++stage) {

            float h0 = hist[2*stage + 0];
            float h1 = hist[2*stage + 1];
            unsigned int i = num;
            do {
                float nsmp, smp = data[i] * k;
                smp  = smp  + h0 * coeff[4*stage + 0];
                nsmp = smp  + h1 * coeff[4*stage + 1];
                smp  = nsmp + h0 * coeff[4*stage + 2];
                smp  = smp  + h1 * coeff[4*stage + 3];

                h1 = h0;
                h0 = nsmp;
                data[i] = smp;
            }
            while (--i);

            hist[2*stage + 0] = h0;
            hist[2*stage + 1] = h1;
            k = 1.0f;
        }
    }
}

inline void FreqFilter::bilinear(float a0, float a1, float a2,
                                 float b0, float b1, float b2,
                                 float *k, int stage) {
   a2 *= 4.0f;
   b2 *= 4.0f;
   a1 *= 2.0f;
   b1 *= 2.0f;

   float ad = a2 + a1 + a0;
   float bd = b2 + b1 + b0;

   *k *= ad/bd;

   coeff[4*stage + 0] = 2.0f*(-b2 + b0) / bd;
   coeff[4*stage + 1] = (b2 - b1 + b0) / bd;
   coeff[4*stage + 2] = 2.0f*(-a2 + a0) / ad;
   coeff[4*stage + 3] = (a2 - a1 + a0) / ad;

   // negate to prevent this is required every time the filter is applied.
   coeff[4*stage + 0] = -coeff[4*stage + 0];
   coeff[4*stage + 1] = -coeff[4*stage + 1];
}

// convert from S-domain to Z-domain
inline void FreqFilter::bilinear_s2z(float *a0, float *a1, float *a2,
                                     float *b0, float *b1, float *b2,
                                     float fc, float fs, float *k, int stage) {
   // prewarp
   float wp = 2.0f*tanf(SG_PI * fc/fs);

   *a2 /= wp*wp;
   *b2 /= wp*wp;
   *a1 /= wp;
   *b1 /= wp;

   bilinear(*a0, *a1, *a2, *b0, *b1, *b2, k, stage);
}

void FreqFilter::butterworth_compute(float fc) {

    // http://www.ti.com/lit/an/sloa049b/sloa049b.pdf
    static const float _Q[SG_FREQFILTER_MAX_STAGES][SG_FREQFILTER_MAX_STAGES]= {
        { 0.7071f, 1.0f,    1.0f,    1.0f    },   // 2nd order
        { 0.5412f, 1.3605f, 1.0f,    1.0f    },   // 4th order
        { 0.5177f, 0.7071f, 1.9320f, 1.0f    },   // 6th roder
        { 0.5098f, 0.6013f, 0.8999f, 2.5628f }    // 8th order
    };

    gain = 1.0f;

    int pos = no_stages-1;
    for (unsigned i = 0; i < no_stages; ++i)
    {
        float a2 = 0.0f;
        float a1 = 0.0f;
        float a0 = 1.0f;

        float b2 = 1.0f;
        float b1 = 1.0f/(_Q[pos][i] * Q);
        float b0 = 1.0f;

        // fill the filter coefficients
        bilinear_s2z(&a0, &a1, &a2, &b0, &b1, &b2, fc, fs, &gain, i);
    }
}



BitCrusher::BitCrusher(float level) {

    float bits = level * 15.0f;
    factor = powf(2.0f, bits);
    devider = 1.0f/factor;
}

BitCrusher::~BitCrusher() {
}

void BitCrusher::update( int16_t *data, unsigned int num ) {

    if (num && factor < 1.0f) {
        unsigned int i = num;
        do {
            float integral;

            modff(data[i]*devider, &integral);
            data[i] = integral*factor;
        } while (--i);
    }
}

}; // namespace simgear

