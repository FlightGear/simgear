// colours.h 
//
// -- This header file contains color related functions
//
// Copyright (C) 2003  FlightGear Flight Simulator
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id$


#ifndef _SG_COLORS_HXX
#define _SG_COLORS_HXX 1

#include <math.h>

#if defined( macintosh )
const float system_gamma = 1.4;

#elif defined (sgi)
const float system_gamma = 1.7;

#else	// others
const float system_gamma = 2.5;
#endif


// simple architecture independant gamma correction function.
inline void gamma_correct(float *color,
                          float reff = 2.5, float system = system_gamma)
{
   color[0] = pow(color[0], reff/system);
   color[1] = pow(color[1], reff/system);
   color[2] = pow(color[2], reff/system);
};

#endif // _SG_COLORS_HXX

