// sg_random.c -- routines to handle random number generation
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$


/*
 * "Cleaned up" and simplified Mersenne Twister implementation.
 * Vastly smaller and more easily understood and embedded.  Stores the
 * state in a user-maintained structure instead of static memory, so
 * you can have more than one, or save snapshots of the RNG state.
 * Lacks the "init_by_array()" feature of the original code in favor
 * of the simpler 32 bit seed initialization.  Lacks the floating
 * point generator, which is an orthogonal problem not related to
 * random number generation.  Verified to be identical to the original
 * MT199367ar code through the first 10M generated numbers.
 */



#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <stdio.h>
#include <stdlib.h>         // for random(), srandom()
#include <time.h>           // for time() to seed srandom()        

#include "sg_random.h"

// Structure for the random number functions.
mt random_seed;

#define MT(i) mt->array[i]

void mt_init(mt *mt, unsigned int seed)
{
    int i;
    MT(0)= seed;
    for(i=1; i<MT_N; i++)
        MT(i) = (1812433253 * (MT(i-1) ^ (MT(i-1) >> 30)) + i);
    mt->index = MT_N+1;
}

void mt_init_time_10(mt *mt)
{
  mt_init(mt, (unsigned int) time(NULL) / 600);  
}

unsigned int mt_rand32(mt *mt)
{
    unsigned int i, y;
    if(mt->index >= MT_N) {
        for(i=0; i<MT_N; i++) {
            y = (MT(i) & 0x80000000) | (MT((i+1)%MT_N) & 0x7fffffff);
            MT(i) = MT((i+MT_M)%MT_N) ^ (y>>1) ^ (y&1 ? 0x9908b0df : 0);
        }
        mt->index = 0;
    }
    y = MT(mt->index++);
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    y ^= (y >> 18);
    return y;
}

double mt_rand(mt *mt)
{
    /* divided by 2^32-1 */ 
    return (double)mt_rand32(mt) * (1.0/4294967295.0); 
}

// Seed the random number generater with time() so we don't see the
// same sequence every time
void sg_srandom_time() {
    mt_init(&random_seed, (unsigned int) time(NULL));
}

// Seed the random number generater with time() in 10 minute intervals
// so we get the same sequence within 10 minutes interval.
// This is useful for synchronizing two display systems.
void sg_srandom_time_10() {
    mt_init(&random_seed, (unsigned int) time(NULL) / 600);
}

// Seed the random number generater with your own seed so can set up
// repeatable randomization.
void sg_srandom( unsigned int seed ) {
    mt_init(&random_seed, seed);
}

// return a random number between [0.0, 1.0)
double sg_random() {
  return mt_rand(&random_seed);
}

