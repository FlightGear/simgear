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

#ifndef _SIMGEAR_FREQUENCY_FILTER_HXX
#define _SIMGEAR_FREQUENCY_FILTER_HXX

#include <cstdint>

namespace simgear {

// Every stage is a 2nd order filter
// Four stages therefore equals to an 8th order filter with a 48dB/oct slope.
#define SG_FREQFILTER_MAX_STAGES	4

class FreqFilter {
    
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
    ~FreqFilter();
    
    void update( int16_t *data, unsigned int num );
};



class BitCrusher {

private:
    float factor, devider;

public:

    // level ranges from 0.0f (all muted) to 1.0f (no change)
    BitCrusher(float level);
    ~BitCrusher();

    void update( int16_t *data, unsigned int num );
};

}; // namespace simgear

#endif // _SIMGEAR_FREQUENCY_FILTER_HXX
