/**
 * \file sg_random.h
 * Routines to handle random number generation and hide platform differences.
 */

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


#ifndef _SG_RANDOM_H
#define _SG_RANDOM_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


#define MT_N 624
#define MT_M 397

/**
 * Structure to hold MT algorithm state to easily allow independant
 * sets of random numbers with different seeds.
 */
typedef struct {unsigned int array[MT_N]; int index; } mt;

/**
 * Initialize a new MT state with a given seed.
 */
void mt_init(mt *mt, unsigned int seed);

/**
 * Initialize a new MT state with a seed that only
 * changes every 10 minutes.  Used to synchronize
 * multi-process deployments.
 */
void mt_init_time_10(mt *mt);

/**
 * Generate a new 32-bit random number based on the given MT state.
 */
unsigned int mt_rand32( mt *mt);

/**
 * Generate a new random number between [0.0, 1.0) based 
 * on the given MT state.
 */
double mt_rand(mt *mt);
 
/**
 * Seed the random number generater with time() so we don't see the
 * same sequence every time.
 */
void sg_srandom_time();

/**
 * Seed the random number generater with time() in 10 minute intervals
 * so we get the same sequence within 10 minutes interval.
 * This is useful for synchronizing two display systems.
 */
void sg_srandom_time_10();

/**
 * Seed the random number generater with your own seed so can set up
 * repeatable randomization.
 * @param seed random number generator seed
 */
void sg_srandom(unsigned int seed );

/**
 * Return a random number between [0.0, 1.0)
 * @return next "random" number in the "random" sequence
 */
double sg_random();


#ifdef __cplusplus
}
#endif


#endif // _SG_RANDOM_H


