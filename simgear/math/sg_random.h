// sg_random.h -- routines to handle random number generation
//
// Written by Curtis Olson, started July 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id$


#ifndef _SG_RANDOM_H
#define _SG_RANDOM_H


#ifdef __cplusplus                                                          
extern "C" {                            
#endif                                   


// Seed the random number generater with time() so we don't see the
// same sequence every time
void sg_srandom_time(void);

// Seed the random number generater with your own seed so can set up
// repeatable randomization.
void sg_srandom( unsigned int seed );

// return a random number between [0.0, 1.0)
double sg_random(void);


#ifdef __cplusplus
}
#endif


#endif // _SG_RANDOM_H


