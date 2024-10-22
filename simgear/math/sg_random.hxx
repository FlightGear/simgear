// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 1997 Curtis L. Olson - http://www.flightgear.org/~curt

/**
 * @file
 * @brief Routines to handle random number generation
 */

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

/**
 * Initialize current state with a given seed.
 */
void pc_init(unsigned int seed);

/**
 * Initialize current state with a seed that only
 * changes every 10 minutes.  Used to synchronize
 * multi-process deployments.
 */
void pc_init_time_10();

/**
 * Return a 32-bit random number based on the current state.
 */
unsigned int pc_rand32();

/**
 * Return a double precision floating point random number
 * between [0.0, 1.0) based on the current state.
 */
double pc_rand();

/**
 * Return a double precision floating point random number
 * between [-10.0, 10.0] with a normal distribution of
 * average zero and standard deviation of one based on the
 * current state.
 */
double pc_norm();

/**
 * Return a 32-bit random number from a noise map.
 */
unsigned int pc_map_rand32(const unsigned int x, const unsigned int y, const unsigned int idx);

/**
 * Return a double precision floating point random number
 * between [0.0, 1.0) from a noise map.
 */
double pc_map_rand(const unsigned int x, const unsigned int y, const unsigned int idx);

/**
 * Return a double precision floating point random number
 * between [-10.0, 10.0] with a normal distribution of
 * average zero and standard deviation of one from a noise
 * map.
 */
double pc_map_norm(const unsigned int x, const unsigned int y, const unsigned int idx);


#ifdef __cplusplus
}
#endif


#endif // _SG_RANDOM_H


