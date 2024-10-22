// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright 2007-2017 by Erik Hofman.
// SPDX-FileCopyrightText: Copyright 2009-2017 by Adalin B.V.

/**
 * @file
 * @brief Butterworth Frequency Filter Fouier Computations
 */

#ifndef _SIMGEAR_FREQUENCY_FILTER_HXX
#define _SIMGEAR_FREQUENCY_FILTER_HXX

#include <cstdint>

namespace simgear {

// Every stage is a 2nd order filter
// Four stages therefore equals to an 8th order filter with a 48dB/oct slope.
#define SG_FREQFILTER_MAX_STAGES	4

class FreqFilter final {
    
private:

    float fs, Q, gain;
    float coeff[4*SG_FREQFILTER_MAX_STAGES];
    float hist[2*SG_FREQFILTER_MAX_STAGES];
    unsigned char no_stages;

    void butterworth_compute(float fc);
    void bilinear(float a0, float a1, float a2,
                  float b0, float b1, float b2,
                  float *k, int stage);
    void bilinear_s2z(float *a0, float *a1, float *a2, 
                      float *b0, float *b1, float *b2,
                      float fc, float fs, float *k, int stage);
    
public:
    
    FreqFilter(int order, float fs, float cutoff, float Qfactor = 1.0f);
    ~FreqFilter();      // non-virtual intentional
    
    void update( int16_t *data, unsigned int num );
};



class BitCrusher final {

private:
    float factor, devider;

public:

    // level ranges from 0.0f (all muted) to 1.0f (no change)
    BitCrusher(float level);
    ~BitCrusher();      // non-virtual intentional

    void update( int16_t *data, unsigned int num );
};

}; // namespace simgear

#endif // _SIMGEAR_FREQUENCY_FILTER_HXX
